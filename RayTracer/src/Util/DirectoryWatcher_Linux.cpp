#ifdef __linux__
#include "DirectoryWatcher.hpp"

#include <cstring>
#include <iostream>
#include <unordered_map>

#include <sys/inotify.h>
#include <unistd.h>

#include "Util/Aliases.hpp"
#include "Util/StringHash.hpp"

namespace
{
using DebounceInfo = std::chrono::steady_clock::time_point;
} // namespace

struct DirectoryWatcher::Implementation
{
    i32 inotify_fd{-1};
    std::unordered_map<i32, std::filesystem::path> watch_descriptor_to_path{};
    std::filesystem::path root_directory{};
    Callback callback_function{};
    bool enabled{false};
    bool recursive{true};

    StringMap<DebounceInfo> file_timers;
    std::chrono::milliseconds debounce_time_milliseconds{250};

    Implementation(const std::filesystem::path &_watch_dir, Callback _on_change, bool _recursive)
        : root_directory{_watch_dir}, callback_function{std::move(_on_change)}, recursive{_recursive}
    {
        inotify_fd = inotify_init1(IN_NONBLOCK);
        if (-1 == inotify_fd)
        {
            std::cerr << "DirectoryWatcher: inotify failed to initialize!\n";
            std::exit(EXIT_FAILURE);
        }

        AddWatch(root_directory);

        if (recursive) { AddWatchRecursive(root_directory); }
    }

    ~Implementation()
    {
        for (auto &[wd, path] : watch_descriptor_to_path)
        {
            inotify_rm_watch(inotify_fd, wd);
        }
        close(inotify_fd);
    }

    void AddWatch(const std::filesystem::path &directory)
    {
        i32 wd = inotify_add_watch(inotify_fd, directory.c_str(),
                                   IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);

        if (-1 == wd)
        {
            std::cerr << "inotify failed to add watch for: " << directory << "\n";
            return;
        }

        auto [it, inserted] = watch_descriptor_to_path.try_emplace(wd, directory);
        if (!inserted)
        {
            std::cerr << "DirectoryWatcher: watch_descriptor already present in watch_descriptor_to_path!\n";
        }
    }

    void AddWatchRecursive(const std::filesystem::path &directory)
    {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(directory))
        {
            if (entry.is_directory()) { AddWatch(entry.path()); }
        }
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

    void RemoveWatch(i32 wd)
    {
        inotify_rm_watch(inotify_fd, wd);
        watch_descriptor_to_path.erase(wd);
    }

    void Poll()
    {
        if (!enabled) { return; }

        alignas(struct inotify_event) char buffer[4096];
        ssize_t bytes_read = read(inotify_fd, buffer, sizeof(buffer));

        if (bytes_read <= 0)
        {
            return; // no events or EAGAIN
        }

        char *ptr = buffer;

        while (ptr < buffer + bytes_read)
        {
            // why reinterpret_cast exists here:
            // - inotify_event has a flexible array member (name[]) whose data follows the struct header in
            //   the buffer
            // - using memcpy with only sizeof(inotify_event) into a local loses name data
            auto *event = reinterpret_cast<struct inotify_event *>(ptr);

            if (event->len > 0)
            {
                std::string_view filename{event->name, std::strlen(event->name)};

                if (ShouldDebounce(filename))
                {
                    ptr += sizeof(struct inotify_event) + event->len;
                    continue;
                }

                // resolve the directory this watch descriptor belongs to
                std::filesystem::path dir = root_directory;
                if (auto it = watch_descriptor_to_path.find(event->wd);
                    it != std::ranges::cend(watch_descriptor_to_path))
                {
                    dir = it->second;
                }

                std::filesystem::path full_path = dir / filename;

                // map inotify mask to FileAction
                FileAction action = FileAction::Modified;
                if (event->mask & IN_CREATE) { action = FileAction::Created; }
                else if (event->mask & IN_DELETE) { action = FileAction::Deleted; }
                else if (event->mask & IN_MOVED_FROM) { action = FileAction::RenamedFrom; }
                else if (event->mask & IN_MOVED_TO) { action = FileAction::RenamedTo; }

                // if a new subdirectory was created and we're recursive, watch it
                if ((event->mask & IN_CREATE) && (event->mask & IN_ISDIR) && recursive)
                {
                    AddWatch(full_path);
                }

                std::cout << "DirectoryWatcher: " << full_path << "\n";
                std::cout << "\t detected action: ";
                switch (action)
                {
                    case FileAction::Created: std::cout << "Created"; break;
                    case FileAction::Deleted: std::cout << "Deleted"; break;
                    case FileAction::RenamedFrom: std::cout << "RenamedFrom"; break;
                    case FileAction::RenamedTo: std::cout << "RenamedTo"; break;
                    case FileAction::Modified: std::cout << "Modified"; break;
                }
                std::cout << "\n";

                callback_function(FileEvent{action, full_path});
            }

            // Advance pointer to next event
            ptr += sizeof(struct inotify_event) + event->len;
        }
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
