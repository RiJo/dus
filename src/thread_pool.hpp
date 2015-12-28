#ifndef __THREAD_POOL_HPP_INCLUDED__
#define __THREAD_POOL_HPP_INCLUDED__

#include <future>
#include <thread>
#include <mutex>
#include <queue>
#include <stdexcept>

namespace threading {
    struct thread_pool_task_t {
        std::function<void ()> task;
    };

    class thread_pool {
        volatile bool destruct {false};
        std::vector<std::thread> threads {};
        std::queue<thread_pool_task_t> task_queue {};

        std::condition_variable cv {};
        std::mutex mutex {};

        void thread_loop(unsigned int thread_index) {
            std::unique_lock<std::mutex> thread_lock(mutex, std::defer_lock);

            while (!destruct) {
                thread_lock.lock();
                if (task_queue.size() == 0) {
                    // Wait for new items
                    cv.wait(thread_lock);

                    if (task_queue.size() == 0 || destruct) {
                        thread_lock.unlock();
                        continue;
                    }
                }

                // Pop next item
                auto item = task_queue.front();
                task_queue.pop();
                thread_lock.unlock();

                // Execute task
                try {
                    item.task();
                }
                catch (...) {
                }
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
                    cv.notify_all();
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
                cv.notify_one();
            }

            void add(std::initializer_list<thread_pool_task_t> args) {
                {
                    std::lock_guard<std::mutex> global_lock(mutex);
                    for (auto task: args)
                        task_queue.push(task);
                }
                cv.notify_all();
            }

            template< typename... T>
            void add(const thread_pool_task_t &task, T... args) {
                {
                    std::lock_guard<std::mutex> global_lock(mutex);
                    task_queue.push(task);
                    //std::cout << "THREAD TASK ENQUEUED" << std::endl;
                }
                add(args...);
            }
    };
}

#endif //__THREAD_POOL_HPP_INCLUDED__
