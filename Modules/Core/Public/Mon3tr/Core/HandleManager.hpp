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
#include <Mon3tr/Core/VirtualArray.hpp>

namespace mon3tr {
    namespace internal {
        struct SparseSlotLayout {
        public:
            static uint32 GetDenseSlotIdx(const uint64 data) {
                return static_cast<uint32>(data & kDenseSlotIdxMask);
            }

            static void SetDenseSlotIdx(uint64& data, const uint32 idx) {
                data = (data & (~kDenseSlotIdxMask)) | static_cast<uint64>(idx);
            }

            static uint32 GetNextFreeListNodeIdx(const uint64 data) {
                return GetDenseSlotIdx(data);
            }

            static void SetNextFreeListNodeIdx(uint64& data, const uint32 idx) {
                SetDenseSlotIdx(data, idx);
            }

            static uint32 GetVersion(const uint64 data) {
                return static_cast<uint32>((data & kVersionMask) >> kVersionOffset);
            }

            static void SetVersion(uint64& data, const uint32 version) {
                data = (data & (~kVersionMask)) | (static_cast<uint64>(0x7FFFFFFFui32 & version) << kVersionOffset);
            }

            static bool IsUsedSlot(const uint64 data) {
                return data & kIsUsedSlotFlagMask;
            }

            static void SetIsUsedSlot(uint64& data, const bool bIsUsedSlot) {
                data = (data & (~kIsUsedSlotFlagMask)) | (static_cast<uint64>(bIsUsedSlot) << kIsUsedFlagOffset);
            }

        private:
            static constexpr uint64 kDenseSlotIdxOffset = 0;
            static constexpr uint64 kDenseSlotIdxMask = (0xFFFFFFFFui64); // If, IsUsedSlot == 1, [0, 31] ~ 32 bits

            static constexpr uint64 kNextFreeListNodeIdxOffset = 0;
            static constexpr uint64 kNextFreeListNodeIdxMask = kDenseSlotIdxMask; // If, IsUsedSlot == 0, [0, 31] ~ 32 bits

            static constexpr uint64 kVersionOffset = 32;
            static constexpr uint64 kVersionMask = (0x7FFFFFFFui64 << 32); // [32, 62] ~ 31 bits

            static constexpr uint64 kIsUsedFlagOffset = 63;
            static constexpr uint64 kIsUsedSlotFlagMask = (1ui64 << 63); // [63, 63] ~ 1 bit

            static_assert((~(kVersionMask | kIsUsedSlotFlagMask)) == kDenseSlotIdxMask);
            static_assert((kVersionMask | kIsUsedSlotFlagMask | kDenseSlotIdxMask) == 0xFFFFFFFFFFFFFFFFui64);
        };

        struct HandleLayout {
        public:
            static uint32 GetSparseSlotIdx(const uint64 data) {
                return static_cast<uint32>(data & kSparseSlotIdxMask);
            }

            static void SetSparseSlotIdx(uint64& data, const uint32 idx) {
                data = (data & (~kSparseSlotIdxMask)) | (static_cast<uint64>(idx) << kSparseSlotIdxOffset);
            }

            static uint32 GetVersion(const uint64 data) {
                return static_cast<uint32>((data & kVersionMask) >> kVersionOffset);
            }

            static void SetVersion(uint64& data, const uint32 version) {
                data = (data & (~kVersionMask)) | (static_cast<uint64>(version) << kVersionOffset);
            }

        private:
            static constexpr uint64 kSparseSlotIdxOffset = 0;
            static constexpr uint64 kSparseSlotIdxMask = (0xFFFFFFFFui64);

            static constexpr uint64 kVersionOffset = 32;
            static constexpr uint64 kVersionMask = (0x7FFFFFFFui64 << 32);
        };
    }

    template<typename T>
    class HandleManager final {
    public:
        explicit HandleManager(const uint32 numInitHandleSlot = 2048ui32, const uint32 numMaxElementsCapacity = 16384ui32) : dense_{numMaxElementsCapacity} {
            sparse_.resize(numInitHandleSlot);
            FillSparseSlotFreeList(0, numInitHandleSlot - 1);
            sparseFreeListNodeHead_ = 0;
        }

