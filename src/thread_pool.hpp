#ifndef __THREAD_POOL_HPP_INCLUDED__
#define __THREAD_POOL_HPP_INCLUDED__

#include <future>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>
#include <set>
#include <queue>
#include <stdexcept>

#include <iostream>

namespace threading {
    enum class task_status {
        pending,
        in_progress,
        done,
        failed,
        aborted
    };

    struct worker_t {
        const unsigned int index;
        std::thread thread;
    };

    struct task_t {
        task_status status;
        const std::function<void (const std::function<bool (const std::shared_ptr<task_t> &)> &)> callback;
    };

    class thread_pool {
        std::atomic_bool destruct {false};
        std::atomic_bool abort {false};
        const unsigned int thread_count;
        std::vector<worker_t> workers {};
        std::map<unsigned int, std::queue<std::shared_ptr<task_t>>> task_queues {};
        std::set<unsigned int> workers_idle {};
        std::map<unsigned int, std::mutex> mutexes {};
        std::map<unsigned int, std::condition_variable> task_notifiers {};
        std::atomic_ulong next_worker_index {0};

        std::mutex wait_mutex {};
        std::condition_variable wait_notifier {};

        inline bool has_completed(const std::shared_ptr<task_t> &wait_for_task) const {
            if (wait_for_task == nullptr)
                return true;

            return (wait_for_task->status == task_status::done || wait_for_task->status == task_status::failed || wait_for_task->status == task_status::aborted);
        }

        std::shared_ptr<task_t> steal_task(const unsigned int worker_index) {
            std::this_thread::yield(); // relaxation: let potential owner threads get task first

            // TODO: inefficient when most threads are asleep (cv.wait()) and one thread has many tasks
            for (unsigned int i = 1; i < thread_count; i++) {
                const unsigned int other_worker_index = (worker_index + i) % thread_count;
                {
                    std::lock_guard<std::mutex> other_lock(mutexes[other_worker_index]);
                    if (task_queues[other_worker_index].size() <= (workers_idle.find(other_worker_index) != workers_idle.end() ? 1 : 0))
                        continue;
#ifdef DEBUG
                    std::cout << "[" << worker_index << "] stealing task from [" << other_worker_index << "]" << std::endl;
#endif
                    // steal task
                    std::shared_ptr<task_t> task = std::move(task_queues[other_worker_index].front());
                    task_queues[other_worker_index].pop();
                    return task;
                }
            }
            return nullptr;
        }

        bool thread_yield(const unsigned int worker_index, const std::shared_ptr<task_t> &wait_for_task = nullptr) {
            if (wait_for_task != nullptr && has_completed(wait_for_task))
                return false;

            std::unique_lock<std::mutex> thread_lock(mutexes[worker_index]);
            if (task_queues[worker_index].size() == 0) {
                thread_lock.unlock();
                if (wait_for_task != nullptr && !has_completed(wait_for_task)) {
                    std::shared_ptr<task_t> task = steal_task(worker_index);
                    if (task != nullptr)
                        execute_task(worker_index, *task);
                    std::this_thread::yield();
                }
                return true;
            }

            // pop and execute next task
            auto task = task_queues[worker_index].front();
            task_queues[worker_index].pop();
            thread_lock.unlock();
            execute_task(worker_index, *task);
            std::this_thread::yield();
            return true;
        }

        inline void execute_task(const unsigned int worker_index, task_t &task) {
            if (abort.load()) {
                task.status = task_status::aborted;
#ifdef DEBUG
                std::cout << "[" << worker_index << "] arborting task" << std::endl;
#endif
                return;
            }

            try {
                task.status = task_status::in_progress;
                task.callback([this, worker_index] (const std::shared_ptr<task_t> &wait_for_task = nullptr) { return thread_yield(worker_index, wait_for_task); });
                task.status = task_status::done;
#ifdef DEBUG
                std::cout << "[" << worker_index << "] completed task" << std::endl;
#endif
            }
            catch (...) {
                task.status = task_status::failed;
            }
        }

        void thread_loop(const unsigned int worker_index) {
            std::unique_lock<std::mutex> thread_lock(mutexes[worker_index], std::defer_lock);
            while (!destruct.load()) {
                std::shared_ptr<task_t> task = nullptr;

                thread_lock.lock();
                if (task_queues[worker_index].size() == 0) {
                    thread_lock.unlock(); // release lock to prevent deadlocks while stealing

                    task = steal_task(worker_index);
                    if (task == nullptr) {
                        // wait for new task
                        thread_lock.lock();
                        if (task_queues[worker_index].size() == 0) {
                            {
                                std::lock_guard<std::mutex> wait_lock(wait_mutex);
                                workers_idle.insert(worker_index);
                                wait_notifier.notify_all(); // tell waiters to evaluate task queues
                            }
#ifdef DEBUG
                            std::cout << "[" << worker_index << "] idle" << std::endl;
#endif
                            task_notifiers[worker_index].wait(thread_lock);
                            workers_idle.erase(worker_index);
                            if (task_queues[worker_index].size() == 0) {
                                thread_lock.unlock();
                                continue; // stolen by other thread
                            }
#ifdef DEBUG
                            std::cout << "[" << worker_index << "] wake up" << std::endl;
#endif
                        }
                    }
                }

                // note: thread_lock locked if (task == nullptr)

                if (destruct.load()) {
                    if (task == nullptr)
                        thread_lock.unlock();
                    break;
                }

                if (task == nullptr) {
                    // pop next task
                    task = task_queues[worker_index].front();
                    task_queues[worker_index].pop();
                    thread_lock.unlock();
                }

                execute_task(worker_index, *task);
            }
        }

