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
#include <Mon3tr/Core/Platform.hpp>
#include <Mon3tr/Core/Types.hpp>

namespace mon3tr {
    template<typename T>
    constexpr bool IsPowerOf2(T x) noexcept { return ((x != 0) && !(x & (x - 1))); }

    constexpr uint64 AlignUp(const uint64 value, const uint64 alignment) noexcept {
        if (alignment == 0) {
            return value;
        }

        M3_ASSERT(IsPowerOf2(alignment));
        return (value + (alignment - 1)) & ~(alignment - 1);
    }

    class Page {
    public:
        static uint64 GetSize() {
            static uint64 cachedPageSize = []() {
#ifdef M3_PLATFORM_WINDOWS
                SYSTEM_INFO sysInfo;
                GetSystemInfo(&sysInfo);
                return sysInfo.dwPageSize;
#else
                M3_UNIMPLEMENTED();
                return 0x1000ui64;
#endif
            }();

            return cachedPageSize;
        }
    };
}
