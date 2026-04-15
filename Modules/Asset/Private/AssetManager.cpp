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
#include <Mon3tr/Asset/AssetManager.hpp>

M3_DEFINE_LOG_CATEGORY(AssetManager);

namespace mon3tr::asset {
    AssetHandle::AssetHandle(AssetManager& assetManager, const Handle<Asset*> rawHandle) : assetManager_(&assetManager)
                                                                                         , handle_(rawHandle) {
        M3_ASSERT(assetManager_ != nullptr);
        M3_ASSERT(!handle_.IsNull());
    }

    AssetHandle::AssetHandle(const AssetHandle& other) : assetManager_(other.assetManager_)
                                                       , handle_(other.handle_) {
        Ref();
    }

    AssetHandle& AssetHandle::operator=(const AssetHandle& rhs) {
        assetManager_ = rhs.assetManager_;
        handle_ = rhs.handle_;

        Ref();

        return *this;
    }

    AssetHandle::AssetHandle(AssetHandle&& other) noexcept : assetManager_(std::exchange(other.assetManager_, nullptr))
                                                           , handle_(std::exchange(other.handle_, Handle<Asset*>{})) {
    }

    AssetHandle& AssetHandle::operator=(AssetHandle&& rhs) noexcept {
        assetManager_ = std::exchange(rhs.assetManager_, nullptr);
        handle_ = std::exchange(rhs.handle_, Handle<Asset*>{});
        return *this;
    }

    AssetHandle::~AssetHandle() {
        Destruct();
    }

    const Asset* AssetHandle::Get() const {
        if (assetManager_ == nullptr) {
            return nullptr;
        }

        if (handle_.IsNull()) {
            return nullptr;
        }

        return assetManager_->Lookup(handle_);
    }

    void AssetHandle::Destruct() {
        Unref();
        handle_ = Handle<Asset*>{};
        assetManager_ = nullptr;
    }

    void AssetHandle::Ref() {
        if (assetManager_ != nullptr && !handle_.IsNull()) {
            assetManager_->Ref(handle_);
        }
    }

    void AssetHandle::Unref() {
        if (assetManager_ != nullptr && !handle_.IsNull()) {
            assetManager_->Unref(handle_);
        }
    }

    AssetManager::AssetManager() {
        MarkAsInitialized();
    }

    void AssetManager::RunGarbageCollect() {
        // 메인 스레드에서만 실행되지만, Garbage Collect가 실행되는 와중에 다른 스레드에서 Unref를 시도하지 않는다는 보장을 할 수 없기에
        // 현재 Garbage Buffer에 대해서는 lock을 걸어준다. 다만, garbage collect counter를 증가시키는건
        // RunGarbageCollect 에서만 수행 되고, 지금 시점에서 사용되는 Garbage Buffer에 대해서만 Unref가 시도되기 때문에
        // 다음 Garbage Buffer에 대해서는 별도의 lock 없이 enqueue 작업이 가능하다.
        {
            const auto       garbageCollectBeginTime = std::chrono::high_resolution_clock::now();
            GarbageBuffer&   currentGarbageBuffer = GetCurrentGarbageBuffer();
            GarbageBuffer&   nextGarbageBuffer = GetNextGarbageBuffer();
            std::unique_lock currentGarbageBufferLock{currentGarbageBuffer.Mutex};
            finalPhasedGarbageBuffer_.reserve(currentGarbageBuffer.Buffer.size());
            while (!currentGarbageBuffer.Buffer.empty()) {
                const Garbage& garbage = currentGarbageBuffer.Buffer.front();
                const Asset*   asset = Lookup(garbage.Target);
                M3_ASSERT(asset != nullptr);
                const auto deltaTime = garbageCollectBeginTime - garbage.QueuedTime;
                if (asset->refCounter_ == 0 && (deltaTime >= garbageLifetime_)) {
                    finalPhasedGarbageBuffer_.emplace_back(FinalPhaseGarbage{.Target = const_cast<Asset*>(asset), .Handle = garbage.Target});
                } else {
                    nextGarbageBuffer.Buffer.push(garbage);
                }
                currentGarbageBuffer.Buffer.pop();
            }
        }

        handleManagerMutex_.lock();
        assetTableMutex_.lock();
        for (const FinalPhaseGarbage& finalPhaseGarbage: finalPhasedGarbageBuffer_) {
            handleManager_.Destroy(finalPhaseGarbage.Handle);
            assetTable_.erase(finalPhaseGarbage.Target->GetGuid());
        }
        assetTableMutex_.unlock();
        handleManagerMutex_.unlock();

        for (const FinalPhaseGarbage& finalPhaseGarbage: finalPhasedGarbageBuffer_) {
            Destroy(finalPhaseGarbage.Target);
        }
        finalPhasedGarbageBuffer_.clear();

        ++garbageCollectCounter_;
    }

    void AssetManager::Shutdown() {
        for (const auto& [guid, asset] : assetTable_) {
            Destroy<Asset, M3_MEM_CATEGORY(Asset)>(*handleManager_.GetMutable(asset));
            handleManager_.Destroy(asset);
        }
        assetTable_.clear();

        for (GarbageBuffer& garbageBuffer : garbageBuffers_) {
            garbageBuffer.Buffer.get_container().clear();
        }
        finalPhasedGarbageBuffer_.clear();

        System::Shutdown();
    }

    const Asset* AssetManager::Lookup(const Handle<Asset*> handle, bool bShouldIgnoreZeroRefCount) const {
        if (handle.IsNull()) {
            return nullptr;
        }

        const Asset* asset = nullptr;
        {
            std::shared_lock     lock{handleManagerMutex_};
            const Asset* const * assetPtr = handleManager_.Get(handle);
            if (assetPtr == nullptr) {
                return nullptr;
            }
            asset = *assetPtr;
        }

        M3_ASSERT(asset != nullptr);
        if (bShouldIgnoreZeroRefCount && asset->refCounter_ == 0) {
            return nullptr;
        }

        return asset;
    }

    void AssetManager::Ref(const Handle<Asset*> handle) {
        Asset* asset = const_cast<Asset*>(Lookup(handle, false));
        if (asset == nullptr) {
            return;
        }

        ++asset->refCounter_;
    }

    void AssetManager::Unref(const Handle<Asset*> handle) {
        Asset* asset = const_cast<Asset*>(Lookup(handle));
        if (asset == nullptr) {
            return;
        }

        const uint64 refCounterBeforeDec = asset->refCounter_.fetch_sub(1);
        if (refCounterBeforeDec > 1) {
            return;
        }

        const auto       currentTime = std::chrono::high_resolution_clock::now();
        GarbageBuffer&   garbageBuffer = GetCurrentGarbageBuffer();
        std::unique_lock garbageBufferLock{garbageBuffer.Mutex};
        garbageBuffer.Buffer.push(Garbage{.Target = handle, .QueuedTime = currentTime});
    }
}
