#include "unit.hpp"
#include "thread_pool.hpp"

// TODO: move to test_unit.hpp

void test_assert_invert_pass() {
    unit::assert_invert([]() { throw unit::assertion_error("ignored"); });
}

void test_assert_pass() {
    unit::assert_invert([]() {
            unit::assert("test");
    });
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

void test_count_failures_zero() {
    unit::test_suite suite("test");
    suite.add_test([]() {});
    unit::assert_equals((size_t)0, suite.count_failure(), "assert_equals(0, suite.count_failure())");
}

void test_count_failures_nonzero() {
    unit::test_suite suite("test");
    suite.add_test([]() {}, "pass");
    suite.add_test([]() { unit::assert("assert"); }, "fail");
    unit::assert_equals((size_t)1, suite.count_failure(), "assert_equals(1, suite.count_failure())");
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
    suite_unit.add_test(test_assert_invert_pass, "test_assert_invert_pass");
    suite_unit.add_test(test_assert_pass, "test_assert_pass");
    suite_unit.add_test(test_assert_true_pass, "test_assert_true_pass");
    suite_unit.add_test(test_assert_true_fail, "test_assert_true_fail");
    suite_unit.add_test(test_assert_false_pass, "test_assert_false_pass");
    suite_unit.add_test(test_assert_false_fail, "test_assert_false_fail");
    suite_unit.add_test(test_assert_equals_pass, "test_assert_equals_pass");
    suite_unit.add_test(test_assert_equals_fail, "test_assert_equals_fail");
    suite_unit.add_test(test_assert_throws_pass, "test_assert_throws_pass");
    suite_unit.add_test(test_assert_throws_fail, "test_assert_throws_fail");
    suite_unit.add_test(test_count_failures_zero, "test_count_failures_zero");
    suite_unit.add_test(test_count_failures_nonzero, "test_count_failures_nonzero");
    std::cout << suite_unit.to_string() << std::endl;


    unit::test_suite suite("thread_pool.hpp");
    suite.add_test2(test_ctor_invalid_thread_count, "c'tor with invalid thread count");
    std::cout << suite.to_string() << std::endl;
    return suite_unit.count_failure() + suite.count_failure();
}
