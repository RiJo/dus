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

    class assertion_error : public std::string {
        public:
            assertion_error(const std::string &message) : std::string(message) {}
    };

    class test_report {
        public:
            const std::string description;
            const test_result result;
            const std::string message;

            std::string to_string(const bool verbose) const {
                switch (result) {
                    case test_result::PASS:
                        if (verbose)
                            return console::color::green() + "PASS" + console::color::reset() + (description.size() > 0 ? " -- " + description : "");
                        break;
                    case test_result::FAIL:
                        return console::color::red() + "FAIL" + console::color::reset() + (description.size() > 0 ? " -- " + description : "") +  (message.size() > 0 ? " -- " + message : "");
                    case test_result::EXCEPTION:
                        return console::color::red() + "EXCEPTION" + console::color::reset() + (description.size() > 0 ? " -- " + description : "") +  (message.size() > 0 ? " -- " + message : "");
                    default:
                        throw std::runtime_error("unhandled test report result");
                }
                return "";
            }
    };

    class test_suite {
        private:
            std::string name;
            std::vector<std::tuple<std::string, std::function<void ()>>> test_cases;
            std::vector<test_report> reports;

            void run_test(const std::string &description, const std::function<void ()> test_case) {
                try {
                    test_case();
                    reports.emplace_back(test_report{description, test_result::PASS, ""});
                }
                catch (const assertion_error& ex) {
                    reports.emplace_back(test_report{description, test_result::FAIL, ex});
                }
                catch (const std::exception& ex) {
                    reports.emplace_back(test_report{description, test_result::EXCEPTION, ex.what()});
                }
                catch (...) {
                    reports.emplace_back(test_report{description, test_result::EXCEPTION, "unhandled exception"});
                }
            }

        public:
            test_suite(std::string n): name(n) {}

            void add_test(const std::function<void ()> test_case, const std::string &description = "") {
                test_cases.push_back(std::make_pair(description, test_case));
            }

            void execute() {
                for (const auto &test_case: test_cases)
                    run_test(std::get<0>(test_case), std::get<1>(test_case));
            }

            void operator() (void) {
                execute();
            }

            size_t count_failure() {
                size_t count{0};
                for (const auto &report: reports)
                    if (report.result != test_result::PASS)
                        count++;
                return count;
            }

            std::string to_string(const bool verbose) const {
                std::stringstream ss {""};

                ss << std::string(80, '=') << std::endl;
                ss << " " + name << std::endl;
                ss << std::string(80, '-') << std::endl;

                bool any_report_printed {false};
                if (reports.size() > 0) {
                    size_t passed = 0;
                    for (size_t i = 0; i < reports.size(); i++) {
                        const test_report report = reports.at(i);
                        if (report.result == test_result::PASS)
                            passed++;

                        const std::string test_report_string = report.to_string(verbose);
                        if (test_report_string.length() == 0)
                            continue;
                        if (!any_report_printed) {
                            any_report_printed = true;
                            ss << std::endl;
                        }
                        ss << "    #" << (i + 1) << " " << report.to_string(verbose) << std::endl;
                    }

                    ss << std::endl;
                    if (passed == reports.size())
                        ss << "  " << console::color::green << "All tests passed!" << console::color::reset;
                    else
                        ss << "  " << console::color::red << passed << "/" << reports.size() << " tests passed.." << console::color::reset;
                }
                else {
                    ss << "  " << console::color::blue << "No tests in suite" << console::color::reset << " :(";
                }
                ss << std::endl;

                return ss.str();
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

    template<typename X, typename Y>
    void assert_equals(const X expected, const Y actual, const std::string &message) {
        if (expected != actual) {
            std::stringstream ss;
            ss << message << " -- expected: [" << expected << "], actual: [" << actual << "]";
            assert(ss.str());
        }
    }

    void assert_true(const bool actual, const std::string &message) {
        assert_equals(true, actual, message);
    }

    void assert_false(const bool actual, const std::string &message) {
        assert_equals(false, actual, message);
    }

    template<typename T>
    void assert_throws(const T &expected, const std::function<void ()> action, const std::string &message) {
        std::stringstream ss;
        try {
            action();
        }
        catch (decltype(expected) e) {
            return;
        }
        catch (...) {
            ss << message << " -- expected: [" << typeid(expected).name() << "], actual: [" << typeid(std::current_exception()).name() << "]";
            assert(ss.str());
        }

        ss << message << " -- expected: [" << typeid(expected).name() << "], actual: <none>";
        assert(ss.str());
    }

    template<typename T, typename E, typename A>
    void assert_container(const E &expected, const A &actual, const std::string &message,
            const std::function<bool (const T&, const T&)> comparator = [](const T &e, const T &a) -> bool { return e == a; }) {
        for (const T &a: expected) {
            bool a_found = false;
            for (const T &b: actual) {
                a_found |= comparator(a, b);

                bool b_found = false;
                for (const T &c: expected)
                    b_found |= comparator(c, b);
                assert_true(b_found, "actual element not in expected collection -- " + message); // TODO: add missing value to message
            }
            assert_true(a_found, "expected element not in actual collection -- " + message); // TODO: add missing value to message
        }
    }
}

#endif //__UNIT_HPP_INCLUDED__
