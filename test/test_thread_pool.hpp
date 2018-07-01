#include "unit.hpp"
#include "thread_pool.hpp"

std::tuple<bool, std::string> test_ctor_invalid_thread_count() {
    try {
        threading::thread_pool tp(0);
        return {false, "no exception was thrown"};
    }
    catch (std::runtime_error) {
        return {true, "caught std::runtime_error"};
    }
    catch (...) {
        return {false, "incorrect exception was thrown"};
    }
}

unit::test_suite get_suite_thread_pool() {
    unit::test_suite suite("thread_pool.hpp");
    suite.add_test2(test_ctor_invalid_thread_count, "c'tor with invalid thread count");
    return suite;
}
