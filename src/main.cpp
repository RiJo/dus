#include "dus.h"
#include "fs.cpp"
#include "console.cpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <future>
#include <math.h>

/*
    TODO:
        - Add ctrl-c listener for termination
        - Howto summarize if permission is denied to some paths?
        - -t to define on how many levels threads should be forked for each directory
        - Independent order of arguments: flags vs. target directory (currently required last)
        - Add support for cin: read target directory/directories from stdin
*/

void usage(const char *application) {
    std::cout << "usage: " << application << " [-c <count>] [-h] [-s <size|name>] [<target directory>]" << std::endl;
    std::cout << std::endl;
    std::cout << "List the contents of the current/given directory as graphs based on file sizes. If no target directory is given the current working directory is used." << std::endl;
    std::cout << std::endl;
    std::cout << "  -c       Number of items to printout. Default is infinite (-1)." << std::endl;
    std::cout << "  -h       Print human readable sizes (e.g., 1K 234M 2G)." << std::endl;
    std::cout << "  -i       Inverted/reverted order of listed result. Default order is set by sort: -s." << std::endl;
    std::cout << "  -s       Sort by property; 'size', 'name'. Default is 'size'." << std::endl;
    std::cout << "  --help   Print this help and exit." << std::endl;
    std::cout << std::endl;
    std::cout << "                  by Rikard Johansson, 2015. Licensed under GPLv3." << std::endl;
}

template<typename T> constexpr T ce_pow(const T value, const int power) {
    return (power > 1) ? value * ce_pow(value, power - 1) : (power == 1) ? value : (power == 0) ? 0 : throw std::runtime_error("Power cannot be negative: " + std::to_string(power));
}

int main(int argc, const char *argv[]) {
    // Parse arguments
    std::string target;
    int count {-1};
    bool order_inverted {false};
    std::string order_by {"size"};
    bool human_readable {false};
    for (int i = 1; i < argc; i++) {
        std::string arg = std::string(argv[i]);
        if (arg == "--help") {
            usage(argv[0]);
            return 0;
        }
        else if (arg == "-h") {
            human_readable = true;
        }
        else if (arg == "-i") {
            order_inverted = !order_inverted;
        }
        else if (arg == "-c" && i < argc) {
            count = std::atoi(argv[i + 1]);
            i++;
        }
        else if (arg == "-s" && i < argc) {
            order_by = std::string(argv[i + 1]);
            i++;
        }
        else if (i == argc - 1 && argv[argc - 1][0] != '-') {
            target = fs::absolute_path(argv[argc - 1]);
        }
        else {
            std::cerr << "Unhandled argument: \"" << arg << "\"" << std::endl;
        }
    }

    // Parse target
    if (target.length() == 0)
        target = fs::current_working_directory();
    if (!fs::is_type<fs::file_type::directory>(target)) {
        std::cerr << "Directory not found: " << target << std::endl;
        return 1;
    }

    // Read directory contents asynchronously (and render loading progress indicator)
    std::future<std::vector<fs::file_info>> future = std::async(std::launch::async, [target] { return fs::read_directory(target, true); });
    int rows, cols;
    {
        console render;
        cols = render.cols;
        rows = render.rows;

        std::future_status status;
        char chr {'-'};
        do {
            // TODO: implement timeout handling (-t to define?)
            status = future.wait_for(std::chrono::milliseconds(50));
            if (status == std::future_status::ready)
                break;

            switch (chr) {
                case '-':
                    chr = '\\';
                    break;
                case '\\':
                    chr = '|';
                    break;
                case '|':
                    chr = '/';
                    break;
                case '/':
                    chr = '-';
                    break;
                default:
                    chr = '?';
                    break;
            }
            render.write(0, 0, chr);
        } while (true /*status != std::future_status::ready*/);
    }
    std::vector<fs::file_info> files { future.get() };

    // Sort contents
    // TODO: use keys in usage printout as available values of '-s'
    // TODO: implement natural sort order for strings. Optional or hardcoded?
    std::map<std::string, std::function<bool (const fs::file_info &, const fs::file_info &)>> comparators;
    comparators["size"] = [order_inverted] (const fs::file_info &first, const fs::file_info &second) { return order_inverted ? first.length < second.length : first.length > second.length; };
    comparators["name"] = [order_inverted] (const fs::file_info &first, const fs::file_info &second) { return order_inverted ? first.name > second.name : first.name < second.name; };
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
    for (auto file: files) {
        if (count == 0)
            break;

        if (!file.authorized) {
            std::cerr << "!" << file.name << std::endl;
            continue;
        }

        std::string row_data {""};

        // Prefix
        if (file.type == fs::file_type::directory)
            row_data += "*";
        else
            row_data += " ";

        // Filename
        //~ std::wstring foo(file.name.begin(), file.name.end());
        //~ std::wcout << "wstring: \"" << foo << "\", length: " << foo.length() << ", std::wcslen: " << std::wcslen(foo) << std::endl;
        // TODO: doesn't calculate correct width if åäöÅÄÖ is used in filename: use wstring instead of string?
        const int name_width {35};
        if (file.name.length() >= name_width - 2)
            row_data += file.name.substr(0, name_width - 2) + "..";
        else
            row_data += file.name + std::string(name_width - file.name.length(), ' ');
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
        int chars_left = cols - row_data.length();
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

        std::cout << row_data << std::endl;

        if (count > 0)
            count--;
    }

    return 0;
}
