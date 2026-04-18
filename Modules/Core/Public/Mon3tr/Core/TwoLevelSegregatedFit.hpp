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

namespace mon3tr {
    // TLSF의 로직을 수행하는 클래스, 실제 메모리 공간을 할당하지 않으며. 주어진 가상의 메모리 공간에 대해 TLSF를 통한 메모리 할당을 시뮬레이션한다.
    // 실제 TLSF의 경우, 기존 할당 받은 메모리 공간을 연속적으로 확장 하지 않는한(ex. Virtual Alloc), 기존 메모리 블럭과 물리적으로 연속하지 않은 새로운 블럭을 넣어야 한다.
    // 다만, 해당 TLSF 클래스는 Virtual Alloc과 같은 방식으로 연속적인 공간의 확장이 가능하다고 가정한다.
    // (주로 GPU 공간상의 메모리 공간을 자유롭게 다루기 위해 설계하므로, 새로게 더 큰 버퍼를 할당하는 것은 기존 공간을 자연스럽게 확장 하는 것과 다를바 없기 때문.)
    // 이 경우, 가장 끝 TLSF 노드를 찾아서 (새로운 버퍼 크기-기존 버퍼크기) bytes 만큼의 메모리 블럭을 물리적으로 연결된 다음 노드로 설정하면 된다.
    class TwoLevelSegregatedFit {
    public:
        struct Block {
            friend class TwoLevelSegregatedFit;

        public:
            uint64 Offset = 0;
            uint64 Size   : 63 = 0;
            bool   bIsFree: 1 = true;

        public:
            Block* PhysicalPrev = nullptr;
            Block* PhysicalNext = nullptr;
            Block* FreeListPrev = nullptr;
            Block* FreeListNext = nullptr;
        };

        struct MappedLevel {
            uint64 GetFreeListIndex(const uint64 numSecondLevelSubdivisions) const noexcept {
                M3_ASSERT(SecondLevel < numSecondLevelSubdivisions);
                return (FirstLevel * numSecondLevelSubdivisions) + SecondLevel;
            }

        public:
            uint64 FirstLevel = 0;
            uint64 SecondLevel = 0;
        };

    public:
        // 각 First Level 구간은 2^secondLevelIdx 개의 서브구간으로 나뉨
        TwoLevelSegregatedFit(uint64 initialBlockSize, uint64 secondLevelIdx);

        ~TwoLevelSegregatedFit();

        TwoLevelSegregatedFit(const TwoLevelSegregatedFit& other) = delete;

        TwoLevelSegregatedFit& operator=(const TwoLevelSegregatedFit& other) = delete;

        TwoLevelSegregatedFit(TwoLevelSegregatedFit&& other) noexcept = delete;

        TwoLevelSegregatedFit& operator=(TwoLevelSegregatedFit&& other) noexcept = delete;

        const Block* Allocate(const uint64 size);

        void Deallocate(const Block* targetBlock);

        void Grow(const uint64 newSize);

    private:
        void SetInitialBlockSize(const uint64 size);

        // TLSF@M. Masmano et al.이 제안한 맵핑 공식
        // f = floor(log_2(size))
        // s = (size - 2^f) * (2^SLI / 2^f) = (size - 2^f) * 2^(SLI-f) ; SLI
        // => s = (size ^ (1 << f)) >> (f - SLI)
        // s_0 = (size - 2^f) == (size^(1<<f)): 2^f가 정확히 2의 승수이므로, xor 연산이 정확히 해당 값을 빼는 것과 동일한 결과
        // s_0 >> (f-SLI): s_0 * 2^(SLI-f) = s_0 / 2^(f-SLI) 이 성립하며, 2 ^ (f-SLI)로 나눈다는건 right bit shift 연산을 (f-SLI)만큼 수행하는 것과 동등
        // 그러므로, s = (size - 2^f) * (s^SLI / s^f) = (size ^ (1 << f)) >> (f - SLI)
        // s_0 = (size - 2^f) => 현재 f 구간에서의 상대적인 offset
        // 2^f = 현재 구간의 Offset = 0 지점,
        // s_0 * (2^SLI / 2^f) = s_0 / (2^f/2^SLI)
        // 나눠질 구역 수 D = 2^SLI
        // S: [2^f, 2^(f+1)) 구간을 D 개의 서브 구간으로 나눔,
        // 이 때, 구간 S의 크기는 2^f
        // 그러므로, 구간 S를 D 개의 서브 구간으로 나누었을 때, 각 서브 구간의 크기는 2^f/D = 2^f/2^SLI
        // 구간에서의 상대 offset s_0를 각 서브 구간으로 나눔으로써, 어떤 서브 구간에 위치하는지 알아낼 수 있음
        // 그러므로, s = (size-2^f) / (2^f / 2^SLI) = (size-2^f) * (2^SLI / 2^f) 성립
        // ex. f = 4, 2^4 = 16, SLI = 4
        // 2^4~2^5 = [16, 32) 16/4 = 4, subdivision당 1바이트
        // size = 20 bytes, s = (20 - 16) * 16 / 16 = 4
        // 이를 통해 알 수 있는 사실, SLI는 최소 할당 크기를 정하게됨.
        // SLI=4의 경우 구간을 16개로 나누기에 최소 2^4 바이트 이상의 할당에 대해서만 작동
        // (만약 2^3 바이트~2^4 바이트 사이의 할당이 가능하다 하면, 서브 구간의 크기가 1 바이트 미만이 되므로 모순)
        // 다만, 실제 논문에서는 헤더의 크기가 이 최소 크기를 보장하기에 실질적인 문제는 없으며.
        // 맵핑이 1:1로 되어서 실질적으로 구간을 나누는 능력이 약해지더라도 여전히 2단 계층 구조로 최소한 Segregated Fit을 사용하는 것과 동일한 동작 보장 가능
        MappedLevel Mapping(uint64 size);

        // Insert to free list structure; newBlock은 현재 FreeList에 등록되지 않은 블럭이여야 한다.
        void InsertToFreeList(Block* newBlock);

        // Extract from free list structure; block은 현재 FreeList에 등록되어 있는 블럭이여야 한다.
        void ExtractFromFreeList(Block* block);

        // freeBlock은 현재 FreeList에 등록되지 않은 블럭이여야 한다. 하지만 할당 해제 예정인 블럭이여야 한다(bIsFree==true)
        void MergeWithFreePhysicallyContinuousBlock(Block* freeBlock);

        // block은 현재 FreeList에 등록되지 않은 블럭이여야 한다.
        void DestroyBlock(Block* block);

    private:
        uint64 initialBlockSize_ = 0;
        uint64 allocatedSize_ = 0;

        uint64 numFirstLevels_ = 0;
        uint64 secondLevelIndex_ = 0;
        uint64 numSecondLevelSubdivisions_ = 0;

        // numFirstLevels_ = log_2(initialBlockSize) + 1
        // initialBlockSize = 16 bytes -> numFirstLevels_ = 4, secondLevel[4] => [16, 32) bytes block
        uint64 firstLevelBitmap_ = 0;
        // secondLevelBitmaps_.size() == numFirstLevels_
        Vector<uint64> secondLevelBitmaps_{};
        // numFirstLevels_ * numSecondLevelSubdivisions_
        // numSecondLevelSubdivisions_ = 2^{secondLevelIndex_}; secondLevelIndex_ > 1
        Vector<Block*> freeLists_{};

        Block* physicalHead_ = nullptr;
        Block* physicalTail_ = nullptr;
    };
}
