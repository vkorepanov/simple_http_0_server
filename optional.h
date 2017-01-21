/*
 * optional.h
 * Copyright (C) 2017 Korepanov Vyacheslav <real93@live.ru>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef OPTIONAL_H
#define OPTIONAL_H

#include <cassert>
#include <type_traits>
#include <utility>

template <typename T>
class optional
{
    public:
        optional() noexcept
            : initialized(false)
        {}
        explicit optional(T&& val) noexcept
            : initialized(true)
        {
            new (&value) T(std::forward<T>(val));
        }

        optional(const optional<T>& other) = default;
        optional(optional<T>&& other) = default;
        optional& operator=(optional<T>&& other) = default;

        ~optional() {
            if (initialized)
                castValue()->~T();
        }

        operator bool() const noexcept {
            return initialized;
        }

        T&& take() noexcept {
            assert(initialized);
            return std::move(*castValue());
        }

    private:
        T* castValue() noexcept {
            return reinterpret_cast<T*>(&value);
        }

    private:
        bool initialized;
        std::aligned_storage<sizeof(T), alignof(T)> value;
};

template <typename T>
optional<T> nothing() {
    return optional<T>();
}

template <typename T>
optional<T> just(T&& val) {
    return optional<T>(std::forward<T>(val));
}

#endif /* !OPTIONAL_H */
