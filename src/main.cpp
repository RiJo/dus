#include "dus.hpp"
#include "fs.hpp"
#include "pipes.hpp"
#include "console.hpp"

#include <iomanip>
#include <iostream>
#include <locale>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <future>
#include <math.h>

void print_usage(const std::string &application) {
    std::cout << "usage: " << PROGRAM_NAME << " [-] [-0] [-c <count>] [--color] [-d] [-h] [i] [-n] [-s <size|name|atime|mtime|ctime>] [-t <milliseconds>] [<target file/directory>]" << std::endl;
    std::cout << std::endl;
    std::cout << "List the contents of the given file/directory as graphs based on file sizes. If no target is given the current working directory is used." << std::endl;
    std::cout << std::endl;
    std::cout << "  -           Force read from stdin. Default is reading from stdin only performed if no target is given." << std::endl;
    std::cout << "  -0          Use null character ('\\0') as target separator for stdin. Default is newline ('\\n')." << std::endl;
    std::cout << "  -c <count>  Number of items to printout of result head. Default is infinite (-1)." << std::endl;
    std::cout << "  --color     Colorized output for easier interpretation." << std::endl;
    std::cout << "  -d          Don't enter directory. Only used if a single directory is defined as target." << std::endl;
    std::cout << "  -h          Print human readable sizes (e.g., 1K 234M 5G)." << std::endl;
    std::cout << "  --help      Print this help and exit." << std::endl;
    std::cout << "  -i          Inverted/reverted order of listed result. Default order is set by sort: -s." << std::endl;
    std::cout << "  -n          Enable natural sort order if sort order is a string representation. Default is disabled." << std::endl;
    std::cout << "  -s <...>    Sort by property; 'size', 'name', 'atime', 'mtime', 'ctime'. Default is 'size'." << std::endl;
    std::cout << "  -t <ms>     File/directory parse timeout given in milliseconds. Default is infinite (-1)." << std::endl;
    std::cout << "  --tsep=<c>  Add thousands seperator. Default is none." << std::endl;
    std::cout << "  --version   Print out version information." << std::endl;
    std::cout << std::endl;
    std::cout << "                  Copyright (C) " PROGRAM_YEAR ". Licensed under " PROGRAM_LICENSE "." << std::endl;
}

int cmp_natural_order(const std::string &str1, const std::string &str2) {
    if (str1 == str2)
        return 0;

    for (unsigned int i = 0; i < (str1.length() < str2.length()) ? str1.length() : str2.length(); i++) {
        bool numeric_compare = (str1[i] >= '0' && str1[i] <= '9' && str2[i] >= '0' && str2[i] <= '9');
        if (numeric_compare) {
            std::string str1_numeric = "";
            for (unsigned int j = i; j < str1.length(); j++) {
                if (str1[j] < '0' || str1[j] > '9')
                    break;
                if (str1[j] == '0' && str1_numeric.length() == 0)
                    continue;
                str1_numeric += str1[j];
            }

            std::string str2_numeric = "";
            for (unsigned int j = i; j < str2.length(); j++) {
                if (str2[j] < '0' || str2[j] > '9')
                    break;
                if (str2[j] == '0' && str2_numeric.length() == 0)
                    continue;
                str2_numeric += str2[j];
            }

            if (str1_numeric != str2_numeric) {
                if (str1_numeric.length() > str2_numeric.length())
                    return 1;
                if (str1_numeric.length() < str2_numeric.length())
                    return -1;

                for (unsigned int j = 0; j < str1_numeric.length(); j++) {
                    if (str1_numeric[j] > str2_numeric[j])
                        return 1;
                    if (str1_numeric[j] < str2_numeric[j])
                        return -1;
                }
            }

            i += str1_numeric.length();
            continue;
        }
        else {
            if (str1[i] == str2[i])
                continue;

            if (str1[i] > str2[i])
                return 1;
            else
                return -1;
        }
    }

    if (str1.length() > str2.length())
        return 1;
    else
        return -1;
}

void print_version(const std::string &application) {
    std::cout << PROGRAM_NAME << " v" PROGRAM_VERSION ", built " __DATE__ " " __TIME__ "." << std::endl;
}

template<typename T> constexpr T ce_pow(const T value, const int power) {
    return (power > 1) ? value * ce_pow(value, power - 1) : (power == 1) ? value : (power == 0) ? 0 : throw std::runtime_error("Power cannot be negative: " + std::to_string(power));
}

#if 0
unsigned int strlen_utf8(const std::string &str) {
    unsigned int length = 0;
    for (unsigned int i = 0; i < str.length(); i++) {
        unsigned char c = str[i];
        if (c > 127) {
            if ((c & 0xE0) == 0xC0)
                i += 1; // 2 byte sequence
            else if ((c & 0xF0) == 0xE0)
                i += 2; // 3 byte sequence
            else if ((c & 0xF8) == 0xF0)
                i += 3; // 4 byte sequence
            else if ((c & 0xFC) == 0xF8)
                i += 4; // 5 byte sequence
            else if ((c & 0xFE) == 0xFC)
                i += 5; // 6 byte sequence
            else
                return str.length(); // Not valid UTF-8, probably ISO-8859-1.
        }

        length++;
    }
    return length;
}
#endif

