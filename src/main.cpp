#include "dus.hpp"
#include "fs.hpp"
#include "pipes.hpp"
#include "console.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <future>
#include <math.h>
#include <chrono>

void print_usage(const std::string &application) {
    std::cout << "usage: " << PROGRAM_NAME << " [-c <count>] [-h] [i] [-n] [-s <size|name>] [-t <milliseconds>] [<target file/directory>]" << std::endl;
    std::cout << std::endl;
    std::cout << "List the contents of the given file/directory as graphs based on file sizes. If no target is given the current working directory is used." << std::endl;
    std::cout << std::endl;
    std::cout << "  -0          Use null character ('\\0') as target separator for stdin. Default is newline ('\\n')." << std::endl;
    std::cout << "  -c <count>  Number of items to printout of result head. Default is infinite (-1)." << std::endl;
    std::cout << "  -d          Don't enter directory. Only used if a single directory is defined as target." << std::endl;
    std::cout << "  -h          Print human readable sizes (e.g., 1K 234M 5G)." << std::endl;
    std::cout << "  -i          Inverted/reverted order of listed result. Default order is set by sort: -s." << std::endl;
    std::cout << "  -n          Enable natural sort order if sort order is a string representation. Default is disabled." << std::endl;
    std::cout << "  -s <...>    Sort by property; 'size', 'name'. Default is 'size'." << std::endl;
    std::cout << "  -t <ms>     File/directory parse timeout given in milliseconds. Default is infinite (-1)." << std::endl;
    std::cout << "  --help      Print this help and exit." << std::endl;
    std::cout << "  --version   Print out version information." << std::endl;
    std::cout << std::endl;
    std::cout << "                  by Rikard Johansson, 2015. Licensed under " PROGRAM_LICENSE "." << std::endl;
}

