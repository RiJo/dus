#include "unit.hpp"
#include "fs.hpp"

void test_dirname_null() {
    unit::assert_throws(std::runtime_error(""), []() { fs::dirname(nullptr); }, "dirname(nullptr)");
}

void test_dirname_empty_string() {
    std::string actual = fs::dirname("");
    unit::assert_equals("", actual, "dirname(\"\")");
}

void test_dirname_root() {
    std::string actual = fs::dirname("/");
    unit::assert_equals("/", actual, "dirname(\"/\")");
}

void test_dirname_ending_slash() {
    std::string actual = fs::dirname("/foo/bar/");
    unit::assert_equals("/foo/bar", actual, "dirname(\"/foo/bar/\")");
}

void test_dirname_no_ending_slash() {
    std::string actual = fs::dirname("/foo/bar");
    unit::assert_equals("/foo", actual, "dirname(\"/foo/bar\")");
}

void test_basename_null() {
    unit::assert_throws(std::runtime_error(""), []() { fs::basename(nullptr); }, "basename(nullptr)");
}

void test_basename_empty_string() {
    std::string actual = fs::basename("");
    unit::assert_equals("", actual, "basename(\"\")");
}

void test_basename_root() {
    std::string actual = fs::basename("/");
    unit::assert_equals("", actual, "basename(\"/\")");
}

void test_basename_ending_slash() {
    std::string actual = fs::basename("/foo/bar/");
    unit::assert_equals("", actual, "basename(\"/foo/bar/\")");
}

void test_basename_no_ending_slash() {
    std::string actual = fs::basename("/foo/bar");
    unit::assert_equals("bar", actual, "basename(\"/foo/bar\")");
}

unit::test_suite get_suite_fs() {
    unit::test_suite suite("fs.hpp");
    suite.add_test(test_dirname_null, "");
    suite.add_test(test_dirname_empty_string, "");
    suite.add_test(test_dirname_ending_slash, "");
    suite.add_test(test_dirname_no_ending_slash, "");
    suite.add_test(test_basename_null, "");
    suite.add_test(test_basename_empty_string, "");
    suite.add_test(test_basename_ending_slash, "");
    suite.add_test(test_basename_no_ending_slash, "");
    return suite;
}

