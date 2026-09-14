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
#include <Mon3tr/Core/TwoLevelSegregatedFit.hpp>

namespace mon3tr {
    static uint64 CalculateMostSignificantBitIndex(const uint64 value) {
        return std::min<uint64>(64 - std::countl_zero(value) - 1, 63Ui64);
    }

    TwoLevelSegregatedFit::TwoLevelSegregatedFit(const uint64 initialBlockSize, const uint64 secondLevelIdx) : initialBlockSize_(initialBlockSize)
        , secondLevelIndex_(secondLevelIdx)
        , numSecondLevelSubdivisions_(1Ui64 << secondLevelIdx) {
        SetInitialBlockSize(initialBlockSize);

        Block* initialBlock = Create<Block>();
        initialBlock->Offset = 0;
        initialBlock->Size = initialBlockSize_;
        InsertToFreeList(initialBlock);
        physicalHead_ = initialBlock;
        physicalTail_ = initialBlock;
    }

    TwoLevelSegregatedFit::~TwoLevelSegregatedFit() {
        Block* currentBlock = physicalHead_;
        while (currentBlock != nullptr) {
            Block* nextBlock = currentBlock->PhysicalNext;
            Destroy<Block>(currentBlock);
            currentBlock = nextBlock;
        }
    }

    void TwoLevelSegregatedFit::SetInitialBlockSize(const uint64 size) {
        initialBlockSize_ = size;
        // 블럭 자신이 들어가는 구간을 정확하게 포함하기 위함.
        // ex. 16 바이트 -> msbi = 4 -> [1, 2), [2, 4), [4, 8), [8, 16) => 16바이트 포함 불가
        // 한 개의 구간 [16, 32)을 더 포함 함으로써 16바이트 블럭을 확실하게 포함.
        numFirstLevels_ = CalculateMostSignificantBitIndex(size) + 1;
        secondLevelBitmaps_.resize(numFirstLevels_);
        freeLists_.resize(numFirstLevels_ * numSecondLevelSubdivisions_);
    }

    TwoLevelSegregatedFit::MappedLevel TwoLevelSegregatedFit::Mapping(const uint64 size) {
        const uint64                  f = CalculateMostSignificantBitIndex(size);
        [[maybe_unused]] const uint64 frhs = 1Ui64 << f;
        return MappedLevel{
            .FirstLevel = f,
            // 할당 구간의 크기가 1바이트 미만으로 내려가는 경우 Second Level을 항상 0으로 맵핑한다.
            .SecondLevel = (secondLevelIndex_ <= f) ? (size ^ (1Ui64 << f)) >> (f - secondLevelIndex_) : 0
        };
    }

    void TwoLevelSegregatedFit::InsertToFreeList(Block* newBlock) {
        M3_ASSERT(newBlock != nullptr);
        M3_ASSERT(newBlock->Size > 0);
        M3_ASSERT(newBlock->bIsFree);
        M3_ASSERT(newBlock->FreeListPrev == nullptr && newBlock->FreeListNext == nullptr);

        const MappedLevel level = Mapping(newBlock->Size);
        const uint64      freeListIdx = level.GetFreeListIndex(numSecondLevelSubdivisions_);
        M3_ASSERT(freeListIdx < freeLists_.size());
        Block* prevFreeListHead = freeLists_[freeListIdx];
        if (prevFreeListHead != nullptr) {
            M3_ASSERT(prevFreeListHead->FreeListPrev == nullptr);
            prevFreeListHead->FreeListPrev = newBlock;
        }
        newBlock->FreeListNext = prevFreeListHead;
        freeLists_[freeListIdx] = newBlock;

        firstLevelBitmap_ |= 1Ui64 << level.FirstLevel;
        secondLevelBitmaps_[level.FirstLevel] |= 1Ui64 << level.SecondLevel;
    }