        ~HandleManager() {
#if defined(DEBUG) || defined(_DEBUG)
            if (!dense_.IsEmpty()) {
                //spdlog::critical("Handle leaking founds: {}", dense_.GetSize());
                for (const auto& callStack: createCallStacks_) {
                    for (const auto& entry: callStack) {
                        //spdlog::critical("src: {}\n line: {}\n description: {}\n", entry.source_file(), entry.source_line(), entry.description());
                    }
                }
                M3_ASSERT(false);
            }
#endif
        }

        template<typename... Args>
        Handle<T> Create(Args&&... args) {
            if (sparseFreeListNodeHead_ == kInvalidFreeListNodeIdx) {
                GrowSparseSlots();
            }

            const bool bDenseAllocSuccess = dense_.EmplaceBack(std::forward<Args>(args)...);
            if (!bDenseAllocSuccess) {
                return Handle<T>{};
            }

            const uint32 targetSparseSlotIdx = sparseFreeListNodeHead_;
            M3_ASSERT(targetSparseSlotIdx < sparse_.size());
            uint64& targetSparseSlot = sparse_[targetSparseSlotIdx];
            M3_ASSERT(!internal::SparseSlotLayout::IsUsedSlot(targetSparseSlot));

            const uint32 newVersion = internal::SparseSlotLayout::GetVersion(targetSparseSlot) + 1;
            internal::SparseSlotLayout::SetVersion(targetSparseSlot, newVersion);
            internal::SparseSlotLayout::SetIsUsedSlot(targetSparseSlot, true);
            sparseFreeListNodeHead_ = internal::SparseSlotLayout::GetNextFreeListNodeIdx(targetSparseSlot);
            internal::SparseSlotLayout::SetDenseSlotIdx(targetSparseSlot, dense_.GetSize() - 1);
            derefToSparse_.emplace_back(targetSparseSlotIdx);

#if defined(DEBUG) || defined(_DEBUG)
            createCallStacks_.emplace_back(std::stacktrace::current());
#endif

            uint64 newRawHandle = 0;
            internal::HandleLayout::SetSparseSlotIdx(newRawHandle, targetSparseSlotIdx);
            internal::HandleLayout::SetVersion(newRawHandle, newVersion);
            return Handle<T>{newRawHandle};
        }

        void Destroy(const Handle<T> handle) {
            if (handle.IsNull()) {
                return;
            }

            const uint32 targetSparseSlotIdx = internal::HandleLayout::GetSparseSlotIdx(handle.Raw);
            if (targetSparseSlotIdx >= sparse_.size()) {
                return;
            }

            uint64& targetSparseSlot = sparse_[targetSparseSlotIdx];
            if (!internal::SparseSlotLayout::IsUsedSlot(targetSparseSlot) ||
                internal::HandleLayout::GetVersion(handle.Raw) != internal::SparseSlotLayout::GetVersion(targetSparseSlot)) {
                return;
            }

            const uint32 targetDenseSlotIdx = internal::SparseSlotLayout::GetDenseSlotIdx(targetSparseSlot);
            M3_ASSERT(targetDenseSlotIdx < dense_.GetSize());
            M3_ASSERT(derefToSparse_[targetDenseSlotIdx] != kInvalidSparseSlotIdx);
            if (dense_.GetSize() > 1) {
                const uint32 lastDenseElementIdx = static_cast<uint32>(dense_.GetSize()) - 1;
                std::swap(dense_[targetDenseSlotIdx], dense_[lastDenseElementIdx]);
#if defined(DEBUG) || defined(_DEBUG)
                std::swap(createCallStacks_[targetDenseSlotIdx], createCallStacks_[lastDenseElementIdx]);
#endif
                const uint32 swappedElementSparseSlotIdx = derefToSparse_[lastDenseElementIdx];
                M3_ASSERT(swappedElementSparseSlotIdx != kInvalidSparseSlotIdx && swappedElementSparseSlotIdx < sparse_.size());
                uint64& swappedElementSparseSlot = sparse_[swappedElementSparseSlotIdx];
                M3_ASSERT(internal::SparseSlotLayout::IsUsedSlot(swappedElementSparseSlot));
                M3_ASSERT(internal::SparseSlotLayout::GetDenseSlotIdx(swappedElementSparseSlot) == lastDenseElementIdx);
                internal::SparseSlotLayout::SetDenseSlotIdx(swappedElementSparseSlot, targetDenseSlotIdx);
                derefToSparse_[targetDenseSlotIdx] = swappedElementSparseSlotIdx;
            }

            internal::SparseSlotLayout::SetIsUsedSlot(targetSparseSlot, false);
            internal::SparseSlotLayout::SetNextFreeListNodeIdx(targetSparseSlot, sparseFreeListNodeHead_);
            sparseFreeListNodeHead_ = targetSparseSlotIdx;

            dense_.PopBack();
            derefToSparse_.pop_back();
            M3_POST_COND(dense_.GetSize() == derefToSparse_.size());

#if defined(DEBUG) || defined(_DEBUG)
            createCallStacks_.pop_back();
            M3_POST_COND(dense_.GetSize() == createCallStacks_.size());
#endif
        }

