#ifndef __THREAD_POOL_HPP_INCLUDED__
#define __THREAD_POOL_HPP_INCLUDED__

#include <future>
#include <thread>
#include <mutex>
#include <queue>
#include <stdexcept>

namespace threading {
    enum class task_status {
        pending,
        in_progress,
        done,
        failed
    };

    struct task_t {
        task_status status;
        std::function<void (std::function<bool (std::shared_ptr<task_t>)>)> callback;
    };

    class thread_pool {
        volatile bool destruct {false};
        volatile unsigned int active_threads {0}; // TODO: switch to unordered_set<int>?
        std::vector<std::thread> threads {};
        std::queue<std::shared_ptr<task_t>> task_queue {};
        std::condition_variable task_notifier {};
        std::mutex mutex {};

        inline bool has_completed(const std::shared_ptr<task_t> wait_for_task) {
            if (wait_for_task == nullptr)
                return true;

            {
                //std::lock_guard<std::mutex> global_lock(mutex);
                return (wait_for_task->status == task_status::done || wait_for_task->status == task_status::failed);
            }
        }

        bool thread_yield(const std::shared_ptr<task_t> wait_for_task = nullptr) {
            if (wait_for_task != nullptr && has_completed(wait_for_task))
                return false;

            std::unique_lock<std::mutex> thread_lock(mutex, std::defer_lock);
            thread_lock.lock();
            if (task_queue.size() == 0) {
                thread_lock.unlock();
                if (wait_for_task != nullptr && !has_completed(wait_for_task)) {
                    std::this_thread::yield();
                    return true;
                }
                return false;
            }

            // Pop next item
            auto task = task_queue.front();
            task_queue.pop();
            thread_lock.unlock();

            // Execute task
            execute_task(*task);

            std::this_thread::yield();
            return true;
        }

        void execute_task(task_t &task) {
            try {
                task.status = task_status::in_progress;
                task.callback([&] (const std::shared_ptr<task_t> wait_for_task = nullptr) { return thread_yield(wait_for_task); });
                task.status = task_status::done;
            }
            catch (...) {
                task.status = task_status::failed;
            }
        }

        void thread_loop() {
            std::unique_lock<std::mutex> thread_lock(mutex, std::defer_lock);

            while (!destruct) {
                thread_lock.lock();
                if (task_queue.size() == 0) {
                    // Wait for new items
                    task_notifier.wait(thread_lock);

                    if (task_queue.size() == 0 || destruct) {
                        thread_lock.unlock();
                        continue;
                    }
                }

                active_threads++;

                // Pop next item
                auto task = task_queue.front();
                task_queue.pop();
                thread_lock.unlock();

                // Execute task
                execute_task(*task);

                thread_lock.lock();
                active_threads--;
                thread_lock.unlock();
            }
        }

        public:
            thread_pool(const unsigned int thread_count) {
                if (thread_count < 1)
                    throw std::runtime_error("Thread count must be at least one: " + std::to_string(thread_count));

                for (unsigned int i = 0; i < thread_count; i++)
                    threads.emplace_back(&thread_pool::thread_loop, this);
            }

            ~thread_pool() {
                destruct = true;
                {
                    std::lock_guard<std::mutex> global_lock(mutex);
                    task_notifier.notify_all();
                }

                for (auto &thread: threads)
                    thread.join();
                threads.clear();
            }

            std::shared_ptr<task_t> add(const std::function<void (std::function<bool (std::shared_ptr<task_t>)>)> &task) {
                std::shared_ptr<task_t> temp = std::make_shared<task_t>();
                temp->status = task_status::pending;
                temp->callback = task;
                {
                    std::lock_guard<std::mutex> global_lock(mutex);
                    task_queue.push(temp);
                }
                task_notifier.notify_one();
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
            inline bool idle() {
                std::lock_guard<std::mutex> global_lock(mutex);
                return (active_threads == 0 && task_queue.size() == 0);
            }

            void wait() {
                // TODO: replace while-sleep w/ event signal on change
                while(true) {
                    if (idle())
                        break;
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
            }
    };
}

#endif //__THREAD_POOL_HPP_INCLUDED__
