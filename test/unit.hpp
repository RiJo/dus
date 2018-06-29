#ifndef __UNIT_HPP_INCLUDED__
#define __UNIT_HPP_INCLUDED__

#include "console.hpp"

#include <vector>
#include <functional>
#include <string>

namespace unit {
    enum class test_result {
        PASS,
        FAIL,
        EXCEPTION
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

            void add_test(std::function<std::tuple<bool, std::string> ()> test, std::string description) {
                try {
                    auto [pass, message] = test();
                    reports.emplace_back(test_report{description, pass ? test_result::PASS : test_result::FAIL,message});
                }
                catch (const std::exception& ex) {
                    reports.emplace_back(test_report{description, test_result::EXCEPTION, ex.what()});
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

                return s;
            }
    };
}

#endif //__UNIT_HPP_INCLUDED__
