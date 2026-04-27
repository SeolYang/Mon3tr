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
#include <Mon3tr/Asset/AssetUtils.hpp>

namespace mon3tr::asset {
    class Asset;

    enum EAssetType {
        Text,
        AudioClip,
        StaticMesh,
        SkeletalMesh,
        Shader
    };

    struct AssetImportDesc {
        fs::path RawFilePath{};

        // 한번 Import 되면 에셋에 GUID가 부여되고 에셋을 사람이 쉽게 인식하기 위한 라벨이 붙혀진다.
        // 기본적으로 라벨은 단순히 사람이 쉽게 보기 위한 목적만을 가지며 해당 에셋을 대표하지는 않는다.
        // (라벨은 중복 가능)
        fs::path Label{};
    };

    template<typename Importer>
    struct AssetImportPayload {
        const AssetImportDesc& ImportDesc;
        const Importer::Desc&  ImporterSpecificDesc;

        nlohmann::json& MetadataRoot;

        const fs::path& AssetBinaryPath;
    };

    template<typename Importer>
    struct AssetMultipleImportPayload {
    public:
        const AssetImportDesc& ImportDesc;
        const Importer::Desc&  ImporterSpecificDesc;

        Vector<nlohmann::json>& MetadataRoots;
        xg::Guid PlaceholderGuid{};
        Vector<fs::path>& BinaryPlaceholderPaths;

    public:
        // Importer 내부에서 에셋의 총 개수를 확정짓는데 사용되어야함.
        void SetNumAssets(const uint64 numAssets) {
            M3_ASSERT(PlaceholderGuid.isValid());
            M3_ASSERT(numAssets > 0);

            MetadataRoots.resize(numAssets);
            BinaryPlaceholderPaths.resize(numAssets);
            for (uint64 idx = 0; idx < numAssets; ++idx) {
                BinaryPlaceholderPaths[idx] = CreateAssetBinaryPlaceholderPath(PlaceholderGuid, idx);
            }
        }

    };

    struct AssetLoadDesc {
        Guid AssetGuid{};
    };

    template<typename Loader>
    struct AssetLoadPayload {
        const AssetLoadDesc& LoadDesc;
        const Loader::Desc&  LoaderSpecificDesc;

        const fs::path& AssetBinaryPath;
        const fs::path& Label;

        const nlohmann::json& MetadataRoot;
    };
}
