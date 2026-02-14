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
#include <Mon3tr/Core/Types.hpp>
#include <Mon3tr/Core/Assertion.hpp>
#include <Mon3tr/Core/Platform.hpp>

namespace mon3tr {
    /*
    * Reference about PAGE_XXXX constants & WIN32 Virtual Mem API
    * https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc
    * https://learn.microsoft.com/en-us/windows/win32/memory/memory-protection-constants
    */
    class VirtualMemory {
    public:
        static void* Reserve(const uint64 sizeBytes) {
            M3_PRE_COND(sizeBytes > 0);

            void* memPtr = nullptr;
#ifdef M3_PLATFORM_WINDOWS
            memPtr = VirtualAlloc(nullptr, sizeBytes, MEM_RESERVE, PAGE_NOACCESS);
#else
            M3_UNIMPLEMENTED();
#endif
            return memPtr;
        }

        static bool Commit(void* memPtr, const uint64 sizeBytes) {
#ifdef M3_PLATFORM_WINDOWS
            void* result = VirtualAlloc(memPtr, sizeBytes, MEM_COMMIT, PAGE_READWRITE);
            return result != nullptr;
#else
            M3_UNIMPLEMENTED();
            return false;
#endif
        }

        static void Free(void* memPtr) {
            if (memPtr == nullptr) {
                M3_ASSERT(false);
                return;
            }

#ifdef M3_PLATFORM_WINDOWS
            VirtualFree(memPtr, 0, MEM_RELEASE);
#else
            M3_UNIMPLEMENTED();
#endif
        }
    };
}
