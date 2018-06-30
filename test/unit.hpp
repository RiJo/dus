#ifndef __UNIT_HPP_INCLUDED__
#define __UNIT_HPP_INCLUDED__

#include "console.hpp"

#include <vector>
#include <functional>
#include <string>
#include <sstream>

namespace unit {
    enum class test_result {
        PASS,
        FAIL,
        EXCEPTION
    };

    class assertion_error : virtual public std::runtime_error {
        public:
            assertion_error(const std::string &message) : std::runtime_error(message) {}
    };

    class test_report {
        public:
            const std::string description;
            const test_result result;
            const std::string message;

            std::string to_string() const {
                switch (result) {
                    case test_result::PASS:
                        return console::color::green() + "PASS" + console::color::reset();
                    case test_result::FAIL:
                        return console::color::red() + "FAIL" + console::color::reset() + " - " + message;
                    case test_result::EXCEPTION:
                        return console::color::red() + "EXCEPTION" + console::color::reset() + " - " + message;
                    default:
                        throw std::runtime_error("unhandled test report result");
                }
            }
    };

    class test_suite {
        private:
            std::string name;
            std::vector<test_report> reports;

        public:
            test_suite(std::string n): name(n) {}

            void add_test(const std::function<void ()> test, const std::string &description) {
                try {
                    test();
                    reports.emplace_back(test_report{description, test_result::PASS, ""});
                }
                catch (const assertion_error& ex) {
                    reports.emplace_back(test_report{description, test_result::FAIL, ex.what()});
                }
                catch (const std::exception& ex) {
                    reports.emplace_back(test_report{description, test_result::EXCEPTION, ex.what()});
                }
                catch (...) {
                    reports.emplace_back(test_report{description, test_result::EXCEPTION, "unhandled exception"});
                }
            }

            void add_test2(const std::function<std::tuple<bool, std::string> ()> test, const std::string &description) {
                try {
                    auto [pass, message] = test();
                    reports.emplace_back(test_report{description, pass ? test_result::PASS : test_result::FAIL, message});
                }
                catch (const assertion_error& ex) {
                    reports.emplace_back(test_report{description, test_result::FAIL, ex.what()});
                }
                catch (const std::exception& ex) {
                    reports.emplace_back(test_report{description, test_result::EXCEPTION, ex.what()});
                }
                catch (...) {
                    reports.emplace_back(test_report{description, test_result::EXCEPTION, "unhandled exception"});
                }
            }

            std::string to_string() const {
                std::string s {name};

                s += "\n";
                size_t passed = 0;
                for (size_t i = 0; i < reports.size(); i++) {
                    const test_report report = reports.at(i);
                    s += "    #" + std::to_string(i + 1) + " " + report.to_string() + "\n";

                    if (report.result == test_result::PASS)
                        passed++;
                }

                s += "\n";
                if (passed == reports.size())
                    s += "  " + console::color::green() + "All tests passed!" + console::color::reset();
                else
                    s += "  " + console::color::red() + std::to_string(passed) + "/" + std::to_string(reports.size()) + " tests passed.." + console::color::reset();
                s += "\n";

                return s;
            }
    };

    void assert(const std::string &message) {
        throw assertion_error(message);
    }

    void assert_invert(const std::function<void ()> test) {
        try {
            test();
        }
        catch (const assertion_error &ex) {
            return;
        }
        catch (...) {
            assert("unhandled exception");
        }
        assert("no assertion");
    }

    template<typename T>
    void assert_equals(const T expected, const T actual, const std::string &message) {
        if (expected != actual) {
            std::stringstream ss;
            ss << message << " -- expected: [" << expected << "], actual: [" << actual << "]";
            throw assertion_error(ss.str());
        }
    }

    void assert_true(const bool actual, const std::string &message) {
        assert_equals(true, actual, message);
    }

    void assert_false(const bool actual, const std::string &message) {
        assert_equals(false, actual, message);
    }

    void assert_throws(const std::function<void ()> action, const std::string &message) {
        try {
            action();
        }
        catch (...) {
            return;
        }
        assert(message);
    }
}

#endif //__UNIT_HPP_INCLUDED__
