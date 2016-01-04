#ifndef __FS_HPP_INCLUDED__
#define __FS_HPP_INCLUDED__

#include <string>
#include <vector>
#include <stdexcept>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h> // getcwd()
#include <stdlib.h> // getenv()

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

    enum class file_error {
        none,
        invalid_path,
        file_not_found,
        permission_denied,
        undefined
    };

    struct file_info {
        fs::file_error error;
        fs::file_type type;
        std::string path;
        std::string name;
        unsigned int mode;
        unsigned int uid;
        unsigned int gid;
        unsigned long length;
        unsigned int access_time;
        unsigned int modify_time;
        unsigned int change_time;
    };

    /*
        File mode bits:
           S_ISUID     04000   set-user-ID bit
           S_ISGID     02000   set-group-ID bit (see below)
           S_ISVTX     01000   sticky bit (see below)

           S_IRWXU     00700   owner has read, write, and execute permission
           S_IRUSR     00400   owner has read permission
           S_IWUSR     00200   owner has write permission
           S_IXUSR     00100   owner has execute permission

           S_IRWXG     00070   group has read, write, and execute permission
           S_IRGRP     00040   group has read permission
           S_IWGRP     00020   group has write permission
           S_IXGRP     00010   group has execute permission

           S_IRWXO     00007   others (not in group) have read, write, and
                               execute permission
           S_IROTH     00004   others have read permission
           S_IWOTH     00002   others have write permission
           S_IXOTH     00001   others have execute permission
    */
    enum permission_flag {
        read = 0x04,
        write = 0x02,
        execute = 0x01,
    };

    std::string dirname(const std::string &path) {
        std::size_t pos_last_backslash { path.find_last_of("/\\") };
        if (pos_last_backslash == std::string::npos)
            return path; // No slashes
        if (pos_last_backslash == path.length() - 1)
            return dirname(path.substr(0, pos_last_backslash)); // Skip trailing slash

        return path.substr(0, pos_last_backslash);
    }

    std::string basename(const std::string &path) {
        std::size_t pos_last_backslash { path.find_last_of("/\\") };
        if (pos_last_backslash == std::string::npos)
            return path; // No slashes
        if (pos_last_backslash == path.length() - 1)
            return basename(path.substr(0, pos_last_backslash)); // Skip trailing slash

        return path.substr(pos_last_backslash + 1);
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
        if (path[0] == '~')
            return getenv("HOME") + path.substr(1); // Replace home directory
        if (path[0] != '.')
            return current_working_directory() + '/' + path; // Assume current working directory
        if (path == ".")
            return current_working_directory(); // Current working directory
        if (path[1] == '.')
            return dirname(current_working_directory()) + path.substr(2); // Parent directory: replace with current working directory

        return current_working_directory() + '/' + path;
    }

    bool is_authorized(const fs::file_info &file, const fs::permission_flag &evaluation) {
        if (((file.mode & 0x07) & evaluation) == evaluation)
            return true;

        bool gid_match = (file.gid == getgid());
        if (gid_match)
            if ((((file.mode >> 3) & 0x07) & evaluation) == evaluation)
                return true;

        bool uid_match = (file.uid == getuid());
        if (uid_match)
            if ((((file.mode >> 6) & 0x07) & evaluation) == evaluation)
                return true;

        return false;
    }

    fs::file_info read_file(const std::string &path) {
        fs::file_info fi;
        fi.path = fs::dirname(path);
        fi.name = fs::basename(path);

        struct stat sb;
        if (lstat((fi.path + '/' + fi.name).c_str(), &sb) == -1) {
            // Failed to stat file
            switch (errno) {
                case ENOENT: // A component of path does not name an existing file or path is an empty string.
                    fi.error = fs::file_error::file_not_found;
                    break;
                case EACCES: // A component of the path prefix denies search permission.
                    fi.error = fs::file_error::permission_denied;
                    break;
                case ELOOP: // A loop exists in symbolic links encountered during resolution of the path argument.
                case ENAMETOOLONG: // The length of a pathname exceeds {PATH_MAX} or a pathname component is longer than {NAME_MAX}.
                case ENOTDIR: // A component of the path prefix is not a directory.
                    fi.error = fs::file_error::invalid_path;
                    break;
                case EIO: // An error occurred while reading from the file system.
                case EOVERFLOW: // The file size in bytes or the number of blocks allocated cannot be represented correctly in the structure pointed to by buf.
                    fi.error = fs::file_error::undefined;
                    break;
            }
            fi.length = 0;
            fi.type = fs::file_type::unknown;
            return fi;
        }

        fi.error = fs::file_error::none;
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
        fi.mode = sb.st_mode;
        fi.uid = sb.st_uid;
        fi.gid = sb.st_gid;
        fi.access_time = sb.st_atime;
        fi.modify_time = sb.st_mtime;
        fi.change_time = sb.st_ctime;

        if (fi.type == fs::file_type::directory && !fs::is_authorized(fi, fs::permission_flag::read))
            fi.error = fs::file_error::permission_denied;
        return fi;
    }

    bool exists(const std::string &path) {
        fs::file_info fi = read_file(path);
        return (fi.error != fs::file_error::file_not_found && fi.error != fs::file_error::invalid_path);
    }

    template<fs::file_type T> bool is_type(const std::string &path) {
        fs::file_info fi = read_file(path);
        return (fi.error == fs::file_error::none && fi.type == T);
    }

    std::vector<fs::file_info> read_directory(const std::string &path, bool enter_directory, bool calculate_directory_length) {
        std::vector<fs::file_info> contents;

        DIR *dp {nullptr};
        if((dp  = opendir(path.c_str())) == nullptr) {
            if (!fs::exists(path))
                throw std::runtime_error("Directory not found: " + path);
            else
                return contents; // Probably no permissions to read directory contents
        }

        fs::file_info fi_root = read_file(path);
        if (fi_root.type != fs::file_type::directory)
            throw new std::runtime_error("Path is not a directory: " + path);

        if (!enter_directory) {
            if (calculate_directory_length) {
                for (const auto &fi_child: read_directory(path, true, true)) {
                    if (fi_child.error == fs::file_error::permission_denied)
                        fi_root.error = fs::file_error::permission_denied;
                    fi_root.length += fi_child.length;
                }
            }
            contents.push_back(std::move(fi_root));
            return contents;
        }

        struct dirent *dirp {nullptr};
        while ((dirp = readdir(dp)) != nullptr) {
            std::string filename { std::string(dirp->d_name) };
            if (filename == "." || filename == "..")
                continue; // Skip virtual paths

            fs::file_info fi_child = read_file(path + '/' + filename);

            if (calculate_directory_length && fi_child.type == fs::file_type::directory) {
                for (const auto &fi_grandchild: read_directory(path + '/' + filename, enter_directory, true)) {
                    if (fi_grandchild.error == fs::file_error::permission_denied)
                        fi_child.error = fs::file_error::permission_denied;
                    fi_child.length += fi_grandchild.length;
                }
            }

            contents.push_back(std::move(fi_child));
        }
        closedir(dp);
        return contents;
    }
}

#endif //__FS_HPP_INCLUDED__
