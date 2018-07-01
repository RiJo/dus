#include "unit.hpp"
#include "test_unit.hpp"
#include "test_thread_pool.hpp"

int main(int, const char **) {
    // unit.hpp
    unit::test_suite suite_unit = get_suite_unit();
    std::cout << suite_unit.to_string() << std::endl;

    // thread_pool.hpp
    unit::test_suite suite_thread_pool = get_suite_thread_pool();
    std::cout << suite_thread_pool.to_string() << std::endl;

    return suite_unit.count_failure() + suite_thread_pool.count_failure();
}

