#pragma once

#include <functional>
#include <utility>

template <typename T, T Invalid = {}>
class AutoRelease
{
private:
    T object_;
    std::function<void(T)> deleter_;

public:
    AutoRelease() : AutoRelease({}, nullptr) {}
    AutoRelease(T object, std::function<void(T)> deleter) noexcept : object_{object}, deleter_{deleter} {}

    // Delete copy constructors
    AutoRelease(const AutoRelease &) = delete;
    AutoRelease &operator=(const AutoRelease &) = delete;

    AutoRelease(AutoRelease &&other) noexcept : object_{other.object_}, deleter_{std::move(other.deleter_)}
    { other.object_ = Invalid; }

    AutoRelease &operator=(AutoRelease &&other) noexcept
    {
        auto new_object = AutoRelease{std::move(other)};
        Swap(new_object);
        return *this;
    }

    ~AutoRelease() { Reset(); }

    T *operator&() { return std::addressof(object_); }

    auto Swap(AutoRelease &other)
    {
        std::ranges::swap(object_, other.object_);
        std::ranges::swap(deleter_, other.deleter_);
    }

    void Reset() noexcept
    {
        if ((object_ != Invalid) && deleter_) { deleter_(object_); }
        object_ = Invalid;
    }

    T Get() { return object_; }

    operator T() const { return object_; }
};
