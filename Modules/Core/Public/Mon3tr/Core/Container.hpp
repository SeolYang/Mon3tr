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
#include <EASTL/vector.h>
#include <EASTL/queue.h>
#include <EASTL/array.h>
#include <EASTL/stack.h>
#include <Mon3tr/Core/Memory.hpp>
#include <Mon3tr/Core/Allocator.hpp>

namespace mon3tr {
    template<typename T, MemoryCategory C = M3_MEM_CATEGORY(Unspecified)>
    using Vector = eastl::vector<T, Allocator<C> >;

    template<typename T, MemoryCategory C = M3_MEM_CATEGORY(Unspecified)>
    using Queue = eastl::queue<T, eastl::deque<T, Allocator<C> > >;

    template<typename T, MemoryCategory C = M3_MEM_CATEGORY(Unspecified)>
    using Stack = eastl::stack<T, eastl::deque<T, Allocator<C> > >;

    template<typename T, size_t N>
    using Array = eastl::array<T, N>;
}
