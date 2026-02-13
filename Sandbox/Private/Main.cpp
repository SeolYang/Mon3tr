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

#include <Mon3tr/Core/CoreMinimal.hpp>
#include <Mon3tr/Core/HandleManager.hpp>
#include <Mon3tr/Core/VirtualArray.hpp>

#include "../../ThirdParty/directxtk12-src/Inc/DirectXHelpers.h"

int main() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("console", console_sink);
    spdlog::set_default_logger(logger);

    namespace m3 = mon3tr;
    m3::HandleManager<int> test;

    const m3::Handle<int> handle = test.Create(35);
    static_assert(sizeof(m3::Handle<int>) == sizeof(m3::uint64));
    M3_ASSERT(!handle.IsNull());

    int* ptr = test.GetMutable(handle);
    M3_ASSERT(ptr != nullptr);
    M3_ASSERT(*ptr == 35);
    test.Destroy(handle);

    M3_ASSERT(mon3tr::AlignUp(3, 1024) == 1024);

    constexpr std::string_view        kStrTable[8] = {"A", "AB", "ABC", "ABCD", "ABCDE", "ABCDEF", "ABCDEFG", "ABCDEFGH"};
    mon3tr::VirtualArray<std::string> vArray{64};
    std::vector<std::string>          vec;
    vec.reserve(64);

    for (size_t idx = 0; idx < 64; ++idx) {
        vArray.EmplaceBack(kStrTable[idx % 8]);
        vec.emplace_back(kStrTable[idx % 8]);
    }

    M3_ASSERT(vArray.GetSize() == 64);
    for (size_t idx = 0; idx < 64; ++idx) {
        M3_ASSERT(vec[idx] == vArray[idx]);
    }

    for (size_t idx = 0; idx < 32; ++idx) {
        vArray.PopBack();
        vec.pop_back();
    }
    M3_ASSERT(vec.back() == vArray.GetLastElement());
    M3_ASSERT(vec.size() == vArray.GetSize());

    vArray.Clear();
    M3_ASSERT(vArray.GetSize() == 0);
    M3_ASSERT(vArray.IsEmpty());

    return 0;
}
