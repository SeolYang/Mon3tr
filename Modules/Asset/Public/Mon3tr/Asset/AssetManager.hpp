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

M3_DECLARE_LOG_CATEGORY(AssetManager);

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
        AssetBinaryDoesNotExistAfterImport,
    };

    enum class EAssetLoadResult {
        Success,
        InvalidAssetGuid,
        AssetDoesNotExist,
        AssetMetadataDoesNotExist,
        FailedToOpenAssetMetadata,
        EmptyAssetMetadata,
        AssetMetadataValidationFailed,
        LoaderFailure,
    };

    struct AssetImportDesc {
        fs::path RawFilePath;

        // 한번 Import 되면 에셋에 GUID가 부여되고 에셋을 사람이 쉽게 인식하기 위한 라벨이 붙혀진다.
        // 기본적으로 라벨은 단순히 사람이 쉽게 보기 위한 목적만을 가지며 해당 에셋을 대표하지는 않는다.
        fs::path Label;
    };

    template<typename Importer>
    struct AssetImportPayload {
        const AssetImportDesc* ImportDesc = nullptr;
        const Importer::Desc*  ImporterSpecificDesc = nullptr;

        nlohmann::json* MetadataRoot = nullptr;

        fs::path AssetBinaryPath;
    };

    struct AssetLoadDesc {
        Guid AssetGuid;
    };

    template<typename Loader>
    struct AssetLoadPayload {
        const AssetLoadDesc* LoadDesc = nullptr;
        const Loader::Desc*  LoaderSpecificDesc = nullptr;

        fs::path AssetBinaryPath;

        const nlohmann::json* MetadataRoot = nullptr;
    };

    class AssetManager;
    // cast_to<T> -> dynamic cast for asset! 에셋 타입 당 하나의 에셋 타입!
    // T::kAssetType == Asset* asset->GetType() -> Valid Down Casting!
    // smart handle for asset!
    class AssetHandle {
    public:
        AssetHandle() = default;

        AssetHandle(AssetManager& assetManager, const Handle<Asset*> rawHandle);

        // 만약 핸들이 유효하다면, 핸들을 복사하고 해당 핸들에 해당하는 에셋의 레퍼런스 카운트를 증가시킨다. (AssetManager::Ref)
        AssetHandle(const AssetHandle& other);

        AssetHandle& operator=(const AssetHandle& rhs);

        // 핸들의 유/무효와 관계없이 핸들을 이동시킨다. 레퍼런스 카운트에 아무런 영향을 주지 않는다. 이동된 에셋 핸들(rhs)는 이동후 무효화 된다.
        AssetHandle(AssetHandle&& other) noexcept;

        AssetHandle& operator=(AssetHandle&& rhs) noexcept;

        // 핸들이 유효하다면 레퍼런스 카운트를 감소 시킨다. (AssetManager::Unref)
        ~AssetHandle();

        const Asset* Get() const;

        template<typename T>
        const T* Cast() const {
            const Asset* asset = Get();
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

        // 수동으로 에셋 핸들을 무효화 시킨다. 강제로 Unref 호출을 발생시키며, 호출 당사자 에셋 핸들은 무효화된다.
        void Destruct();

    private:
        void Ref();

        void Unref();

    private:
        AssetManager*  assetManager_ = nullptr;
        Handle<Asset*> handle_{};
    };

    class AssetManager : public System {
        friend class AssetHandle;

        struct Garbage {
            Handle<Asset*>                                 Target{};
            std::chrono::high_resolution_clock::time_point QueuedTime{};
        };

        struct GarbageBuffer {
            Queue<Garbage, M3_MEM_CATEGORY(Asset)> Buffer{};
            mutable std::mutex                     Mutex{};
        };

        struct FinalPhaseGarbage {
            Asset*         Target = nullptr;
            Handle<Asset*> Handle{};
        };

    public:
        // 현재로서는 AssetManager는 별도 초기화가 필요없는 독립적인 System이다. 다만, 여전히 Shutdown을 필요로 하다.
        AssetManager();

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
        EAssetImportResult Import(const AssetImportDesc& assetImportDesc, const Importer::Desc& importerDesc) {
            if (assetImportDesc.RawFilePath.empty()) {
                return EAssetImportResult::EmptyRawFilePath;
            }

            if (!fs::exists(assetImportDesc.RawFilePath)) {
                return EAssetImportResult::RawFileDoesNotExist;
            }

            const Guid newGuid = xg::newGuid();

            const fs::path newAssetBinaryPath = CreateBinaryPath(newGuid);
            nlohmann::json metadataRoot{};
            const auto     importResult = Importer::Import(AssetImportPayload<Importer>{
                .ImportDesc = &assetImportDesc,
                .ImporterSpecificDesc = &importerDesc,
                .MetadataRoot = &metadataRoot,
                .AssetBinaryPath = newAssetBinaryPath
            });
            if (importResult != decltype(importResult)::Success) {
                M3_LOG(AssetManager, Error, "[{}] Failed to import asset {}. => {}", Importer::kName, assetImportDesc.RawFilePath, importResult);
                fs::remove(newAssetBinaryPath);
                return EAssetImportResult::ImporterFailure;
            } else if (!fs::exists(newAssetBinaryPath)) {
                return EAssetImportResult::AssetBinaryDoesNotExistAfterImport;
            }

            // record general asset metadata
            nlohmann::json generalMetadata;
            generalMetadata[kLabelMetadataJsonKey] = assetImportDesc.Label;
            generalMetadata[kVersionMetadataJsonKey] = Importer::kVersion;
            metadataRoot[kGeneralMetadataJsonKey] = generalMetadata;

            const fs::path newAssetMetadataPath = CreateMetadataPath(newGuid);
            std::ofstream  assetMetadataStream{newAssetMetadataPath, std::ios::out | std::ios::trunc};
            assetMetadataStream << metadataRoot.dump();
            assetMetadataStream.close();

            return EAssetImportResult::Success;
        }

        template<typename Loader>
        std::expected<AssetHandle, EAssetLoadResult> Load(const AssetLoadDesc& assetLoadDesc, const Loader::Desc& loaderDesc) {
            //! Asset Table shared lock
            {
                std::shared_lock assetTableLock{assetTableMutex_};
                if (const auto foundItr = assetTable_.find(assetLoadDesc.AssetGuid);
                    foundItr != assetTable_.end()) {
                    return AssetHandle{*this, foundItr->second};
                }
            }
            //! Asset Table shared unlock

            const fs::path metadataPath = CreateMetadataPath(assetLoadDesc.AssetGuid);
            if (!fs::exists(metadataPath)) {
                return std::unexpected{EAssetLoadResult::AssetMetadataDoesNotExist};
            }
            std::ifstream metadataStream{metadataPath, std::ios::in};
            if (!metadataStream.is_open()) {
                return std::unexpected{EAssetLoadResult::FailedToOpenAssetMetadata};
            }
            nlohmann::json metadataRoot = nlohmann::json::parse(metadataStream, nullptr, false);
            metadataStream.close();

            const nlohmann::json generalMetadata = metadataRoot.value(kGeneralMetadataJsonKey, nlohmann::json{});
            const int64          version = generalMetadata.value(kVersionMetadataJsonKey, -1);
            fs::path             label = generalMetadata.value(kLabelMetadataJsonKey, "Unknown");
            if (version != Loader::kVersion) {
                M3_LOG(AssetManager, Warning, "[{}] Loader Versions mismatch with asset {}({}) metadata version. Expected: {}, Found: {}",
                       Loader::kName,
                       label, assetLoadDesc.AssetGuid,
                       Loader::kVersion, version);
            }

            // @todo async load는 어떻게 처리? Loader의 Load 부분만 따로 async? flecs와 유기적으로 연동가능한지?
            using ELoadResult = Loader::ELoadResult;
            static_assert(std::is_enum_v<ELoadResult>);
            std::expected<Asset*, ELoadResult> expectedAsset = Loader::Load(AssetLoadPayload{
                .LoadDesc = &assetLoadDesc,
                .LoaderSpecificDesc = &loaderDesc,
                .AssetPath = CreateBinaryPath(assetLoadDesc.AssetGuid),
                .MetadataRoot = &metadataRoot,
            });
            if (!expectedAsset.has_value()) {
                M3_LOG(AssetManager, Error, "[{}] Failed to load asset {}({}). Reason: {}",
                       Loader::kName,
                       label, assetLoadDesc.AssetGuid,
                       expectedAsset.error());
                return std::unexpected{EAssetLoadResult::LoaderFailure};
            }

            Asset* asset = expectedAsset.value();
            asset->guid_ = assetLoadDesc.AssetGuid;
            asset->label_ = std::move(label);

            //! Handle Manager lock
            handleManagerMutex_.lock();
            const Handle<Asset*> newRawHandle = handleManager_.Create(asset);
            handleManagerMutex_.unlock();
            //! Handle Manager unlock

            //! Asset Table lock
            assetTableMutex_.lock();
            assetTable_[assetLoadDesc.AssetGuid] = newRawHandle;
            assetTableMutex_.unlock();
            //! Asset Table unlock

            return AssetHandle{*this, newRawHandle};
        }

        // @todo Unload: AssetHandle에 의해서 Unload 로직이 모두 처리되므로 더이상 필요없지 않은가? 강제 Unload 필요? 또는 garbage를 강제로 해제?

        // 항상 메인 스레드에서만 실행되어야함!
        void RunGarbageCollect();

        [[nodiscard]] std::chrono::seconds GetGarbageLifetime() const noexcept {
            return garbageLifetime_;
        }

    private:
        // 에셋이 해당 핸들에 대해 유효하더라도, 에셋의 ref count가 0이면 null을 반환해야 한다.
        const Asset* Lookup(Handle<Asset*> handle, bool bShouldIgnoreZeroRefCount = true) const;

        // AssetHandle의 복사와 소멸(destruction)은 Ref/Unref를 호출함
        void Ref(Handle<Asset*> handle);

        void Unref(const Handle<Asset*> handle);

        [[nodiscard]] GarbageBuffer& GetCurrentGarbageBuffer() noexcept { return garbageBuffers_[garbageCollectCounter_ % kNumGarbageBuffer]; }
        [[nodiscard]] GarbageBuffer& GetNextGarbageBuffer() noexcept { return garbageBuffers_[(garbageCollectCounter_ + 1) % kNumGarbageBuffer]; }

        static fs::path CreateBinaryPath(const Guid& guid) {
            M3_ASSERT(guid.isValid());
            constexpr std::string_view kAssetBinaryPathFormat = "Assets\\{}.m3tr";
            return std::format(kAssetBinaryPathFormat, guid.str());
        }

        static fs::path CreateMetadataPath(const Guid& guid) {
            M3_ASSERT(guid.isValid());
            constexpr std::string_view kAssetMetadataPathFormat = "Assets\\{}.m3mt";
            return std::format(kAssetMetadataPathFormat, guid.str());
        }

    private:
        mutable std::shared_mutex                           assetTableMutex_;
        ankerl::unordered_dense::map<Guid, Handle<Asset*> > assetTable_;

        mutable std::shared_mutex handleManagerMutex_;
        HandleManager<Asset*>     handleManager_;

        constexpr static uint64                 kNumGarbageBuffer = 2;
        std::atomic_uint64_t                    garbageCollectCounter_ = 0;
        std::chrono::seconds                    garbageLifetime_ = std::chrono::seconds{60};
        Array<GarbageBuffer, kNumGarbageBuffer> garbageBuffers_{};
        Vector<FinalPhaseGarbage>               finalPhasedGarbageBuffer_{};

    private:
        static constexpr std::string_view kGeneralMetadataJsonKey = "General";
        static constexpr std::string_view kVersionMetadataJsonKey = "Version";
        static constexpr std::string_view kLabelMetadataJsonKey = "Label";
    };
}