        const T* Get(const Handle<T> handle) const {
            if (handle.IsNull()) {
                return nullptr;
            }

            const uint32 sparseSlotIdx = internal::HandleLayout::GetSparseSlotIdx(handle.Raw);
            if (sparseSlotIdx >= sparse_.size()) {
                return nullptr;
            }

            const uint64& sparseSlot = sparse_[sparseSlotIdx];
            if (!internal::SparseSlotLayout::IsUsedSlot(sparseSlot) ||
                internal::HandleLayout::GetVersion(handle.Raw) != internal::SparseSlotLayout::GetVersion(sparseSlot)) {
                return nullptr;
            }

            const uint32 denseSlotIdx = internal::SparseSlotLayout::GetDenseSlotIdx(sparseSlot);
            M3_ASSERT(denseSlotIdx < dense_.GetSize());
            M3_ASSERT(derefToSparse_[denseSlotIdx] != kInvalidSparseSlotIdx);
            return &dense_[denseSlotIdx];
        }

        T* GetMutable(const Handle<T> handle) {
            return const_cast<T*>(Get(handle));
        }

    private:
        void GrowSparseSlots() {
            M3_PRE_COND(sparseFreeListNodeHead_ == kInvalidFreeListNodeIdx);

            const uint64 oldSparseSlotSize = sparse_.size();
            sparse_.resize(oldSparseSlotSize + (oldSparseSlotSize / 2));
            FillSparseSlotFreeList(oldSparseSlotSize, sparse_.size() - 1);

            sparseFreeListNodeHead_ = oldSparseSlotSize;
        }

        void FillSparseSlotFreeList(const uint64 beginSparseSlotIdx, const uint64 endSparseSlotIdx) {
            M3_PRE_COND(beginSparseSlotIdx <= endSparseSlotIdx);
            M3_PRE_COND(beginSparseSlotIdx < sparse_.size());

            for (uint64 idx = beginSparseSlotIdx; idx <= endSparseSlotIdx; ++idx) {
                internal::SparseSlotLayout::SetNextFreeListNodeIdx(sparse_[idx], (idx == endSparseSlotIdx) ? kInvalidFreeListNodeIdx : idx + 1);
                internal::SparseSlotLayout::SetVersion(sparse_[idx], 0);
                internal::SparseSlotLayout::SetIsUsedSlot(sparse_[idx], false);
            }
        }

    private:
        constexpr static uint32 kInvalidFreeListNodeIdx = std::numeric_limits<uint32>::max();
        constexpr static uint32 kInvalidSparseSlotIdx = std::numeric_limits<uint32>::max();

        std::vector<uint64> sparse_;
        std::vector<uint32> derefToSparse_;
        VirtualArray<T>     dense_;

        uint32 sparseFreeListNodeHead_ = kInvalidFreeListNodeIdx;

#if defined(DEBUG) || defined(_DEBUG)
        std::vector<std::stacktrace> createCallStacks_;
#endif
    };
}