        void safe_thread_loop(const unsigned int worker_index) {
            try {
                thread_loop(worker_index);
#ifdef DEBUG
                std::cout << "[" << worker_index << "] terminated" << std::endl;
#endif
            }
            catch (const std::exception &e) {
                std::cerr << "[" << worker_index << "] uncaught exception: " << e.what() << std::endl;
                throw;
            }
        }

        public:
            thread_pool() = delete;

            thread_pool(const unsigned int thread_count_) : thread_count(thread_count_) {
#ifdef DEBUG
                std::cout << "c'tor" << std::endl;
#endif
                if (thread_count < 1)
                    throw std::runtime_error("Thread count must be at least one: " + std::to_string(thread_count));

                for (unsigned int i = 0; i < thread_count; i++) {
                    mutexes[i];
                    task_notifiers[i];
                    task_queues[i];
                }
                for (unsigned int i = 0; i < thread_count; i++) {
                    workers.emplace_back(worker_t{i, std::thread{&thread_pool::safe_thread_loop, this, i}});
                }
            }

            ~thread_pool() {
#ifdef DEBUG
                std::cout << "d'tor" << std::endl;
#endif
                // abort all tasks and wait for threads to idle
#ifdef DEBUG
                std::cout << "abort all tasks..." << std::endl;
#endif
                abort.store(true);
                wait();

                // notify all threads to terminate
#ifdef DEBUG
                std::cout << "destruct threads..." << std::endl;
#endif
                destruct.store(true);
                for (unsigned int worker_index = 0; worker_index < thread_count; worker_index++)
                    task_notifiers[worker_index].notify_all();

#ifdef DEBUG
                std::cout << "join threads..." << std::endl;
#endif
                for (worker_t &worker: workers)
                    worker.thread.join();

                workers.clear();
#ifdef DEBUG
                std::cout << "d'tor completed" << std::endl;
#endif
            }

            std::shared_ptr<task_t> add(const std::function<void (const std::function<bool (const std::shared_ptr<task_t> &)> &)> task) {
                std::shared_ptr<task_t> temp = std::make_shared<task_t>(task_t{task_status::pending, std::move(task)});

                // TODO: only perform this if add() is called by thread_pool's internal threads
                //~ if (active_threads.load() == threads.size()) {
                    //~ // No pending threads, execute immediately in current thread
                    //~ execute_task(*temp);
                    //~ return temp;
                //~ }

                // TODO: begin with locating empty task queue, else perform logic below
                unsigned int next_index = next_worker_index.fetch_add(1) % thread_count;
                {
                    std::lock_guard<std::mutex> global_lock(mutexes[next_index]);
                    task_queues[next_index].push(temp);
                    task_notifiers[next_index].notify_one();
#ifdef DEBUG
                    std::cout << "[" << next_index << "] task added" << std::endl;
#endif
                }

                return temp;
            }
#if FALSE
            // TODO: re-implement
            std::vector<std::shared_ptr<task_t>> add(std::initializer_list<std::function<void (std::function<bool ()>)>> args) {
                if (args.size() == 0)
                    return 0;
                if (args.size() == 1)
                    return add(*args.begin());

                std::vector<std::shared_ptr<task_t>> result;
                {
                    // TODO: don't lock for all these instructions!
                    std::lock_guard<std::mutex> global_lock(mutex);
                    for (auto task: args) {
                        std::shared_ptr<task_t> temp = std::make_shared<task_t>();
                        temp->callback = task;
                        result.push_back(temp);
                        task_queue.push(temp);
                    }
                }
                task_notifier.notify_all();
                return result;
            }

            // TODO: re-implement
            template<typename... T>
            std::shared_ptr<task_t> add(const std::function<void (std::function<bool ()>)> &task, T... args) {
                add(task);
                return add(args...);
            }
#endif

            bool all_tasks_idle() {
                for (unsigned int worker_index = 0; worker_index < thread_count; worker_index++) {
                    if (workers_idle.find(worker_index) != workers_idle.end()) { // TODO: is this check thread safe?
                        std::lock_guard<std::mutex> worker_lock(mutexes[worker_index]);
                        if (task_queues[worker_index].size() == 0)
                            continue;
                    }
                    return false;
                }
                return true;
            }

            void wait() {
                std::unique_lock<std::mutex> wait_lock(wait_mutex);
                bool in_progress;
                do {
                    in_progress = !all_tasks_idle();
                    if (in_progress)
                        wait_notifier.wait(wait_lock);
                } while (in_progress);

                wait_lock.unlock();
#ifdef DEBUG
                std::cout << "wait complete" << std::endl;
#endif
            }
    };
}

#endif //__THREAD_POOL_HPP_INCLUDED__