    void TwoLevelSegregatedFit::ExtractFromFreeList(Block* block) {
        M3_ASSERT(block != nullptr);
        M3_ASSERT(block->Size > 0);
        M3_ASSERT(block->bIsFree);

        if (block->FreeListPrev != nullptr) {
            block->FreeListPrev->FreeListNext = block->FreeListNext;
        }

        if (block->FreeListNext != nullptr) {
            block->FreeListNext->FreeListPrev = block->FreeListPrev;
        }

        const MappedLevel level = Mapping(block->Size);
        const uint64      freeListIdx = level.GetFreeListIndex(numSecondLevelSubdivisions_);
        M3_ASSERT(freeListIdx < freeLists_.size());
        M3_ASSERT(freeLists_[freeListIdx] != nullptr);
        if (freeLists_[freeListIdx] == block) {
            freeLists_[freeListIdx] = block->FreeListNext;
        }

        // 현재 Second Level에 더 이상 유효한 블럭이 없는 경우
        if (freeLists_[freeListIdx] == nullptr) {
            secondLevelBitmaps_[level.FirstLevel] &= ~(1Ui64 << level.SecondLevel);
        }

        // 현재 First Level에 속해있는 모든 하위 구간들에 블럭이 전혀 없는 경우
        if (secondLevelBitmaps_[level.FirstLevel] == 0) {
            firstLevelBitmap_ &= ~(1Ui64 << level.FirstLevel);
        }

        block->FreeListNext = nullptr;
        block->FreeListPrev = nullptr;
    }

    void TwoLevelSegregatedFit::MergeWithFreePhysicallyContinuousBlock(Block* freeBlock) {
        M3_ASSERT(freeBlock != nullptr);
        M3_ASSERT(freeBlock->bIsFree);
        M3_ASSERT(freeBlock->FreeListPrev == nullptr && freeBlock->FreeListNext == nullptr);

        // 전달 받은 블록에 대해 물리적으로 인접한 블록들이 현재 사용중이지 않다면 현재 블록과 합칠 것!
        if (freeBlock->PhysicalPrev != nullptr && freeBlock->PhysicalPrev->bIsFree) {
            freeBlock->Size += freeBlock->PhysicalPrev->Size;
            freeBlock->Offset = freeBlock->PhysicalPrev->Offset;
            ExtractFromFreeList(freeBlock->PhysicalPrev);

            Block* prevBlock = freeBlock->PhysicalPrev;
            Block* neighborBlock = prevBlock->PhysicalPrev;
            if (neighborBlock != nullptr) {
                neighborBlock->PhysicalNext = freeBlock;
            }
            freeBlock->PhysicalPrev = neighborBlock;
            DestroyBlock(prevBlock);
        }

        if (freeBlock->PhysicalNext != nullptr && freeBlock->PhysicalNext->bIsFree) {
            freeBlock->Size += freeBlock->PhysicalNext->Size;
            ExtractFromFreeList(freeBlock->PhysicalNext);

            Block* nextBlock = freeBlock->PhysicalNext;
            Block* neighborBlock = nextBlock->PhysicalNext;
            if (neighborBlock != nullptr) {
                neighborBlock->PhysicalPrev = freeBlock;
            }
            freeBlock->PhysicalNext = neighborBlock;
            DestroyBlock(nextBlock);
        }
    }

    void TwoLevelSegregatedFit::DestroyBlock(Block* block) {
        M3_ASSERT(block != nullptr);
        if (block == physicalHead_) {
            M3_ASSERT(block->PhysicalPrev == nullptr);
            M3_ASSERT(block->PhysicalNext != nullptr);
            physicalHead_ = block->PhysicalNext;
        }

        if (block == physicalTail_) {
            M3_ASSERT(block->PhysicalPrev != nullptr);
            M3_ASSERT(block->PhysicalNext == nullptr);
            physicalTail_ = block->PhysicalPrev;
        }
        Destroy(block);
    }

