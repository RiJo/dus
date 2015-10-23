#ifndef __FS_HPP_INCLUDED__
#define __FS_HPP_INCLUDED__

#include <string>
#include <vector>
#include <stdexcept>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h> // getcwd()

namespace fs {
    enum class file_type {
        block_device,
        character_device,
        directory,
        fifo,
        symlink,
        file,
        socket,
        unknown
    };

    struct file_info {
        bool authorized;
        bool exists;
        fs::file_type type;
        std::string path;
        std::string name;
        unsigned int length;

        //~ std::ostream& operator<<(std::ostream& os, const struct file_info fi) {
            //~ return os << fi.path << '/' << fi.name << '\n';
        //~ }
    };

    enum permission_flag {
        read = 0x04,
        write = 0x02,
        execute = 0x01,
    };

    std::string dirname(const std::string &path) {
        std::size_t found { path.find_last_of("/\\") };
        if (found == std::string::npos)
            return path;

        return path.substr(0, found);
    }

    std::string basename(const std::string &path) {
        std::size_t found { path.find_last_of("/\\") };
        if (found == std::string::npos)
            return path;

        return path.substr(found + 1);
    }

    std::string current_working_directory() {
        char buffer[255];
        const char *temp { getcwd(buffer, 255) };
        if (temp == nullptr)
            throw std::runtime_error("Unable to retrieve current working directory.");
        return std::string(temp);
    }

    std::string absolute_path(const std::string &path) {
        if (path.length() == 0)
            return "";
        if (path[0] == '/')
            return path; // Already absolute
        if (path[0] != '.')
            return current_working_directory() + '/' + path; // Assume current working directory
        if (path == ".")
            return current_working_directory(); // Current working directory
        if (path[1] == '.')
            return dirname(current_working_directory()) + path.substr(2); // Parent directory: replace with current working directory

        return current_working_directory() + '/' + path;
    }

    bool is_authorized(fs::file_type type, int mode, int uid, int gid, fs::permission_flag evaluation) {
        if (mode & evaluation)
            return true;

        bool gid_match = (gid == getgid());
        if (gid_match)
            if ((mode >> 8) & evaluation)
                return true;

        bool uid_match = (uid == getuid());
        if (uid_match)
            if ((mode >> 16) & evaluation)
                return true;

        return false;
    }

    fs::file_info read_file(const std::string &path) {
        fs::file_info fi;
        fi.authorized = false;
        fi.path = fs::dirname(path);
        fi.name = fs::basename(path);

        // struct stat {
        //     dev_t     st_dev;     /* ID of device containing file */
        //     ino_t     st_ino;     /* inode number */
        //     mode_t    st_mode;    /* protection */
        //     nlink_t   st_nlink;   /* number of hard links */
        //     uid_t     st_uid;     /* user ID of owner */
        //     gid_t     st_gid;     /* group ID of owner */
        //     dev_t     st_rdev;    /* device ID (if special file) */
        //     off_t     st_size;    /* total size, in bytes */
        //     blksize_t st_blksize; /* blocksize for file system I/O */
        //     blkcnt_t  st_blocks;  /* number of 512B blocks allocated */
        //     time_t    st_atime;   /* time of last access */
        //     time_t    st_mtime;   /* time of last modification */
        //     time_t    st_ctime;   /* time of last status change */
        // };
        struct stat sb;
        if (lstat(path.c_str(), &sb) == -1) {
            // File doesn't exist
            fi.exists = false;
            fi.length = 0;
            fi.type = fs::file_type::unknown;
            return fi;
        }

        fi.exists = true;
        fi.length = sb.st_size;
        switch (sb.st_mode & S_IFMT) {
            case S_IFBLK:
                fi.type = fs::file_type::block_device;
                break;
            case S_IFCHR:
                fi.type = fs::file_type::character_device;
                break;
            case S_IFDIR:
                fi.type = fs::file_type::directory;
                break;
            case S_IFIFO:
                fi.type = fs::file_type::fifo;
                break;
            case S_IFLNK:
                fi.type = fs::file_type::symlink;
                break;
            case S_IFREG:
                fi.type = fs::file_type::file;
                break;
            case S_IFSOCK:
                fi.type = fs::file_type::socket;
                break;
            default:
                fi.type = fs::file_type::unknown;
                break;
        }

        if (fi.type == fs::file_type::directory)
            fi.authorized = fs::is_authorized(fi.type, sb.st_mode, sb.st_uid, sb.st_gid, fs::permission_flag::read);
        else
            fi.authorized = true;
        return fi;
    }

    bool exists(const std::string &path) {
        fs::file_info fi = read_file(path);
        return fi.exists;
    }

    template<fs::file_type T> bool is_type(const std::string &path) {
        fs::file_info fi = read_file(path);
        return fi.exists && fi.type == T;
    }

    std::vector<fs::file_info> read_directory(const std::string &path, bool calculate_directory_length = false) {
        std::vector<fs::file_info> contents;

        DIR *dp {nullptr};
        if((dp  = opendir(path.c_str())) == nullptr) {
            if (!fs::exists(path))
                throw std::runtime_error("Directory not found: " + path);
            else
                return contents; // Probably no permissions to read directory contents
        }

        struct dirent *dirp {nullptr};
        while ((dirp = readdir(dp)) != nullptr) {
            std::string filename { std::string(dirp->d_name) };
            if (filename == "." || filename == "..")
                continue; // Skip virtual paths

            fs::file_info fi = read_file(path + '/' + filename);

            if (calculate_directory_length && fi.type == fs::file_type::directory) {
                for (const auto &sub: read_directory(path + '/' + filename, true)) {
                    if (!sub.authorized)
                        fi.authorized = false;
                    fi.length += sub.length;
                }
            }

            contents.push_back(std::move(fi));
        }
        closedir(dp);
        return contents;
    }
}

#endif //__FS_HPP_INCLUDED__
