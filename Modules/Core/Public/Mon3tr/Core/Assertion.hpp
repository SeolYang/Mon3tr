/*
 * Copyright (c) 2026 SeolYang(Yang Kyowon)
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once
#include <cstdlib>
#include <print>
#include <source_location>
#include <stacktrace>

#if defined(DEBUG) || defined(_DEBUG)
// 디버그 모드: 상세한 정보 출력 및 중단
#define M3_ASSERT(x) \
    do { \
        if (!(x)) [[unlikely]] { \
        auto loc = std::source_location::current(); \
        std::println(stderr, "Assertion Failed: {}\nFile: {}:{}\nFunction: {}\nStacktrace:\n{}", \
        #x, loc.file_name(), loc.line(), loc.function_name(), \
        std::stacktrace::current()); \
        std::abort(); \
        } \
    } while(false)
#else
#define M3_ASSERT(x) ((void)0)
#endif

#define M3_PRE_COND(x) M3_ASSERT(x)
#define M3_POST_COND(x) M3_ASSERT(x)

#define M3_UNIMPLEMENTED() M3_ASSERT(false)