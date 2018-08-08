#include "unit.hpp"
#include "console.hpp"


#include "test_unit.hpp"
#include "test_console.hpp"
#include "test_fs.hpp"
#include "test_thread_pool.hpp"

int main(int argc, const char *argv[]) {
    bool verbose {false};
    for (const auto &arg: console::parse_args(argc, argv)) {
        if (arg.key == "--verbose" || arg.key == "-v") {
            verbose = true;
        }
        else {
            std::cerr << console::color::red << "Unhandled argument key: \"" << arg.key << "\", value: \"" << arg.value << "\"" << console::color::reset << std::endl;
        }
    }

    // unit.hpp
    unit::test_suite suite_unit = get_suite_unit();
    suite_unit.execute();
    std::cout << suite_unit.to_string(verbose) << std::endl;

    // console.hpp
    unit::test_suite suite_console = get_suite_console();
    suite_console.execute();
    std::cout << suite_console.to_string(verbose) << std::endl;

    // fs.hpp
    unit::test_suite suite_fs = get_suite_fs();
    suite_fs.execute();
    std::cout << suite_fs.to_string(verbose) << std::endl;

    // thread_pool.hpp
    unit::test_suite suite_thread_pool = get_suite_thread_pool();
    suite_thread_pool.execute();
    std::cout << suite_thread_pool.to_string(verbose) << std::endl;

    return suite_unit.count_failure() + suite_console.count_failure() + suite_fs.count_failure() + suite_thread_pool.count_failure();
}

