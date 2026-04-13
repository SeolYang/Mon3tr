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

namespace mon3tr::asset {
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
        fs::path Label{};
    };

    template<typename Importer>
    struct AssetImportPayload {
        const AssetImportDesc* ImportDesc = nullptr;
        const Importer::Desc*  ImporterSpecificDesc = nullptr;

        nlohmann::json* MetadataRoot = nullptr;

        fs::path AssetBinaryPath{};
    };

    struct AssetLoadDesc {
        Guid AssetGuid{};
    };

    template<typename Loader>
    struct AssetLoadPayload {
        const AssetLoadDesc* LoadDesc = nullptr;
        const Loader::Desc*  LoaderSpecificDesc = nullptr;

        fs::path AssetBinaryPath{};

        const nlohmann::json* MetadataRoot = nullptr;
    };
}
