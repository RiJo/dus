#ifndef __THREAD_POOL_HPP_INCLUDED__
#define __THREAD_POOL_HPP_INCLUDED__

#include <future>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>
#include <queue>
#include <stdexcept>

#include <iostream>

namespace threading {
    enum class task_status {
        pending,
        in_progress,
        done,
        failed
    };

    struct worker_t {
        unsigned int index;
        std::thread worker;
    };

    struct task_t {
        task_status status;
        const std::function<void (const std::function<bool (const std::shared_ptr<task_t> &)> &)> callback;
    };

    class thread_pool {
        std::atomic_bool destruct {false};
        const unsigned int thread_count;
        std::vector<worker_t> threads {};
        std::map<unsigned int, std::queue<std::shared_ptr<task_t>>> task_queues {};
        std::map<unsigned int, bool> workers_idle {};
        std::map<unsigned int, std::mutex> mutexes {};
        std::map<unsigned int, std::condition_variable> task_notifiers {};
        std::atomic_uint next_worker_index {0};

        std::mutex wait_mutex {};
        std::condition_variable wait_notifier {};

        inline bool has_completed(const std::shared_ptr<task_t> &wait_for_task) {
            if (wait_for_task == nullptr)
                return true;

            {
                //std::lock_guard<std::mutex> global_lock(mutex);
                return (wait_for_task->status == task_status::done || wait_for_task->status == task_status::failed);
            }
        }

        bool thread_yield(unsigned int worker_index, const std::shared_ptr<task_t> &wait_for_task = nullptr) {
            if (wait_for_task != nullptr && has_completed(wait_for_task))
                return false;

            // TODO: look in other queues if current one is empty
            std::unique_lock<std::mutex> thread_lock(mutexes[worker_index]);
            if (task_queues[worker_index].size() == 0) {
                thread_lock.unlock();
                if (wait_for_task != nullptr && !has_completed(wait_for_task)) {
                    // TODO: copied from thread_loop()
                    for (unsigned int i = 1; i < thread_count; i++) {
                        unsigned int other_worker_index = (worker_index + i) % thread_count;
                        {
                            std::shared_ptr<task_t> task = nullptr;
                            {
                                std::lock_guard<std::mutex> other_lock(mutexes[other_worker_index]);
                                if (task_queues[other_worker_index].size() == 0)
                                    continue;

                                // Steal item
                                task = std::move(task_queues[other_worker_index].front());
                                task_queues[other_worker_index].pop();
#ifdef DEBUG
                                std::cout << "[" << worker_index << "] stole task from [" << other_worker_index << "] - yield" << std::endl;
#endif
                            }

                            if (task == nullptr)
                                throw std::runtime_error("WOOT?");
                            execute_task(worker_index, *task);

                            std::this_thread::yield();
                            return true;
                        }
                    }

                    std::this_thread::yield();
                }
                return true;
            }

            // Pop next item
#ifdef DEBUG
            if (task_queues[worker_index].size() == 0)
                throw std::runtime_error("task queue is empty in thread_yield..."); // TODO: unlock mutex?
#endif
            auto task = task_queues[worker_index].front();
#ifdef DEBUG
            if (task == nullptr)
                throw std::runtime_error("popped a nullptr in thread_yield..."); // TODO: unlock mutex?
#endif
            task_queues[worker_index].pop();

            thread_lock.unlock();

            // Execute task
            if (task == nullptr)
                throw std::runtime_error("WOOT?");
            execute_task(worker_index, *task);

            std::this_thread::yield();
            return true;
        }