int cmp_natural_order(const std::string &str1, const std::string &str2) {
    if (str1 == str2)
        return 0;

    for (int i = 0; i < str1.length() < str2.length() ? str1.length() : str2.length(); i++) {
        bool numeric_compare = (str1[i] >= '0' && str1[i] <= '9' && str2[i] >= '0' && str2[i] <= '9');
        if (numeric_compare) {
            std::string str1_numeric = "";
            for (int j = i; j < str1.length(); j++) {
                if (str1[j] < '0' || str1[j] > '9')
                    break;
                if (str1[j] == '0' && str1_numeric.length() == 0)
                    continue;
                str1_numeric += str1[j];
            }

            std::string str2_numeric = "";
            for (int j = i; j < str2.length(); j++) {
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

                for (int j = 0; j < str1_numeric.length(); j++) {
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

int strlen_utf8(const std::string &str)
{
    int length = 0;
    for (int i = 0; i < str.length(); i++) {
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
    for (int i = 1; i < argc; i++) {
        std::string arg = std::string(argv[i]);
        if (arg == "--help") {
            print_usage(fs::basename(std::string(argv[0])));
            return 0;
        }
        else if (arg == "--version" || arg == "-v") {
            print_version(fs::basename(std::string(argv[0])));
            return 0;
        }
        else if (arg == "-0") {
            stdin_separator = '\0';
        }
        else if (arg == "-c" && i < argc) {
            count = std::atoi(argv[i + 1]);
            i++;
        }
        else if (arg == "-d") {
            enter_directory = false;
        }
        else if (arg == "-h") {
            human_readable = true;
        }
        else if (arg == "-i") {
            order_inverted = !order_inverted;
        }
        else if (arg == "-n") {
            natural_order = true;
        }
        else if (arg == "-s" && i < argc) {
            order_by = std::string(argv[i + 1]);
            i++;
        }
        else if (arg == "-t" && i < argc) {
            timeout_ms = std::atoi(argv[i + 1]);
            i++;
        }
        else {
            std::string potential_target = fs::absolute_path(arg);
            if (fs::exists(potential_target))
                targets.insert(std::move(potential_target));
            else
                std::cerr << "Unhandled argument: \"" << arg << "\"" << std::endl;
        }
    }

    // Check stdin for targets
    for (auto const &target: pipes::read_stdin(stdin_separator, 10)) {
        if (target.length() > 0)
            targets.insert(std::move(fs::absolute_path(target)));
    }

    // Use current working directory as default target
    if (targets.size() == 0)
        targets.insert(std::move(fs::current_working_directory()));

    // Read file/directory contents asynchronously (and render loading progress indicator)
    enter_directory &= targets.size() == 1; // Only enter directory if it's the only target
    std::future<std::vector<fs::file_info>> future = std::async(std::launch::async, [targets, enter_directory] {
        std::vector<fs::file_info> result;
        for (auto const &target: targets) {
            if (fs::is_type<fs::file_type::directory>(target))
                for (auto const &file:fs::read_directory(target, enter_directory, true))
                    result.push_back(std::move(file));
            else
                result.push_back(std::move(fs::read_file(target)));
        }
        return result;
    });
    bool timeout = false;
    int rows, cols;
    {
        console render;
        cols = render.cols;
        rows = render.rows;

        std::future_status status;
        const std::string spinner = R"(-\|/)";
        const int spinner_chars = spinner.length();
        int spin_char = 0;
        char chr {spinner[spin_char]};
        auto start_time = std::chrono::system_clock::now();
        do {
            status = future.wait_for(std::chrono::milliseconds(50));
            if (status == std::future_status::ready)
                break;

            // Render spinner
            chr = spinner[spin_char];
            spin_char = (spin_char + 1) % spinner_chars;
            render.write(0, 0, chr);

            // Check timeout
            if (timeout_ms > 0 && std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::system_clock::now() - start_time).count() > timeout_ms) {
                timeout = true;
                break;
            }
        } while (true /*status != std::future_status::ready*/);
    }

    if (timeout) {
        std::cerr << "Timeout after " << timeout_ms << "ms" << std::endl;
        exit(3); // TODO: properly terminate std::future<>, we'd like to call 'return X' here.
    }

    std::vector<fs::file_info> files { future.get() };

    // Sort contents
    // TODO: use keys in usage printout as available values of '-s'
    std::map<std::string, std::function<bool (const fs::file_info &, const fs::file_info &)>> comparators;
    comparators["size"] = [order_inverted] (const fs::file_info &first, const fs::file_info &second) { return order_inverted ? first.length < second.length : first.length > second.length; };
    comparators["name"] = [order_inverted, natural_order] (const fs::file_info &first, const fs::file_info &second) {
        if (natural_order)
            return order_inverted ? cmp_natural_order(first.name, second.name) > 0 : cmp_natural_order(first.name, second.name) < 0;
        return order_inverted ? first.name > second.name : first.name < second.name;
    };
    if (!comparators.count(order_by)) {
        std::cerr << "Undefined sort type: \"" << order_by << "\"" << std::endl;
        return 2;
    }
    std::sort(files.begin(), files.end(), comparators[order_by]);

    // Find highest value (used for percentage)
    double total_length {0.0};
    for (const auto &file: files)
        total_length += file.length;

    // Dump result
    for (auto const &file: files) {
        if (count == 0)
            break;

        std::string row_data {""};

        // Prefix
        if (!file.authorized)
            row_data += "!";
        else if (file.type == fs::file_type::directory)
            row_data += "*";
        else
            row_data += " ";

        // Filename
        const int name_width {35};
        int file_name_length = strlen_utf8(file.name);
        if (file_name_length >= name_width - 2)
            row_data += file.name.substr(0, name_width - 2) + "..";
        else if (file.type == fs::file_type::directory)
            row_data += file.name + '/' + std::string(name_width - file_name_length - 1, ' ');
        else
            row_data += file.name + std::string(name_width - file_name_length, ' ');
        row_data += " ";

        // File size
        const int size_width {10};
        std::string temp;
        if (human_readable && file.length >= 1024) {
            if (file.length >= ce_pow(1024, 3))
                temp = std::to_string(file.length / ce_pow(1024, 3)) + "G";
            else if (file.length >= ce_pow(1024, 2))
                temp = std::to_string(file.length / ce_pow(1024, 2)) + "M";
            else
                temp = std::to_string(file.length / 1024) + "K";
        }
        else {
            temp = std::to_string(file.length);
        }
        row_data += temp + std::string(size_width - temp.length(), ' ');
        row_data += " ";

        // Progress bar
        int chars_left = cols - strlen_utf8(row_data);
        int progress_width = chars_left - 4 /* last 4 chars for "xxx%" */;
        int bar_width = (progress_width - 3) * (file.length / total_length);
        row_data += "[";
        row_data += std::string(bar_width, '=');
        row_data += "|";
        row_data += std::string(progress_width - bar_width - 3, ' ');
        row_data += "]";

        // Percentage
        double percent = (file.length / total_length) * 100.0;
        //percent = ((int)(percent * 10)) / 10.0; // Round to one decimal
        if ((int)percent < 10)
            row_data += "  ";
        else if ((int)percent < 100)
            row_data += " ";
        row_data += std::to_string((int)percent) + "%";

        if (file.authorized)
            std::cout << row_data << std::endl;
        else
            std::cerr << row_data << std::endl;

        if (count > 0)
            count--;
    }

    return 0;
}
