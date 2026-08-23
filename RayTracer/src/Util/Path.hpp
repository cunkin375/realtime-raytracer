#pragma once

#include <filesystem>

#include <fstream>
#include <iostream>

#include <source_location>

#include "Log.hpp"

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

std::vector<char> LoadAsBinary(const std::string_view path,
                               std::source_location location = std::source_location::current())
{
    auto resolved = ResolvePath(path);
    auto file = std::ifstream{ resolved, std::ios::binary };

    if (!file.is_open())
    {
        using namespace Log;
        Log::PrintAt<Level::Error>(location, "Failed to open {}", path);
        return {};
    }

    auto data = std::vector<char>{ (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>() };

    if (data.empty())
    {
        using namespace Log;
        Log::PrintAt<Level::Error>(location, "Failed to load data: {} is empty", path);
        return {};
    }

    return data;
}

} // namespace Util
