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
#include <Mon3tr/Core/HandleManager.hpp>
#include <Mon3tr/Asset/AssetMinimal.hpp>
#include <Mon3tr/Asset/Asset.hpp>

namespace mon3tr::asset {
    enum class EAssetManagerInitializeResult {
        Success,
    };

    enum class EAssetImportResult {
        Success,
        EmptyRawFilePath,
        RawFileDoesNotExist,
        InvalidLabel,
        ImporterFailure,
    };

    enum class EAssetLoadResult {
        Success,
        InvalidAssetGuid,
        AssetDoesNotExist,
        AssetMetadataDoesNotExist,
        AssetMetadataValidationFailed,
        LoaderFailure,
    };

    struct GeneralAssetImportDesc {
        fs::path RawFilePath;

        // 한번 Import 되면 에셋에 GUID가 부여되고 에셋을 사람이 쉽게 인식하기 위한 라벨이 붙혀진다.
        // 기본적으로 라벨은 단순히 사람이 쉽게 보기 위한 목적만을 가지며 해당 에셋을 대표하지는 않는다.
        fs::path Label;
    };

    struct GeneralAssetLoadDesc {
        Guid AssetGuid;
    };

    class AssetManager;
    // cast_to<T> -> dynamic cast for asset! 에셋 타입 당 하나의 에셋 타입!
    // T::kAssetType == Asset* asset->GetType() -> Valid Down Casting!
    // smart handle for asset!
    class AssetHandle {
    public:
        // @todo Impl Copy/Move Constructor & operator which increase ref count of asset
        // @todo Impl Destructor which decrease ref count of asset (call unload internally)

        template<typename T>
        const T* Cast() const {
            if (assetManager_ == nullptr) {
                return nullptr;
            }

            if (handle_.IsNull()) {
                return nullptr;
            }

            const Asset* asset = GetAssetFromManager();
            if (asset == nullptr) {
                return nullptr;
            }

            if (T::kAssetType != asset->GetType()) {
                return nullptr;
            }

            return static_cast<const T*>(asset);
        }

        template<typename T>
        T* Cast() {
            return const_cast<T*>(const_cast<const AssetHandle*>(this)->Cast<T>());
        }

        // 강제로 Unref 호출: 호출 이후 핸들은 무효화
        void Unload();

    private:
        const Asset* GetAssetFromManager() const;

    private:
        AssetManager* assetManager_ = nullptr;
        Handle<Asset> handle_;
    };

    class AssetManager : public System {
        friend class AssetHandle;

    public:
        AssetManager(const AssetManager&) = delete;

        AssetManager& operator=(const AssetManager&) = delete;

        AssetManager(AssetManager&&) = delete;

        AssetManager& operator=(AssetManager&&) = delete;

        // 임포터 규격: Import static function을 가지며, 해당 함수가 결과 enumerator를 반환함.
        // 해당 enum은 최소 Success를 가지고 있어야함.
        // 입력으로는 GeneralAssetImportDesc에 대한 const ref, 에셋을 쓸 수 있는 file ptr가 전달되야한다, 추가적으로 메타데이터를 기록한 json object ref도 전달되어야 한다.
        // 에셋 기본 규격
        // GENERATED_GUID.m3tr: Binary
        // GENERATED_GUID.m3mt: Metadata
        template<typename Importer>
        EAssetImportResult Import(const GeneralAssetImportDesc& generalImportDesc, const Importer::Desc& importerDesc) {
            if (generalImportDesc.RawFilePath.empty()) {
                return EAssetImportResult::EmptyRawFilePath;
            }

            if (!fs::exists(generalImportDesc.RawFilePath)) {
                return EAssetImportResult::RawFileDoesNotExist;
            }

            const Guid newGuid = xg::newGuid();

            constexpr std::string_view kAssetBinaryPathFormat = "Assets\\{}.m3tr";
            const fs::path             newAssetBinaryPath = std::format(kAssetBinaryPathFormat, newGuid.str());
            std::ofstream              assetFileStream{newAssetBinaryPath, std::ios::out | std::ios::binary | std::ios::trunc};
            nlohmann::json             metadataRoot{};
            const auto                 importResult = Importer::Import(generalImportDesc, importerDesc, metadataRoot, assetFileStream);
            if (importResult != decltype(importResult)::Success) {
                fs::remove(newAssetBinaryPath);
                return EAssetImportResult::ImporterFailure;
            }
            assetFileStream.close();

            // record general asset metadata
            nlohmann::json generalMetadata;
            generalMetadata["Guid"] = newGuid.str();
            generalMetadata["Label"] = generalImportDesc.Label;
            generalMetadata["Version"] = Importer::kVersion;
            metadataRoot["General"] = generalMetadata;

            constexpr std::string_view kAssetMetadataPathFormat = "Assets\\{}.m3mt";
            const fs::path             newAssetMetadataPath = std::format(kAssetMetadataPathFormat, newGuid.str());
            std::ofstream              assetMetadataStream{newAssetMetadataPath, std::ios::out | std::ios::trunc};
            assetMetadataStream << metadataRoot.dump();
            assetMetadataStream.close();

            return EAssetImportResult::Success;
        }

        template<typename Loader>
        std::expected<AssetHandle, EAssetLoadResult> Load(const GeneralAssetLoadDesc& generalDesc, const Loader::Desc& loaderDesc);

        // Unload: AssetHandle에 의해서 Unload 로직이 모두 처리되므로 더이상 필요없지 않은가?

    private:
        const Asset* Lookup(Handle<Asset> handle) const;

        // AssetHandle의 복사와 소멸(destruction)은 Ref/Unref를 호출함
        void Ref(const Handle<Asset> handle);

        void Unref(const Handle<Asset> handle);

    private:
        ankerl::unordered_dense::map<Guid, Handle<Asset> > assetTable_;

        mutable std::shared_mutex handleManagerMutex_;
        HandleManager<Asset*>     handleManager_;

        // Unload->If ref count == 0 -> treat as garbage -> queue_(N%2)->enqueue(garbage)
        // # at the end of current frame
        // if delta time(CurrentTime - QueuedTime) >= x
        //  if still ref count == 0 then remove Target!
        //  else ignore
        // else
        //   queue_(N+1%2)->enqueue(garbage)
        struct Garbage {
            Handle<Asset>                                  Target;
            std::chrono::high_resolution_clock::time_point QueuedTime;
        };

        Array<Queue<Garbage, M3_MEM_CATEGORY(Asset)>, 2> GarbageQueues{};
    };
}
