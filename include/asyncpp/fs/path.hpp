#pragma once
#include <concepts>
#include <Windows.h>
#include "../generator.hpp"
#include "../task.hpp"

namespace async::fs
{
    template <typename StringType = std::string>
    class path
    {
    public:
        using CharType = typename StringType::value_type;

        class attributes
        {
        public:
            attributes() = default;

            attributes(std::uint32_t flags) noexcept
                : _flags(flags)
            {
            }

            bool is_archive() const noexcept
            {
                return (_flags & FILE_ATTRIBUTE_ARCHIVE);
            }

            bool is_compressed() const noexcept
            {
                return (_flags & FILE_ATTRIBUTE_COMPRESSED);
            }

            bool is_device() const noexcept
            {
                return (_flags & FILE_ATTRIBUTE_DEVICE);
            }

            bool is_directory() const noexcept
            {
                return (_flags & FILE_ATTRIBUTE_DIRECTORY);
            }

            bool is_encrypted() const noexcept
            {
                return (_flags & FILE_ATTRIBUTE_ENCRYPTED);
            }

            bool is_hidden() const noexcept
            {
                return (_flags & FILE_ATTRIBUTE_HIDDEN);
            }

            bool is_integrity_stream() const noexcept
            {
                return (_flags & FILE_ATTRIBUTE_INTEGRITY_STREAM);
            }

            bool is_normal() const noexcept
            {
                return (_flags & FILE_ATTRIBUTE_NORMAL);
            }

            bool is_not_content_indexed() const noexcept
            {
                return (_flags & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
            }

            bool is_no_scrub_data() const noexcept
            {
                return (_flags & FILE_ATTRIBUTE_NO_SCRUB_DATA);
            }

            bool is_offline() const noexcept
            {
                return (_flags & FILE_ATTRIBUTE_OFFLINE);
            }

            bool is_read_only() const noexcept
            {
                return (_flags & FILE_ATTRIBUTE_READONLY);
            }

            bool is_recall_on_data_access() const noexcept
            {
                return (_flags & FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS);
            }

            bool is_recall_on_open() const noexcept
            {
                return (_flags & FILE_ATTRIBUTE_RECALL_ON_OPEN);
            }

            bool is_reparse_point() const noexcept
            {
                return (_flags & FILE_ATTRIBUTE_REPARSE_POINT);
            }

            bool is_sparse_file() const noexcept
            {
                return (_flags & FILE_ATTRIBUTE_SPARSE_FILE);
            }

            bool is_system() const noexcept
            {
                return (_flags & FILE_ATTRIBUTE_SYSTEM);
            }

            bool is_temporary() const noexcept
            {
                return (_flags & FILE_ATTRIBUTE_TEMPORARY);
            }

            bool is_virtual() const noexcept
            {
                return (_flags & FILE_ATTRIBUTE_VIRTUAL);
            }

            bool exists() const noexcept
            {
                return (_flags != INVALID_FILE_ATTRIBUTES);
            }

        private:
            std::uint32_t _flags;
        };

        path() = default;

        path(const path &other)
            : _text(other._text)
        {
        }

        path(path &&other)
            : _text(std::move(other._text))
        {
        }

        path(const std::convertible_to<StringType> auto &source)
            : _text(source)
        {
        }

        path(std::convertible_to<StringType> auto &&source)
            : _text(std::move(source))
        {
        }

        path &operator=(const path &other)
        {
            _text = other._text;
            return *this;
        }

        path &operator=(path &&other)
        {
            _text = std::move(other._text);
            return *this;
        }

        attributes get_attributes() const
        {
            if constexpr (std::is_same_v<StringType, std::wstring>)
            {
                return attributes(GetFileAttributesW(_text.c_str()));
            }
            else
            {
                return attributes(GetFileAttributesA(_text.c_str()));
            }
        }

