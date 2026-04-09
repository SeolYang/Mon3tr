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
#include <Mon3tr/Core/CoreMinimal.hpp>
#include <Mon3tr/Asset/AssetMinimal.hpp>

namespace mon3tr::asset {
    // Asset을 상속 받는 에셋의 경우 EAssetType의 요소와 1:1로 대응되어야 한다
    // 또한 public static constexpr 멤버 상수로 kAssetType을 정의해야 하며, GetType() 함수를 오버라이드 하여 정의한 kAssetType을 반환하여야 한다.
    class Asset {
        friend class AssetManager;

    public:
        Asset(const Asset&) = delete;

        Asset(Asset&&) noexcept = delete;

        virtual ~Asset() = default;

        Asset& operator=(const Asset&) = delete;

        Asset& operator=(Asset&&) noexcept = delete;

        [[nodiscard]] Guid GetGuid() const noexcept { return guid_; }

        virtual EAssetType GetType() const noexcept = 0;

    private:
        std::atomic_uint64_t refCounter_{1};
        Guid                 guid_;
        fs::path             label_;
    };
}
