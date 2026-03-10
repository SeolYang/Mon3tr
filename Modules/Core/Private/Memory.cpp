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
#pragma warning(push)
#pragma warning(disable : 4324)
#pragma warning(disable : 4864)
#include <snmalloc/snmalloc.h>
#pragma warning(pop)
#include <Mon3tr/Core/CoreMinimal.hpp>

#if defined(DEBUG) || defined(_DEBUG)
#ifndef M3_TRACK_MEM_ALLOCATIONS
#define M3_TRACK_MEM_ALLOCATIONS
#endif
#endif

M3_DEFINE_MEM_CATEGORY(Unspecified)

M3_DECLARE_LOG_CATEGORY(Memory)

M3_DEFINE_LOG_CATEGORY(Memory)

namespace mon3tr::internal {
#ifdef M3_TRACK_MEM_ALLOCATIONS
    struct AllocationInfo {
        uint64           Size = 0;
        std::string_view CategoryName = M3_MEM_CATEGORY(Unspecified)::Name;
        std::stacktrace  CallStack{};
    };

    static std::shared_mutex                                   gAllocationMutex;
    static ankerl::unordered_dense::map<void*, AllocationInfo> gAllocationMap;
#endif

    void* Allocate(const uint64 size, const uint64 alignment, const std::string_view categoryName) {
        M3_ASSERT(size > 0);
        M3_ASSERT(alignment > 0 && IsPowerOf2(alignment));
        void* const ptr = snmalloc::alloc_aligned(alignment, size);

#ifdef M3_TRACK_MEM_ALLOCATIONS
        {
            const std::stacktrace callStack = std::stacktrace::current();
            std::unique_lock      lock(gAllocationMutex);
            gAllocationMap[ptr] = AllocationInfo{
                .Size = size,
                .CategoryName = categoryName,
                .CallStack = callStack
            };
        }
#endif

        return ptr;
    }

    void Deallocate(void* const ptr, const std::string_view categoryName) {
        M3_ASSERT(ptr != nullptr);
#ifdef M3_TRACK_MEM_ALLOCATIONS
        {
            std::unique_lock lock(gAllocationMutex);
            const auto       itr = gAllocationMap.find(ptr);
            M3_ASSERT(itr != gAllocationMap.end());
            if (itr == gAllocationMap.end()) {
                M3_LOG(Memory, Fatal, "Invalid memory deallocation : {}", ptr);
                M3_ASSERT(false);
            }

            if (itr->second.CategoryName != categoryName) {
                M3_LOG(Memory, Fatal, "Memory category unmatched (expected: {}, requested: {}): {}\n CallStack:\n{}",
                       itr->second.CategoryName, categoryName,
                       ptr,
                       itr->second.CallStack);
                M3_ASSERT(false);
            }
            gAllocationMap.erase(itr);
        }
#endif

        snmalloc::dealloc(ptr);
    }

    void DumpMemoryLeaks() {
#ifdef M3_TRACK_MEM_ALLOCATIONS
        std::shared_lock lock(gAllocationMutex);
        if (!gAllocationMap.empty()) {
            M3_LOG(Memory, Fatal, "Memory leaks found: {}", gAllocationMap.size());
            for (const auto& [ptr, info]: gAllocationMap) {
                M3_LOG(Memory, Fatal, "[{}] At {}, Size: {}\n{}", info.CategoryName, ptr, info.Size, info.CallStack);
            }
        }
#endif
    }

    uint64 GetAllocationSize(const void* const ptr) {
        return snmalloc::alloc_size(ptr);
    }
}
