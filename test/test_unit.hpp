#include "unit.hpp"

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
    unit::assert_throws(std::string{}, []() { throw std::string("message"); }, "assert_throws(throw)");
}

void test_assert_throws_fail() {
    unit::assert_invert([]() {
        unit::assert_throws(std::string{}, []() {}, "assert_throws(nop)");
    });
}

void test_assert_throws_wrong() {
    unit::assert_invert([]() {
        unit::assert_throws(std::string{}, []() { throw std::runtime_error("test"); }, "assert_throws(wrong)");
    });
}

void test_count_failures_zero() {
    unit::test_suite suite("test");
    suite.add_test([]() {});
    suite();
    unit::assert_equals((size_t)0, suite.count_failure(), "assert_equals(0, suite.count_failure())");
}

void test_count_failures_nonzero() {
    unit::test_suite suite("test");
    suite.add_test([]() {}, "pass");
    suite.add_test([]() { unit::assert("assert"); }, "fail");
    suite();
    unit::assert_equals((size_t)1, suite.count_failure(), "assert_equals(1, suite.count_failure())");
}

unit::test_suite get_suite_unit() {
    unit::test_suite suite("unit.hpp");
    suite.add_test(test_assert_invert_pass, "test_assert_invert_pass");
    suite.add_test(test_assert_pass, "test_assert_pass");
    suite.add_test(test_assert_true_pass, "test_assert_true_pass");
    suite.add_test(test_assert_true_fail, "test_assert_true_fail");
    suite.add_test(test_assert_false_pass, "test_assert_false_pass");
    suite.add_test(test_assert_false_fail, "test_assert_false_fail");
    suite.add_test(test_assert_equals_pass, "test_assert_equals_pass");
    suite.add_test(test_assert_equals_fail, "test_assert_equals_fail");
    suite.add_test(test_assert_throws_pass, "test_assert_throws_pass");
    suite.add_test(test_assert_throws_fail, "test_assert_throws_fail");
    suite.add_test(test_assert_throws_wrong, "test_assert_throws_wrong");
    suite.add_test(test_count_failures_zero, "test_count_failures_zero");
    suite.add_test(test_count_failures_nonzero, "test_count_failures_nonzero");
    return suite;
}

