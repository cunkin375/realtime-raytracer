#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "DirectoryWatcher.hpp"

#include <iostream>

#include <windows.h>

#include <iostream>
#include <memory>

#include "Util/Log.hpp"
#include "Util/StringHash.hpp"

namespace
{
using DebounceInfo = std::chrono::steady_clock::time_point;
} // namespace

struct DirectoryWatcher::Implementation
{
    // Win32 specific
    HANDLE directory_handle{INVALID_HANDLE_VALUE};
    HANDLE event_handle{nullptr}; // manual reset event for OVERLAPPED
    OVERLAPPED overlapped{};
    bool pending_read{false};
    alignas(DWORD) char buffer[4096];

    // Common across implementations
    std::filesystem::path root_directory{};
    Callback callback_function{};
    bool enabled{false};
    bool recursive{true};
    StringMap<DebounceInfo> file_timers;
    std::chrono::milliseconds debounce_time_milliseconds{250};

    Implementation(const std::filesystem::path &_watch_dir, Callback _on_change, bool _recursive)
        : root_directory{_watch_dir}, callback_function{std::move(_on_change)}, recursive{_recursive}
    {
        directory_handle =
            CreateFileW(root_directory.c_str(), FILE_LIST_DIRECTORY,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);

        if (INVALID_HANDLE_VALUE == directory_handle)
        {
            Log::Fatal("CreateFileW failed!");
            std::exit(EXIT_FAILURE);
        }

        event_handle = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!event_handle)
        {
            Log::Fatal("CreateEventW failed!");
            CloseHandle(directory_handle);
            std::exit(EXIT_FAILURE);
        }

        overlapped.hEvent = event_handle;

        // issue first async read
        BeginRead();
    }

    bool ShouldDebounce(std::string_view filename)
    {
        bool should_debounce = true;
        bool should_not_debounce = false;

        // reject editor temporary/backup files
        if (filename.ends_with('~')                             // vim backup
            || filename.starts_with('.')                        // hidden / editor dotfiles
            || filename.find(".swp") != std::string_view::npos  // vim swap
            || filename.find(".swx") != std::string_view::npos  // vim swap
            || filename.find(".tmp") != std::string_view::npos) // generic temp
        {
            return should_debounce;
        }

        auto has_shader_extension = [](std::string_view name) -> bool
        {
            constexpr std::string_view extensions[] = {".vert", ".frag", ".geom", ".tesc",
                                                       ".tese", ".comp", ".glsl"};
            for (auto extension : extensions)
            {
                if (name.size() >= extension.size() &&
                    name.substr(name.size() - extension.size()) == extension)
                {
                    return true;
                }
            }
            return false;
        };

        if (!has_shader_extension(filename)) { return should_debounce; }

        auto now = std::chrono::steady_clock::now();

        if (auto it = file_timers.find(filename); it != std::ranges::cend(file_timers))
        {
            using namespace std::chrono;
            auto &[key, last_event_time] = *it;
            auto time_elapsed = duration_cast<milliseconds>(now - last_event_time);

            if (time_elapsed < debounce_time_milliseconds)
            {
                last_event_time = now;
                return should_debounce;
            }

            // cooldown expired, event is valid
            last_event_time = now;
            return should_not_debounce;
        }

        file_timers.emplace(std::string{filename}, DebounceInfo{now});
        return should_not_debounce;
    }

    void BeginRead()
    {
        // must reset event_handle at beginning of every read
        ResetEvent(event_handle);

        DWORD bytes_returned = 0;
        BOOL success = ReadDirectoryChangesW(
            directory_handle, buffer, sizeof(buffer), recursive ? TRUE : FALSE,
            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME,
            &bytes_returned, &overlapped, nullptr);

        DWORD errors = GetLastError();
        DWORD PASS_THESE_ERRORS = ERROR_IO_INCOMPLETE | ERROR_IO_PENDING;
        if (!success && !(errors & PASS_THESE_ERRORS))
        {
            Log::Error("ReadDirectoryChangesW failed with code: {}", errors);
            pending_read = false;
            return;
        }

        // read events in Poll();
        pending_read = true;
    }

    void Poll()
    {
        if (!enabled) { return; }

        DWORD bytes_transferred = 0;
        BOOL result = GetOverlappedResult(directory_handle, &overlapped, &bytes_transferred, FALSE);

        if (!result)
        {
            DWORD error = GetLastError();
            DWORD PASS_THESE_ERRORS = ERROR_IO_INCOMPLETE | ERROR_IO_PENDING;
            // currently waiting and non-blocking
            if (error & PASS_THESE_ERRORS) { return; }

            // actual error
            Log::Error("GetOverlappedResult failed with code: {}", error);
            pending_read = false;
            return;
        }

        // buffer overflow, read again
        if (bytes_transferred == 0)
        {
            BeginRead();
            return;
        }

        ParseEventBuffer();
    }

    void ParseEventBuffer()
    {
        char *ptr = buffer;

        while (true)
        {
            auto *info = reinterpret_cast<FILE_NOTIFY_INFORMATION *>(ptr);

            std::wstring wide_name(info->FileName, info->FileNameLength / sizeof(WCHAR));
            std::filesystem::path relative_path{wide_name};

            std::string_view filename{relative_path.filename().string()};

            if (!ShouldDebounce(filename))
            {
                std::filesystem::path full_path = root_directory / relative_path;

                // clang-format off
                // map Win32 actions to FileAction
                FileAction action = FileAction::Modified;
                switch (info->Action) 
                {
                    case FILE_ACTION_ADDED:            action = FileAction::Created; break;
                    case FILE_ACTION_REMOVED:          action = FileAction::Deleted; break;
                    case FILE_ACTION_MODIFIED:         action = FileAction::Modified; break;
                    case FILE_ACTION_RENAMED_OLD_NAME: action = FileAction::RenamedFrom; break;
                    case FILE_ACTION_RENAMED_NEW_NAME: action = FileAction::RenamedTo; break;
                }

                // log action
                std::cout << "DirectoryWatcher: " << full_path << "\n";
                std::cout << "\t detected action: ";
                switch (action) 
                {
                    case FileAction::Created:     std::cout << "Created"; break;
                    case FileAction::Deleted:     std::cout << "Deleted"; break;
                    case FileAction::RenamedFrom: std::cout << "RenamedFrom"; break;
                    case FileAction::RenamedTo:   std::cout << "RenamedTo"; break;
                    case FileAction::Modified:    std::cout << "Modified"; break;
                } std::cout << "\n";
                // clang-format on

                callback_function(FileEvent{action, full_path});
            }

            // at the end of the list, return
            if (info->NextEntryOffset == 0) { break; }

            ptr += info->NextEntryOffset;
        }

        // REQUIRED for Win32, otherwise watcher silently stops
        BeginRead();
    }
};

// === PUBLIC METHODS ===

DirectoryWatcher::DirectoryWatcher(const std::filesystem::path &watch_dir, Callback on_change, bool recursive)
    : impl_{std::make_unique<Implementation>(watch_dir, std::move(on_change), recursive)}
{
}

DirectoryWatcher::~DirectoryWatcher() = default;

DirectoryWatcher::DirectoryWatcher(DirectoryWatcher &&other) noexcept = default;
DirectoryWatcher &DirectoryWatcher::operator=(DirectoryWatcher &&other) noexcept = default;

void DirectoryWatcher::PollEvents() { impl_->Poll(); }
void DirectoryWatcher::SetEnabled(bool enabled) { impl_->enabled = enabled; }
bool DirectoryWatcher::IsEnabled() const { return impl_->enabled; }

#endif
