#include "unit.hpp"
#include "thread_pool.hpp"

void test_ctor_invalid_thread_count() {
    unit::assert_throws([]() { threading::thread_pool tp(0); }, "c'tor with zero threads");
}

unit::test_suite get_suite_thread_pool() {
    unit::test_suite suite("thread_pool.hpp");
    suite.add_test(test_ctor_invalid_thread_count, "c'tor with invalid thread count");
    return suite;
}
