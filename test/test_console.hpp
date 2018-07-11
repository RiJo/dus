#include "unit.hpp"
#include "console.hpp"

#include <string.h> // strncpy()

void verify_parse_args(std::vector<std::string> args, std::function<void (const std::vector<console::arg_t>)> verifier) {
    std::cout << " args: " << args.size() << std::endl;
    char** c_args = new char*[args.size() + 1];
    for (unsigned int i = 0; i < args.size(); i++) {
        c_args[i] = (char*)malloc(args.at(i).size() + 1);
        strncpy(c_args[i], args.at(i).c_str(), args.at(i).size());
    }
    c_args[args.size() + 1] = nullptr;

    const std::vector<console::arg_t> parsed = console::parse_args(args.size(), (const char**)c_args);
    std::cout << " parsed: " << parsed.size() << std::endl;
    verifier(parsed);
}

void test_parse_args_none() {
    verify_parse_args(std::vector<std::string>{ }, [](const std::vector<console::arg_t> parsed) {
        unit::assert_equals((size_t)0, parsed.size(), "number of parsed arguments");
    });
}

void test_parse_args_single_flag() {
    verify_parse_args({ "/tmp/test_console", "-x" }, [](const std::vector<console::arg_t> parsed) {
        unit::assert_equals((size_t)1, parsed.size(), "number of parsed arguments");
        unit::assert_equals("-x", parsed.at(0).key, "key");
        unit::assert_equals("", parsed.at(0).value, "value");
        /* unit::assert_equals(0, parsed.at(0).next, "next"); */
    });
}

void test_parse_args_long_variable() {
    verify_parse_args({ "/tmp/test_console", "--foo=bar" }, [](const std::vector<console::arg_t> parsed) {
        unit::assert_equals((size_t)1, parsed.size(), "number of parsed arguments");
        unit::assert_equals("--foo", parsed.at(0).key, "key");
        unit::assert_equals("bar", parsed.at(0).value, "value");
        /* unit::assert_equals(0, parsed.at(0).next, "next"); */
    });
}

unit::test_suite get_suite_console() {
    unit::test_suite suite("console.hpp");
    suite.add_test(test_parse_args_none, "test_parse_args_none");
    suite.add_test(test_parse_args_single_flag, "test_parse_args_single_flag");
    suite.add_test(test_parse_args_long_variable, "test_parse_args_long_variable");
    return suite;
}

