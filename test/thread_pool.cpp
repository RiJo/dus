#include "unit.hpp"
#include "thread_pool.hpp"

// TODO: move to test_unit.hpp

void test_assert_invert_pass() {
    unit::assert_invert([]() { throw unit::assertion_error("ignored"); });
}

void test_assert_true_pass() {
    unit::assert_true(true, "assert_true(true)");
}

void test_assert_true_fail() {
    unit::assert_invert([]() {
        unit::assert_true(false, "assert_true(false)");
    });
}

void test_assert_false_pass() {
    unit::assert_false(false, "assert_false(false)");
}

void test_assert_false_fail() {
    unit::assert_invert([]() {
        unit::assert_false(true, "assert_false(true)");
    });
}

void test_assert_equals_pass() {
    unit::assert_equals("expected", "expected", "assert_equals(\"expected\", \"expected\")");
}

void test_assert_equals_fail() {
    unit::assert_invert([]() {
        unit::assert_equals("expected", "actual", "assert_equals(\"expected\", \"actual\")");
    });
}

void test_assert_throws_pass() {
    unit::assert_throws([]() { throw std::runtime_error("message"); }, "assert_throws(throw)");
}

void test_assert_throws_fail() {
    unit::assert_invert([]() {
        unit::assert_throws([]() {}, "assert_throws(nop)");
    });
}

// TODO: move to test_unit.hpp


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

int main(int, const char **) {
    unit::test_suite suite_unit("unit.hpp");
    suite_unit.add_test(test_assert_invert_pass, "assert_invert(throw)");
    suite_unit.add_test(test_assert_true_pass, "assert_true(true)");
    suite_unit.add_test(test_assert_true_fail, "assert_true(false)");
    suite_unit.add_test(test_assert_false_pass, "assert_false(false)");
    suite_unit.add_test(test_assert_false_fail, "assert_false(true)");
    suite_unit.add_test(test_assert_equals_pass, "assert_equals(\"expected\", \"expected\")");
    suite_unit.add_test(test_assert_equals_fail, "assert_equals(\"expected\", \"actual\")");
    suite_unit.add_test(test_assert_throws_pass, "TBD");
    suite_unit.add_test(test_assert_throws_fail, "TBD");
    std::cout << suite_unit.to_string() << std::endl;


    unit::test_suite suite("thread_pool.hpp");
    suite.add_test2(test_ctor_invalid_thread_count, "c'tor with invalid thread count");
    std::cout << suite.to_string() << std::endl;
    return 0;
}
