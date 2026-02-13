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
#include <Mon3tr/Core/VirtualMemory.hpp>

namespace mon3tr {
    template<typename T>
    class VirtualArray {
    public:
        /*
         * Max Capacity used to calculate size of reserved virtual memory space for vector.
         * If size of elements(which referred to GetSizeBytes()) exceed size of reserved virtual memory bytes, it must fail allocation.
         */
        explicit VirtualArray(const uint64 maxCapacity) {
            sizeOfReservedMemBytes_ = AlignUp(sizeof(T) * maxCapacity, Page::GetSize());
            data_ = static_cast<T*>(VirtualMemory::Reserve(sizeOfReservedMemBytes_));
            maxCapacity_ = maxCapacity;
        }

        VirtualArray(const VirtualArray& other) {
            sizeOfReservedMemBytes_ = other.sizeOfReservedMemBytes_;
            data_ = static_cast<T*>(VirtualMemory::Reserve(sizeOfReservedMemBytes_));
            maxCapacity_ = other.maxCapacity_;
            sizeOfCommitedMemBytes_ = other.sizeOfCommitedMemBytes_;
            VirtualMemory::Commit(data_, sizeOfCommitedMemBytes_);
            numElements_ = other.numElements_;

            for (uint64 idx = 0; idx < numElements_; ++idx) {
                std::construct_at(&data_[idx], other.data_[idx]);
            }
        }

        VirtualArray(VirtualArray&& other) noexcept {
            sizeOfReservedMemBytes_ = std::exchange(other.sizeOfReservedMemBytes_, 0);
            data_ = std::exchange(other.data_, nullptr);
            maxCapacity_ = std::exchange(other.maxCapacity_, 0);
            sizeOfCommitedMemBytes_ = std::exchange(other.sizeOfCommitedMemBytes_, 0);
            numElements_ = std::exchange(other.numElements_, 0);
        }

        ~VirtualArray() {
            Clear();
            VirtualMemory::Free(data_);
        }

        VirtualArray& operator=(const VirtualArray& other) {
            if (this == &other) {
                return *this;
            }

            void* newMemPtr = VirtualMemory::Reserve(other.sizeOfReservedMemBytes_);
            if (newMemPtr == nullptr) {
                return *this;
            }

            const bool bNewMemPtrCommitSuccess = VirtualMemory::Commit(newMemPtr, other.sizeOfCommitedMemBytes_);
            if (!bNewMemPtrCommitSuccess) {
                VirtualMemory::Free(newMemPtr);
                return *this;
            }

            Clear();
            VirtualMemory::Free(data_);

            sizeOfReservedMemBytes_ = other.sizeOfReservedMemBytes_;
            data_ = static_cast<T*>(newMemPtr);
            maxCapacity_ = other.maxCapacity_;
            sizeOfCommitedMemBytes_ = other.sizeOfCommitedMemBytes_;
            numElements_ = other.numElements_;

            for (uint64 idx = 0; idx < numElements_; ++idx) {
                std::construct_at(&data_[idx], other.data_[idx]);
            }

            return *this;
        }

        VirtualArray& operator=(VirtualArray&& other) noexcept(false) {
            if (this == &other) {
                return *this;
            }
            Clear();
            VirtualMemory::Free(data_);

            sizeOfReservedMemBytes_ = std::exchange(other.sizeOfReservedMemBytes_, 0);
            data_ = std::exchange(other.data_, nullptr);
            maxCapacity_ = std::exchange(other.maxCapacity_, 0);
            sizeOfCommitedMemBytes_ = std::exchange(other.sizeOfCommitedMemBytes_, 0);
            numElements_ = std::exchange(other.numElements_, 0);
            return *this;
        }

        [[nodiscard]] const T& operator[](const uint64 idx) const { return At(idx); }
        [[nodiscard]] T&       operator[](const uint64 idx) { return At(idx); }

        [[nodiscard]] const T& At(const uint64 idx) const {
            M3_PRE_COND(idx < numElements_);
            return data_[idx];
        }

        [[nodiscard]] T& At(const uint64 idx) {
            return const_cast<T&>(static_cast<const VirtualArray&>(*this).At(idx));
        }

        [[nodiscard]] const T& GetLastElement() const { return At(numElements_ - 1); }
        [[nodiscard]] T&       GetLastElement() { return At(numElements_ - 1); }

        template<typename... Args>
        bool EmplaceBack(Args&&... args) {
            M3_PRE_COND((sizeOfCommitedMemBytes_ % Page::GetSize()) == 0);
            const uint64 requiredSizeBytes = sizeof(T) * (numElements_ + 1);
            if (requiredSizeBytes >= sizeOfCommitedMemBytes_) {
                const uint64 targetCommitMemBytes = numElements_ == 0
                                                        ? AlignUp(16ui64 * sizeof(T), Page::GetSize())
                                                        : AlignUp(((numElements_ + 1) / 2) * sizeof(T), Page::GetSize());
                M3_ASSERT(targetCommitMemBytes > 0);
                M3_ASSERT(targetCommitMemBytes <= sizeOfReservedMemBytes_);

                if ((sizeOfCommitedMemBytes_ + targetCommitMemBytes) > sizeOfReservedMemBytes_) {
                    return false;
                }

                const bool bCommitSuccess = VirtualMemory::Commit(reinterpret_cast<uint8*>(data_) + sizeOfCommitedMemBytes_, targetCommitMemBytes);
                if (!bCommitSuccess) {
                    return false;
                }
                sizeOfCommitedMemBytes_ += targetCommitMemBytes;
                M3_ASSERT((sizeOfCommitedMemBytes_ % Page::GetSize()) == 0);
            }

            std::construct_at(&data_[numElements_], std::forward<Args>(args)...);
            ++numElements_;
            return true;
        }

        void PopBack() {
            M3_PRE_COND(numElements_ > 0);
            --numElements_;
            data_[numElements_].~T();
        }

        void Clear() {
            for (uint64 idx = 0; idx < numElements_; ++idx) {
                data_[idx].~T();
            }
            numElements_ = 0;
        }

        [[nodiscard]] bool   IsEmpty() const noexcept { return numElements_ == 0; }
        [[nodiscard]] uint64 GetSize() const noexcept { return numElements_; }
        [[nodiscard]] uint64 GetSizeBytes() const noexcept { return numElements_ * sizeof(T); }
        [[nodiscard]] uint64 GetMaxCapacity() const noexcept { return maxCapacity_; }
        [[nodiscard]] uint64 GetSizeReservedVirtualMemoryBytes() const noexcept { return sizeOfReservedMemBytes_; }

    private:
        T*     data_ = nullptr;
        uint64 maxCapacity_ = 0;
        uint64 sizeOfCommitedMemBytes_ = 0;
        uint64 sizeOfReservedMemBytes_ = 0;
        uint64 numElements_ = 0;
    };
}