        inline void execute_task(unsigned int worker_index, task_t &task) {
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

        void thread_loop(unsigned int worker_index) {
            std::unique_lock<std::mutex> thread_lock(mutexes[worker_index], std::defer_lock);
            //~ std::cout << "Creating [" << worker_index << "]" << std::endl;
            while (!destruct.load()) {
                std::shared_ptr<task_t> task = nullptr;

                thread_lock.lock();
                if (task_queues[worker_index].size() == 0) {
                    // TODO: wait for task for 10ms(?, relaxation) before stealing from other thread

                    thread_lock.unlock(); // release lock to prevent deadlocks while stealing

                    // TODO: inefficient when most threads are asleep (cv.wait()) and one thread has many tasks
                    for (unsigned int i = 1; i < thread_count; i++) {
                        unsigned int other_worker_index = (worker_index + i) % thread_count;
                        {
                            std::lock_guard<std::mutex> other_lock(mutexes[other_worker_index]);
                            if (task_queues[other_worker_index].size() == 0)
                                continue;

                            // Steal item
                            task = std::move(task_queues[other_worker_index].front());
                            task_queues[other_worker_index].pop();
#ifdef DEBUG
                            std::cout << "[" << worker_index << "] stole task from [" << other_worker_index << "]" << std::endl;
#endif
                            break;
                        }
                    }

                    if (task == nullptr) {
                        // wait for new items
                        thread_lock.lock();
                        if (task_queues[worker_index].size() == 0) {
                            {
                                std::lock_guard<std::mutex> wait_lock(wait_mutex);
                                workers_idle[worker_index] = true;
                                wait_notifier.notify_all(); // tell waiters to evaluate task queues
                            }
#ifdef DEBUG
                            std::cout << "[" << worker_index << "] wait..." << std::endl;
#endif
                            task_notifiers[worker_index].wait(thread_lock);
                            workers_idle[worker_index] = false;
                            if (task_queues[worker_index].size() == 0) {
                                thread_lock.unlock();
                                continue; // stolen by other thread
                            }
#ifdef DEBUG
                            std::cout << "[" << worker_index << "] notified of new task" << std::endl;
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
                    // pop item in current queue
                    task = task_queues[worker_index].front();
#ifdef DEBUG
                    if (task == nullptr) {
                        thread_lock.unlock();
                        throw std::runtime_error("popped a nullptr in thread_loop...");
                    }
#endif
                    task_queues[worker_index].pop();
                    thread_lock.unlock();
                }

                // Execute task
                if (task == nullptr)
                    throw std::runtime_error("WOOT?");
                execute_task(worker_index, *task);
            }
        }

        void safe_thread_loop(unsigned int worker_index) {
            try {
                thread_loop(worker_index);
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
                    workers_idle[i];
                    mutexes[i];
                    task_notifiers[i];
                    task_queues[i];
                }
                for (unsigned int i = 0; i < thread_count; i++) {
                    threads.emplace_back(worker_t{i, std::thread{&thread_pool::safe_thread_loop, this, i}});
                }
            }

            ~thread_pool() {
                destruct.store(true);
                {
#ifdef DEBUG
                    std::cout << "d'tor" << std::endl;
#endif
                    for (unsigned int worker_index = 0; worker_index < thread_count; worker_index++)
                        task_notifiers[worker_index].notify_all();
                    //~ std::lock_guard<std::mutex> global_lock(mutex);
                    //~ task_notifier.notify_all();
                }

                for (worker_t &t: threads)
                    t.worker.join();

                // TODO: clear all queues to make blocked wait() evaluate properly
                wait_notifier.notify_all();

                threads.clear();
            }

            std::shared_ptr<task_t> add(const std::function<void (const std::function<bool (const std::shared_ptr<task_t> &)> &)> task) {
                std::shared_ptr<task_t> temp = std::make_shared<task_t>(task_t{task_status::pending, std::move(task)});
                //~ temp->status = task_status::pending;
                //~ temp->callback = task;

                // TODO: only perform this if add() is called by thread_pool's internal threads
                //~ if (active_threads.load() == threads.size()) {
                    //~ // No pending threads, execute immediately in current thread
                    //~ execute_task(*temp);
                    //~ return temp;
                //~ }

                // TODO: begin with locating empty task queue, else perform logic below
                unsigned int next_index = next_worker_index.fetch_add(1);
                if (next_index == thread_count) {
                    next_worker_index.store(1); // TODO: not atomic due to previous fetch_add(): if add() is called in parallel
                    next_index = 0;
                }

                {
                    std::lock_guard<std::mutex> global_lock(mutexes[next_index]);
                    task_queues[next_index].push(temp);
                    task_notifiers[next_index].notify_one();
#ifdef DEBUG
                    std::cout << "[" << next_index << "] new task added" << std::endl;
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

#if FALSE
            inline bool idle() {
                std::lock_guard<std::mutex> global_lock(mutex);
                return (active_threads.load() == 0 && task_queue.size() == 0);
            }
#endif

            void wait() {
                std::unique_lock<std::mutex> wait_lock(wait_mutex);
                bool in_progress = true;
                while (in_progress) {
                    in_progress = false;
                    for (unsigned int worker_index = 0; worker_index < thread_count; worker_index++) {
                        if (workers_idle[worker_index]) {
                            std::lock_guard<std::mutex> worker_lock(mutexes[worker_index]);
                            if (task_queues[worker_index].size() == 0)
                                continue;
                        }

                        // TODO: we need to know active threads as well here..
#ifdef DEBUG
                        std::cout << "wait() - task [" << worker_index << "] isn't idle" << std::endl;
#endif
                        in_progress = true;
                        break;
                    }

                    if (in_progress) {
                        wait_notifier.wait(wait_lock);
#ifdef DEBUG
                        std::cout << "wait() - re-evaluate" << std::endl;
#endif
                    }
                }

                wait_lock.unlock();
#ifdef DEBUG
                std::cout << "wait() - done" << std::endl;
#endif
            }
    };
}

#endif //__THREAD_POOL_HPP_INCLUDED__
