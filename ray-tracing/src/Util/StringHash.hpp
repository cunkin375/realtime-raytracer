#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

// Overloads allow for heterogeneous lookup
// - dynamic copies will not be made during lookup with a const char *, string_view, or string regardless of
// declared type
struct StringHash
{
    using is_transparent = void;
    std::size_t operator()(const char *str) const { return std::hash<std::string_view>{}(str); }

    std::size_t operator()(std::string_view str) const { return std::hash<std::string_view>{}(str); }

    std::size_t operator()(const std::string &str) const { return std::hash<std::string_view>{}(str); }
};

template <typename T>
using StringMap = std::unordered_map<std::string, T, StringHash, std::equal_to<>>;

using StringSet = std::unordered_set<std::string, StringHash, std::equal_to<>>;
