/*
 * common.h
 * Copyright (C) 2017 Korepanov Vyacheslav <real93@live.ru>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef COMMON_H
#define COMMON_H

#include <cstdio>
#include <functional>
#include <sstream>
#include <utility>

/**
 * @brief Get specific type from string.
 *
 * @tparam T - Type.
 * @param s - String.
 *
 * @return T
 */
template <typename T>
inline T getFromStr(const std::string& s) {
    T result;
    std::istringstream is(s);
    is >> result;
    return result;
}

/**
 * @brief Call a function which returns negative numeric value on error.
 *
 * @tparam Func - Function type.
 * @tparam FailCallback - Type of callback function with zero arguments.
 * @tparam Args - Type of arguments to a Func function.
 * @param callback - Callback function which called then error occurs.
 * @param func - Function.
 * @param args - Arguments which passes to a function.
 *
 * @return Returns result of func(args) call.
 */
template <typename Func, typename FailCallback, typename ...Args>
auto callStdlibFunc(const FailCallback& callback, const Func& func, Args&& ...args)
                 -> decltype(func(std::forward<Args>(args)...)) {
    const auto funcResult = func(std::forward<Args>(args)...);
    if (funcResult < 0) {
        perror("");
        callback();
    }
    return funcResult;
}

#endif /* !COMMON_H */