int main(int argc, const char *argv[]) {
    // Parse arguments
    std::set<std::string> targets;
    int count {-1};
    bool enter_directory {true};
    bool order_inverted {false};
    std::string order_by {"size"};
    bool human_readable {false};
    bool natural_order {false};
    int timeout_ms {-1};
    char stdin_separator {'\n'};
    bool colorize {false};
    bool force_read_stdin {false};
    bool skip_next_arg {false};

    struct numpt_t : std::numpunct<char> {
        char tsep {'\0'};
        numpt_t() : std::numpunct<char>(1) {}
        char do_thousands_sep() const { return tsep; }
        std::string do_grouping() const { return "\03"; }
    } numpt;

    for (const auto &arg: console::parse_args(argc, argv)) {
        if (skip_next_arg) {
            skip_next_arg = false;
            continue;
        }

        if (arg.key == "--help") {
            print_usage(fs::basename(std::string(argv[0])));
            return 0;
        }
        else if (arg.key == "--version" || arg.key == "-v") {
            print_version(fs::basename(std::string(argv[0])));
            return 0;
        }
        else if (arg.key == "-") {
            force_read_stdin = true;
        }
        else if (arg.key == "-0") {
            stdin_separator = '\0';
        }
        else if (arg.key == "-c" && arg.next) {
            count = std::stoi(arg.next->key);
            skip_next_arg = true;
        }
        else if (arg.key == "--color") {
            colorize = true;
        }
        else if (arg.key == "-d") {
            enter_directory = false;
        }
        else if (arg.key == "-h") {
            human_readable = true;
        }
        else if (arg.key == "-i") {
            order_inverted = !order_inverted;
        }
        else if (arg.key == "-n") {
            natural_order = true;
        }
        else if (arg.key == "-s" && arg.next) {
            order_by = std::string(arg.next->key);
            skip_next_arg = true;
        }
        else if (arg.key == "-t" && arg.next) {
            timeout_ms = std::stoi(arg.next->key);
            skip_next_arg = true;
        }
        else if (arg.key == "--tsep") {
            numpt.tsep = arg.value[0];
        }
        else if (arg.value.length() == 0) {
            std::string potential_target = fs::absolute_path(arg.key);
            if (fs::exists(potential_target))
                targets.insert(std::move(potential_target));
            else
                std::cerr << console::color::red << PROGRAM_NAME << ": Unhandled argument flag: \"" << arg.key << "\"" << console::color::reset << std::endl;
        }
        else {
            std::cerr << console::color::red << PROGRAM_NAME << ": Unhandled argument key: \"" << arg.key << "\", value: \"" << arg.value << "\"" << console::color::reset << std::endl;
        }
    }

    // Set console properties
    console::color::enable = colorize;

    // Read stdin as primary default target
    if (targets.size() == 0 || force_read_stdin) {
        for (auto const &target: pipes::read_stdin(stdin_separator, -1)) {
            if (target.length() > 0)
                targets.insert(fs::absolute_path(target));
        }
    }

    // Use current working directory as secondary default target
    if (targets.size() == 0) {
        targets.insert(fs::current_working_directory());
    }

    // Read file/directory contents asynchronously (and render loading progress indicator)
    enter_directory &= targets.size() == 1; // Only enter directory if it's the only target
    std::future<std::vector<fs::file_info>> future = std::async(std::launch::async, [targets, enter_directory] {
        std::vector<fs::file_info> result;
        for (auto const &target: targets) {
            if (fs::is_type<fs::file_type::directory>(target))
                for (auto const &file:fs::read_directory(target, enter_directory, true))
                    result.push_back(std::move(file));
            else
                result.push_back(fs::read_file(target));
        }
        return result;
    });

    // Wait for file system result
    if (timeout_ms > 0) {
        std::future_status status = future.wait_for(std::chrono::milliseconds(timeout_ms));
        if (status != std::future_status::ready) {
            std::cerr << console::color::red << PROGRAM_NAME << ": Timeout after " << timeout_ms << "ms" << console::color::reset << std::endl;
            exit(3); // TODO: properly terminate std::future<>, we'd like to call 'return X' here.
        }
    }
    else {
        future.wait();
    }
    std::vector<fs::file_info> files { future.get() };

    // Sort contents
    // TODO: use keys in usage printout as available values of '-s'
    std::map<std::string, std::function<bool (const fs::file_info &, const fs::file_info &)>> comparators;
    comparators["size"] = [order_inverted] (const fs::file_info &first, const fs::file_info &second) { return order_inverted ? first.length < second.length : first.length > second.length; };
    comparators["atime"] = [order_inverted] (const fs::file_info &first, const fs::file_info &second) { return order_inverted ? first.access_time < second.access_time : first.access_time > second.access_time; };
    comparators["mtime"] = [order_inverted] (const fs::file_info &first, const fs::file_info &second) { return order_inverted ? first.modify_time < second.modify_time : first.modify_time > second.modify_time; };
    comparators["ctime"] = [order_inverted] (const fs::file_info &first, const fs::file_info &second) { return order_inverted ? first.change_time < second.change_time : first.change_time > second.change_time; };
    comparators["name"] = [order_inverted, natural_order] (const fs::file_info &first, const fs::file_info &second) {
        if (natural_order)
            return order_inverted ? cmp_natural_order(first.name, second.name) > 0 : cmp_natural_order(first.name, second.name) < 0;
        return order_inverted ? first.name > second.name : first.name < second.name;
    };
    if (!comparators.count(order_by)) {
        std::cerr << console::color::red << PROGRAM_NAME << ": Undefined sort type: \"" << order_by << "\"" << console::color::reset << std::endl; // TODO: add valid ones to message
        return 2;
    }
    std::sort(files.begin(), files.end(), [&] (const fs::file_info &a, const fs::file_info &b) { return comparators[order_by](a, b); });

    // Find highest value (used for percentage)
    unsigned int total_length {0};
    for (const auto &file: files)
        total_length += file.length;

    // Strip exceeding items
    if (count >= 0 && static_cast<unsigned int>(count) < files.size())
        files.erase(files.begin() + count, files.end());

    // Determine tty width
    int columns;
    {
        console::tty temp;
        columns = temp.cols;
    }

    std::locale locale;
    if (numpt.tsep != '\0')
        locale = std::locale(std::locale(), &numpt);
    else
        locale = std::locale("C");

    // Find out the maximum width for filenames, maximum size for files
    unsigned int name_width {0};
    unsigned int size_width {0};
    for (auto const &file: files) {
        if (file.type == fs::file_type::directory)
            name_width = std::max(name_width, console::text_width(file.name) + 1); // Directories are suffixed with '/'
        else
            name_width = std::max(name_width, console::text_width(file.name));

        std::stringstream temp;
        temp.imbue(locale);
        if (human_readable && file.length >= 1024) {
            if (file.length >= ce_pow(1024, 3))
                temp << file.length / ce_pow(1024, 3);
            else if (file.length >= ce_pow(1024, 2))
                temp << file.length / ce_pow(1024, 2);
            else if (file.length >= 1024)
                temp << file.length / 1024;
        }
        else
            temp << file.length;
        size_width = std::max(size_width, (unsigned int) temp.str().length());
    }

    const unsigned int max_name_width {35};
    name_width = std::min(max_name_width, name_width);
    if (human_readable)
        size_width++;  // 1 for unit or space

    // Dump result
    const int chars_left = columns - (1 + name_width + 1 + size_width + 1);
    for (auto const &file: files) {
        std::string row_data {""};

        // Prefix
        if (!file.authorized)
            row_data += console::color::red() + "!" + console::color::reset();
        else if (file.type == fs::file_type::directory)
            row_data += "*";
        else
            row_data += " ";

        // Filename
        unsigned int file_name_length = console::text_width(file.name);
        if (file_name_length > name_width)
            row_data += file.name.substr(0, name_width - 2) + "..";
        else if (file.type == fs::file_type::directory)
            row_data += file.name + '/' + std::string(name_width - file_name_length - 1, ' ');
        else
            row_data += file.name + std::string(name_width - file_name_length, ' ');
        row_data += " ";

        // File size
        std::stringstream temp;
        temp.imbue(locale);
        if (human_readable) {
            temp << std::setw(size_width - 1);  // only the number part
            if (file.length >= ce_pow(1024, 3))
                temp << file.length / ce_pow(1024, 3) << "G";
            else if (file.length >= ce_pow(1024, 2))
                temp << file.length / ce_pow(1024, 2) << "M";
            else if (file.length >= 1024)
                temp << file.length / 1024 << "K";
            else
                temp << file.length << " ";
        }
        else {
            temp << std::setw(size_width) << file.length;
        }
        row_data += temp.str();
        row_data += " ";

        double factor = (total_length > 0) ? (static_cast<double>(file.length) / static_cast<double>(total_length)) : 0.0;
        double percent = factor * 100.0;

        // Progress bar
        int progress_width = chars_left - 4 /* last 4 chars for "xxx%" */;
        int bar_width = (progress_width - 3) * factor;
        row_data += "[";
        if (percent >= 50)
            row_data += console::color::red();
        else if (percent >= 25)
            row_data += console::color::yellow();
        else
            row_data += console::color::green();
        row_data += std::string(bar_width, '=');
        row_data += "|";
        row_data += console::color::reset();
        row_data += std::string(progress_width - bar_width - 3, ' ');
        row_data += "]";

        // Percentage
        if (static_cast<int>(percent) < 10)
            row_data += "  ";
        else if (static_cast<int>(percent) < 100)
            row_data += " ";
        row_data += std::to_string(static_cast<int>(percent)) + "%";

        if (file.authorized)
            std::cout << row_data << std::endl;
        else
            std::cerr << console::color::red << row_data << console::color::reset << std::endl;
    }

    return 0;
}
