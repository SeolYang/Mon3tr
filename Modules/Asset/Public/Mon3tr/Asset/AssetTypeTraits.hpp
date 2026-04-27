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
#include <Mon3tr/Asset/AssetCoreTypes.hpp>
#include <type_traits>

namespace mon3tr::asset {
    template<typename T>
    concept AssetImporterTrait = requires(const AssetImportPayload<T>& payload)
    {
        // 임포터의 이름 (권장 사항: static constexpr)
        std::is_same_v<std::string_view, decltype(T::kName)>;

        // 임포터의 버전 (권장 사항: 호환되는 로더의 버전과 동일)
        std::is_same_v<uint64, decltype(T::kVersion)>;

        // 임포터 지정 임포트 설명자
        typename T::Desc;
        std::is_class_v<typename T::Desc>;

        // 임포트 결과 열거자
        typename T::EResult;
        std::is_enum_v<typename T::EResult>;
        T::EResult::Success;

        // Import 함수 조건
        std::is_enum_v<decltype(T::Import(payload))>;
    };

    template<typename T>
    concept MultipleAssetImporterTrait = requires(const AssetImportPayload<T>& payload)
    {
        // 임포터의 이름 (권장 사항: static constexpr)
        std::is_same_v<std::string_view, decltype(T::kName)>;

        // 임포터의 버전 (권장 사항: 호환되는 로더의 버전과 동일)
        std::is_same_v<uint64, decltype(T::kVersion)>;

        // 임포터 지정 임포트 설명자
        typename T::Desc;
        std::is_class_v<typename T::Desc>;

        // 임포트 결과 열거자
        typename T::EResult;
        std::is_enum_v<typename T::EResult>;
        T::EResult::Success;

        // Import 함수 조건
        std::is_same_v<Vector<typename T::EResult>, Vector<decltype(T::ImportMultiple(payload))> >;
    };


    // @warning Asset Loader에 의해 할당되는 모든 에셋들은 Asset Memory Category에 대해 할당된다고 가정합니다.
    template<typename T>
    concept AssetLoaderTrait = requires(const AssetLoadPayload<T>& payload)
    {
        // 로더의 이름 (권장 사항: static constexpr)
        std::is_same_v<std::string_view, decltype(T::kName)>;

        // 로더의 버전 (권장 사항: 호환되는 임포터의 버전과 동일)
        std::is_same_v<uint64, decltype(T::kVersion)>;

        // 로더 지정 임포트 설명자
        typename T::Desc;
        std::is_class_v<typename T::Desc>;

        // 로더 결과 열거자
        typename T::EResult;
        std::is_enum_v<typename T::EResult>;
        T::EResult::Success;

        // 로드 함수 조건
        std::is_same_v<decltype(T::Load(payload)), std::expected<class Asset*, typename T::EResult> >;
    };
}
