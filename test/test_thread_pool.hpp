#include "unit.hpp"
#include "thread_pool.hpp"

// TODO: add test for yield() callback
// TODO: add tests for all different task_status'es
// TODO: add performance/benchmark tests with comparison: "normal thread" vs "thread_pool(1)" vs "thread_pool(x)"

void test_ctor_invalid_thread_count() {
    unit::assert_throws([]() { threading::thread_pool tp(0); }, "c'tor with zero threads");
}

void test_threads_join() {
    const uint job_count {5};
    threading::thread_pool tp(3);
    std::atomic_uint counter {0};
    for (uint i = 0; i < job_count; i++) {
        tp.add([&counter](const std::function<bool (const std::shared_ptr<threading::task_t> &)> &) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            counter++;
        });
    }
    tp.wait();
    unit::assert_equals(job_count, counter.load(), "number of jobs completed");
}

void test_thread_throws_exception() {
    threading::thread_pool tp(1);
    std::shared_ptr<threading::task_t> task = tp.add([](const std::function<bool (const std::shared_ptr<threading::task_t> &)> &) { throw new std::runtime_error("exception within thread"); });
    tp.wait();
    unit::assert_true(threading::task_status::failed == task->status, "finished task status");
}

void test_dtor_abort_tasks_in_queue() {
    const auto start_time = std::chrono::high_resolution_clock::now();
    {
        threading::thread_pool tp(3);
        for (unsigned int i = 0; i < 100; i++) {
            tp.add([](const std::function<bool (const std::shared_ptr<threading::task_t> &)> &) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            });
            std::this_thread::yield();
        }
    }
    const auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> elapsed_time = end_time - start_time;
    unit::assert_true(elapsed_time.count() >= 100.0, "d'tor took at least 100ms: " + std::to_string(elapsed_time.count()) + "ms");
    unit::assert_true(elapsed_time.count() < 500.0, "d'tor took at most 500ms: " + std::to_string(elapsed_time.count()) + "ms");
}



unit::test_suite get_suite_thread_pool() {
    unit::test_suite suite("thread_pool.hpp");
    suite.add_test(test_ctor_invalid_thread_count, "c'tor with invalid thread count");
    suite.add_test(test_threads_join, "add more tasks than threads and wait for all jobs to complete");
    suite.add_test(test_thread_throws_exception, "handling of task which throws an unhandled exception");
    suite.add_test(test_dtor_abort_tasks_in_queue, "d'tor should abort all queued tasks and wait for all jobs to complete");
    return suite;
}

