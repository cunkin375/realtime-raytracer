#pragma once

#include <filesystem>
#include <functional>

class DirectoryWatcher
{
public:
    enum class FileAction
    {
        Modified,
        Created,
        Deleted,
        RenamedFrom,
        RenamedTo
    };

    struct FileEvent
    {
        FileAction action;
        std::filesystem::path filepath;
    };

    using Callback = std::function<void(const FileEvent &)>;

    explicit DirectoryWatcher(const std::filesystem::path &watch_dir, Callback on_change,
                              bool recursive = true);

    ~DirectoryWatcher();

    // No copies
    DirectoryWatcher(const DirectoryWatcher &other) = delete;
    DirectoryWatcher &operator=(const DirectoryWatcher &other) = delete;
    // Movable
    DirectoryWatcher(DirectoryWatcher &&other) noexcept;
    DirectoryWatcher &operator=(DirectoryWatcher &&other) noexcept;

    void PollEvents();

    void SetEnabled(bool enabled);
    bool IsEnabled() const;

private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};