        std::chrono::sys_time<std::chrono::seconds> get_creation_time() const
        {
            FILETIME creation_time;
            HANDLE handle = NULL;

            if constexpr (std::is_same_v<StringType, std::wstring>)
            {
                handle = CreateFileW(_text.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            }
            else if constexpr (std::is_same_v<StringType, std::string>)
            {
                handle = CreateFileA(_text.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            }
            else
            {
                static_assert(false, "Unsupported StringType");
            }

            auto err = GetLastError();
            if (handle == INVALID_HANDLE_VALUE)
            {
                auto err = GetLastError();
                throw std::system_error(err, std::system_category(), "Failed to get creation time");
            }

            GetFileTime(handle, &creation_time, NULL, NULL);
            CloseHandle(handle);
            return std::chrono::sys_time<std::chrono::seconds>(std::chrono::seconds(*(std::uint64_t *)&creation_time / 10000000 - 11644473600));
        }

        std::chrono::sys_time<std::chrono::seconds> get_last_access_time() const
        {
            FILETIME last_access_time;
            HANDLE handle = NULL;

            if constexpr (std::is_same_v<StringType, std::wstring>)
            {
                handle = CreateFileW(_text.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            }
            else if constexpr (std::is_same_v<StringType, std::string>)
            {
                handle = CreateFileA(_text.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            }
            else
            {
                static_assert(false, "Unsupported StringType");
            }

            auto err = GetLastError();
            if (handle == INVALID_HANDLE_VALUE)
            {
                auto err = GetLastError();
                throw std::system_error(err, std::system_category(), "Failed to get creation time");
            }

            GetFileTime(handle, NULL, &last_access_time, NULL);
            CloseHandle(handle);
            return std::chrono::sys_time<std::chrono::seconds>(std::chrono::seconds(*(std::uint64_t *)&last_access_time / 10000000 - 11644473600));
        }

        std::chrono::sys_time<std::chrono::seconds> get_last_write_time() const
        {
            FILETIME last_write_time;
            HANDLE handle = NULL;

            if constexpr (std::is_same_v<StringType, std::wstring>)
            {
                handle = CreateFileW(_text.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            }
            else if constexpr (std::is_same_v<StringType, std::string>)
            {
                handle = CreateFileA(_text.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            }
            else
            {
                static_assert(false, "Unsupported StringType");
            }

            auto err = GetLastError();
            if (handle == INVALID_HANDLE_VALUE)
            {
                auto err = GetLastError();
                throw std::system_error(err, std::system_category(), "Failed to get creation time");
            }

            GetFileTime(handle, NULL, NULL, &last_write_time);
            CloseHandle(handle);
            return std::chrono::sys_time<std::chrono::seconds>(std::chrono::seconds(*(std::uint64_t *)&last_write_time / 10000000 - 11644473600));
        }

        StringType file_name() const
        {
            auto pos = _text.find_last_of(_preferred_seperator());
            auto end = _text.end();

            if (pos == std::string::npos)
            {
                return _text;
            }

            return StringType(_text.begin() + pos + 1, end);
        }

        bool is_absolute() const
        {
            return _contains_drive_letter() && (_text.size() >= 3 && _is_slash(_text[2]));
        }

        std::vector<std::byte> read_file() const
        {
            std::vector<std::byte> buffer;
            HANDLE handle = NULL;

            if constexpr (std::is_same_v<StringType, std::wstring>)
            {
                handle = CreateFileW(_text.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            }
            else if constexpr (std::is_same_v<StringType, std::string>)
            {
                handle = CreateFileA(_text.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            }
            else
            {
                static_assert(false, "Unsupported StringType");
            }

            if (handle == INVALID_HANDLE_VALUE)
            {
                auto err = GetLastError();
                throw std::system_error(err, std::system_category(), "Failed to read file");
            }

            std::uint64_t size = 0;
            if (!GetFileSizeEx(handle, (PLARGE_INTEGER)&size))
            {
                auto err = GetLastError();
                throw std::system_error(err, std::system_category(), "Failed to get file size");
            }

            buffer.resize(size);
            DWORD bytes_read = 0;
            if (!ReadFile(handle, buffer.data(), buffer.size(), &bytes_read, NULL))
            {
                auto err = GetLastError();
                throw std::system_error(err, std::system_category(), "Failed to read file");
            }

            CloseHandle(handle);
            return buffer;
        }

        task<std::vector<std::byte>> read_file_async() const
        {
            co_return read_file();
        }

        std::string read_file_text() const
        {
            std::vector<std::byte> buffer = read_file();
            return std::string(reinterpret_cast<const char *>(buffer.data()), buffer.size());
        }

        task<std::string> read_file_text_async() const
        {
            co_return read_file_text();
        }

        generator<path> walk_directory() const
        {
            WIN32_FIND_DATA find_data;
            HANDLE handle = NULL;

            if constexpr (std::is_same_v<StringType, std::wstring>)
            {
                std::wstring pattern = _text + L"\\*";
                handle = FindFirstFileW(pattern.c_str(), &find_data);
            }
            else if constexpr (std::is_same_v<StringType, std::string>)
            {
                std::string pattern = _text + "\\*";
                handle = FindFirstFileA(pattern.c_str(), &find_data);
            }
            else
            {
                static_assert(false, "Unsupported StringType");
            }

            if (handle == INVALID_HANDLE_VALUE)
            {
                auto err = GetLastError();
                throw std::system_error(err, std::system_category(), "Failed to walk directory");
            }

            do
            {
                co_yield path(_text + _preferred_seperator() + find_data.cFileName);
            } while (FindNextFile(handle, &find_data) != 0);

            FindClose(handle);
        }

        std::string_view parent_path() const
        {
            auto pos = _text.find_last_of(_preferred_seperator());
            if (pos == std::string::npos)
            {
                return std::string_view();
            }

            return std::string_view(_text.data(), pos);
        }

        std::string_view stem() const
        {
            auto pos = _text.find_last_of(_preferred_seperator());
            if (pos == std::string::npos)
            {
                return _text;
            }

            auto end = _text.end();
            auto ext_pos = _text.find_last_of('.');
            if(ext_pos == std::string::npos)
            {
                return std::string_view(_text.data() + (pos + 1), _text.size());
            }

            return std::string_view(_text.data() + (pos + 1), ext_pos - (pos + 1));
        }

        std::string_view extension() const
        {
            auto pos = _text.find_last_of('.');
            if (pos == std::string::npos)
            {
                return std::string_view();
            }

            auto end = _text.end();
            auto ext_pos = _text.find_last_of('.');
            if (ext_pos == std::string::npos)
            {
                return std::string_view();
            }

            return std::string_view(_text.data() + (pos + 1), _text.size());
        }
    private:
        StringType _text;

        StringType::iterator _last_seperator() const
        {
            auto it = _text.end();
            while (it != _text.begin())
            {
                --it;
                if (*it == _preferred_seperator())
                {
                    return it;
                }
            }

            return it;
        }

        bool _contains_drive_letter() const
        {
            return _text.size() > 2 && _text[1] == ':';
        }

        bool _is_slash(CharType c) const
        {
            return c == _preferred_seperator();
        }

        auto _preferred_seperator() const
        {
            if constexpr (std::is_same_v<StringType, std::string>)
            {
                return '\\';
            }
            else if constexpr (std::is_same_v<StringType, std::wstring>)
            {
                return L'\\';
            }
            else
            {
                static_assert(false, "Unsupported string type");
            }
        }
    };
}