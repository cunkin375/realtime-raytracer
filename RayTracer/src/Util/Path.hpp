#pragma once

#include <filesystem>

#include <fstream>
#include <iostream>

#include <source_location>

#include "Log.hpp"

namespace Util
{

// Resolves a given path regardless of the current working directory with respect to project root
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

// Loads file into binary format (std::vector<char>)
std::vector<char> LoadAsBinary(const std::string_view path,
                               std::source_location   location = std::source_location::current())
{
    auto resolved = ResolvePath(path);
    auto file     = std::ifstream{ resolved, std::ios::binary };

    if (!file.is_open())
    {
        Log::PrintAt<Log::Level::Error>(location, "Failed to open {}", path);
        return {};
    }

    // std::vector<std::byte> does not work here
    auto data = std::vector<char>{ (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>() };

    if (data.empty())
    {
        Log::PrintAt<Log::Level::Error>(location, "Failed to load data: {} is empty", path);
        return {};
    }

    return data;
}

} // namespace Util
