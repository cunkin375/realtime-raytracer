#pragma once

#include <filesystem>

namespace Util
{

inline std::filesystem::path ResolvePath(const std::filesystem::path &relative_path)
{
    if (std::filesystem::exists(relative_path))
        return relative_path;

    auto current = std::filesystem::current_path();
    while (current.has_parent_path() && current != current.parent_path())
    {
        auto candidate = current / relative_path;
        if (std::filesystem::exists(candidate))
            return candidate;
        current = current.parent_path();
    }
    return relative_path;
}

} // namespace Util