    const TwoLevelSegregatedFit::Block* TwoLevelSegregatedFit::Allocate(const uint64 size) {
        M3_ASSERT(size > 0);
        if (size > initialBlockSize_) {
            return nullptr;
        }

        const uint64 firstLevel = CalculateMostSignificantBitIndex(size);

        const uint64 binWidth = firstLevel >= secondLevelIndex_
                                    ? (1Ui64 << (firstLevel - secondLevelIndex_))
                                    : (1Ui64 << firstLevel);

        const uint64 rounding = binWidth - 1;
        if (size > std::numeric_limits<uint64>::max() - rounding) {
            return nullptr;
        }

        MappedLevel level = Mapping(size + rounding);
        if (level.FirstLevel >= secondLevelBitmaps_.size()) {
            return nullptr;
        }

        uint64 secondLevelBitmap =
                secondLevelBitmaps_[level.FirstLevel] & (~0Ui64 << level.SecondLevel);

        if (secondLevelBitmap == 0) {
            if (level.FirstLevel >= 63Ui64) {
                return nullptr;
            }

            const uint64 candidates =
                    firstLevelBitmap_ & (~0Ui64 << (level.FirstLevel + 1));

            if (candidates == 0) {
                return nullptr;
            }

            level.FirstLevel = std::countr_zero(candidates);
            secondLevelBitmap = secondLevelBitmaps_[level.FirstLevel];
        }

        M3_ASSERT(secondLevelBitmap != 0);
        level.SecondLevel = std::countr_zero(secondLevelBitmap);
        M3_ASSERT(level.SecondLevel < numSecondLevelSubdivisions_);

        const uint64 freeListIdx = level.GetFreeListIndex(numSecondLevelSubdivisions_);
        M3_ASSERT(freeListIdx < freeLists_.size());
        Block* freeBlock = freeLists_[freeListIdx];
        M3_ASSERT(freeBlock != nullptr);
        ExtractFromFreeList(freeBlock);

        if (size < freeBlock->Size) {
            Block* splitBlock = Create<Block>();
            splitBlock->Size = freeBlock->Size - size;
            splitBlock->Offset = freeBlock->Offset + size;

            splitBlock->PhysicalPrev = freeBlock;
            splitBlock->PhysicalNext = freeBlock->PhysicalNext;

            if (freeBlock->PhysicalNext != nullptr) {
                freeBlock->PhysicalNext->PhysicalPrev = splitBlock;
                M3_ASSERT(splitBlock->Offset < freeBlock->PhysicalNext->Offset);
                M3_ASSERT((splitBlock->Offset + splitBlock->Size) == freeBlock->PhysicalNext->Offset);
            }
            freeBlock->PhysicalNext = splitBlock;

            M3_ASSERT(physicalTail_ != nullptr);
            if (freeBlock == physicalTail_) {
                physicalTail_ = splitBlock;
            }

            freeBlock->Size = size;

            InsertToFreeList(splitBlock);
        }

        M3_ASSERT(freeBlock != nullptr);
        M3_ASSERT(freeBlock->Size > 0 && freeBlock->Size == size);
        freeBlock->bIsFree = false;
        allocatedSize_ += size;
        return freeBlock;
    }

    void TwoLevelSegregatedFit::Deallocate(const Block* targetBlock) {
        M3_ASSERT(targetBlock != nullptr);
        M3_ASSERT(!targetBlock->bIsFree);
        M3_ASSERT(targetBlock->Size > 0);
        const uint64 size = targetBlock->Size;
        M3_ASSERT(size <= allocatedSize_);

        Block* block = const_cast<Block*>(targetBlock);
        block->bIsFree = true;
        MergeWithFreePhysicallyContinuousBlock(block);
        InsertToFreeList(block);
        allocatedSize_ -= size;
    }

    void TwoLevelSegregatedFit::Grow(const uint64 newSize) {
        M3_ASSERT(newSize > initialBlockSize_);
        M3_ASSERT(physicalTail_ != nullptr);
        const uint64 oldInitialBlockSize = initialBlockSize_;
        SetInitialBlockSize(newSize);

        Block* newBlock = Create<Block>();
        newBlock->Size = newSize - oldInitialBlockSize;

        physicalTail_->PhysicalNext = newBlock;

        newBlock->Offset = physicalTail_->Offset + physicalTail_->Size;
        newBlock->PhysicalPrev = physicalTail_;
        physicalTail_ = newBlock;

        MergeWithFreePhysicallyContinuousBlock(newBlock);
        InsertToFreeList(newBlock);
    }
}
