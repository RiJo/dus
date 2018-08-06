#include "unit.hpp"
#include "console.hpp"

#include <string.h> // strncpy()

void verify_parse_args(std::vector<std::string> args, std::function<void (const std::vector<console::arg_t>)> verifier) {
    char** c_args = new char*[args.size() + 1];
    for (unsigned int i = 0; i < args.size(); i++) {
        char *ptr = (char *)malloc(args.at(i).size() + 1);
        if (ptr == nullptr)
            throw std::runtime_error("failed to allocate memory");
        c_args[i] = ptr;
        strncpy(c_args[i], args.at(i).c_str(), args.at(i).size());
        c_args[i][args.at(i).size()] = '\0';
    }
    c_args[args.size()] = nullptr;

    const std::vector<console::arg_t> parsed = console::parse_args(args.size(), (const char**)c_args);
    verifier(parsed);

    for (unsigned int i = 0; i < args.size(); i++)
        free(c_args[i]);
    free(c_args);
}

bool compare_args(const console::arg_t &x, const console::arg_t &y) {
    return x.key == y.key && x.value == y.value && x.next == y.next;
}

void test_parse_args_none() {
    verify_parse_args(std::vector<std::string>{ }, [](const std::vector<console::arg_t> parsed) {
        unit::assert_container<console::arg_t>(std::initializer_list<console::arg_t>{}, parsed, "parsed arguments", compare_args);
    });
}

void test_parse_args_short_single_flag() {
    verify_parse_args({ "/tmp/test_console", "-x" }, [](const std::vector<console::arg_t> parsed) {
        unit::assert_container<console::arg_t>(std::initializer_list<console::arg_t>{ console::arg_t{"-x", "", nullptr} }, parsed, "parsed arguments", compare_args);
    });
}

void test_parse_args_short_multiple_flags() {
    verify_parse_args({ "/tmp/test_console", "-x", "-yz" }, [](const std::vector<console::arg_t> parsed) {
        unit::assert_container<console::arg_t>(std::initializer_list<console::arg_t>{
                console::arg_t{"-x", "", nullptr},
                console::arg_t{"-y", "", nullptr},
                console::arg_t{"-z", "", nullptr}
        }, parsed, "parsed arguments", compare_args);
    });
}

void test_parse_args_dash() {
    verify_parse_args({ "/tmp/test_console", "--x", "-", "-y" }, [](const std::vector<console::arg_t> parsed) {
        unit::assert_container<console::arg_t>(std::initializer_list<console::arg_t>{
                console::arg_t{"--x", "", nullptr},
                console::arg_t{"-", "", nullptr},
                console::arg_t{"-y", "", nullptr}
        }, parsed, "parsed arguments", compare_args);
    });
}

void test_parse_args_long_variable() {
    verify_parse_args({ "/tmp/test_console", "--foo=bar" }, [](const std::vector<console::arg_t> parsed) {
        unit::assert_container<console::arg_t>(std::initializer_list<console::arg_t>{ console::arg_t{"--foo", "bar", nullptr} }, parsed, "parsed arguments", compare_args);
    });
}

unit::test_suite get_suite_console() {
    unit::test_suite suite("console.hpp");
    suite.add_test(test_parse_args_none, "test_parse_args_none");
    suite.add_test(test_parse_args_short_single_flag, "test_parse_args_short_single_flag");
    suite.add_test(test_parse_args_short_multiple_flags, "test_parse_args_short_multiple_flags");
    suite.add_test(test_parse_args_dash, "test_parse_args_dash");
    suite.add_test(test_parse_args_long_variable, "test_parse_args_long_variable");
    return suite;
}

