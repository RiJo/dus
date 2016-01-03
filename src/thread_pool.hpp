#ifndef __THREAD_POOL_HPP_INCLUDED__
#define __THREAD_POOL_HPP_INCLUDED__

#include <future>
#include <thread>
#include <mutex>
#include <queue>
#include <stdexcept>

namespace threading {
    struct thread_pool_task_t {
        std::function<void (std::function<bool ()>)> callback;
    };

    class thread_pool {
        volatile bool destruct {false};
        volatile unsigned int active_threads {0};
        std::vector<std::thread> threads {};
        std::queue<thread_pool_task_t> task_queue {};
        std::condition_variable task_notifier {};
        std::mutex mutex {};

        bool thread_yield(const unsigned int thread_index) {
            std::unique_lock<std::mutex> thread_lock(mutex, std::defer_lock);
            thread_lock.lock();
            if (task_queue.size() == 0) {
                thread_lock.unlock();
                return false;
            }

            // Pop next item
            auto task = task_queue.front();
            task_queue.pop();
            thread_lock.unlock();

            // Execute task
            execute_task(thread_index, task);
            return true;
        }

        void execute_task(const unsigned int thread_index, const thread_pool_task_t &task) {
            try {
                task.callback([&] () { return thread_yield(thread_index); });
            }
            catch (...) {
                // TODO: notify?
            }
        }

        void thread_loop(const unsigned int thread_index) {
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
                execute_task(thread_index, task);

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
                    threads.emplace_back(&thread_pool::thread_loop, this, i);
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

            void add(const thread_pool_task_t &task) {
                {
                    std::lock_guard<std::mutex> global_lock(mutex);
                    task_queue.push(task);
                }
                task_notifier.notify_one();
            }

            void add(std::initializer_list<thread_pool_task_t> args) {
                if (args.size() == 0)
                    return;
                if (args.size() == 1)
                    return add(*args.begin());

                {
                    std::lock_guard<std::mutex> global_lock(mutex);
                    for (auto task: args)
                        task_queue.push(task);
                }
                task_notifier.notify_all();
            }

            template<typename... T>
            void add(const thread_pool_task_t &task, T... args) {
                add(task);
                add(args...);
            }

            bool idle() {
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
