#ifndef __THREAD_POOL_HPP_INCLUDED__
#define __THREAD_POOL_HPP_INCLUDED__

#include <future>
#include <thread>
#include <mutex>
#include <queue>
#include <stdexcept>

#include <iostream>

namespace threading {
    struct thread_pool_task_t {
        std::function<void ()> task;
    };

    class thread_pool {
        volatile bool destruct {false};
        std::vector<std::thread> threads {};
        std::queue<thread_pool_task_t> task_queue {};

        std::condition_variable cv {};
        std::mutex m {};

        void thread_loop(unsigned int thread_index) {
            while (!destruct) {
                m.lock();
                if (task_queue.size() == 0) {
                    // Wait for new items
                    std::unique_lock<std::mutex> lk(m, std::adopt_lock);
                    cv.wait(lk);

                    if (task_queue.size() == 0 || destruct) {
                        m.unlock();
                        continue;
                    }
                }

                // Pop next item
                auto item = task_queue.front();
                task_queue.pop();
                m.unlock();

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
                cv.notify_all();
                for (auto &thread: threads)
                    thread.join();
                threads.clear();
            }

            void add_task(const thread_pool_task_t &task) {
                {
                    std::lock_guard<std::mutex> lk(m);
                    task_queue.push(task);
                }
                cv.notify_one();
            }

            void add_tasks(std::initializer_list<thread_pool_task_t> args) {
                {
                    std::lock_guard<std::mutex> lk(m);
                    for (auto task: args)
                        task_queue.push(task);
                }
                cv.notify_all();
            }

            template< typename... T>
            void add_tasks(const thread_pool_task_t &task, T... args) {
                {
                    std::lock_guard<std::mutex> lk(m);
                    task_queue.push(task);
                    //std::cout << "THREAD TASK ENQUEUED" << std::endl;
                }
                add_task(args...);
            }
    };
}

#endif //__THREAD_POOL_HPP_INCLUDED__
