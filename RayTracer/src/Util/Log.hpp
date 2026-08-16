#pragma once

#include <filesystem>
#include <format>
#include <print>
#include <source_location>

namespace Log
{

enum class Level
{
    Debug,
    Info,
    Warn,
    Error,
    Fatal
};

constexpr const char *GetLevel(Level level)
{
    switch (level)
    {
        case Level::Debug: return "DEBUG";
        case Level::Info: return "INFO";
        case Level::Warn: return "WARN";
        case Level::Error: return "ERROR";
        case Level::Fatal: return "FATAL";
        default: return "UNKNOWN";
    }
}

// use of deduction guides limits to a struct, which is whatever
// struct that takes in some number of arguments, configurations for levels are defined later
template <Level Lvl, typename... Args>
struct Print
{
    Print(std::format_string<Args...> message, Args &&...args,
          std::source_location location = std::source_location::current())
    {
        std::println("[{}] {}:{} {}", GetLevel(Lvl),
                     std::filesystem::path{location.file_name()}.filename().string(), location.line(),
                     std::format(message, std::forward<Args>(args)...));
    }
};

// deduction guide to support default location argument
// NOTE: clang does not like this, whatever
template <Level Lvl, typename... Args>
Print(std::format_string<Args...>, Args &&...) -> Print<Lvl, Args...>;

template <typename... Args>
using Debug = Print<Level::Debug, Args...>;

template <typename... Args>
using Info = Print<Level::Info, Args...>;

template <typename... Args>
using Warn = Print<Level::Warn, Args...>;

template <typename... Args>
using Error = Print<Level::Error, Args...>;

template <typename... Args>
using Fatal = Print<Level::Fatal, Args...>;

} // namespace Log
