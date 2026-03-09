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
#include <Mon3tr/Core/Memory.hpp>
#include <random>

namespace mon3tr {
    template<MemoryCategory C>
    class Allocator {
        struct Header {
            void* Base = nullptr;
#if defined(DEBUG) || defined(_DEBUG)
            uint32 MagicNumber = 0xFFFFFFFF;
#endif
        };

    public:
        explicit Allocator([[maybe_unused]] const char* name) {
#if defined(DEBUG) || defined(_DEBUG)
            std::random_device                                           randomDevice{};
            std::mt19937                                                 gen{randomDevice()};
            std::uniform_int_distribution<decltype(Header::MagicNumber)> distribution{};
            this->magicNumber_ = distribution(gen);
#endif
        }

        Allocator([[maybe_unused]] const Allocator& other) : magicNumber_(other.magicNumber_) {
        }

        Allocator([[maybe_unused]] const Allocator& other, [[maybe_unused]] const char* name) : Allocator(other) {
        }

        Allocator& operator=([[maybe_unused]] const Allocator& other) {
            return *this;
        }

        // allocate로 반환되는 포인터는 p + offset == aligned data ptr를 만족해야함
        // 하지만, offset을 고려할 경우, data ptr의 alignment를 일반적으로 충족하기 부족함
        // 그렇다고 중간에 padding을 넣으면 EASTL의 요구사항을 만족하기 어려움
        // 그러므로, offset을 사용하는 경우/그렇지 않은 경우 모두 반환하는 포인터 앞에 실제 주소를 포함하는 헤더를 추가하는 것이 바람직
        // 즉 [Padding][Header(8 bytes fixed)][Offset][Aligned Data]
        void* allocate(const size_t size, [[maybe_unused]] int flags = 0) {
            //return allocate(size, 1, 0, flags); // minimize empty space
            return allocate(size, 16, 0, flags); // balanced one
            //return allocate(size, kCacheLineSize, 0, flags); // maximize cache efficiency of data structure
        }

        void* allocate(const size_t size, const size_t alignment, const size_t offset, [[maybe_unused]] int flags = 0) {
            M3_PRE_COND(size > 0);
            M3_PRE_COND(alignment > 0);

            const size_t additionalPayloadSize = offset + kHeaderSize;
            const size_t paddingSize = AlignUp(additionalPayloadSize, alignment) - additionalPayloadSize;
            M3_ASSERT((additionalPayloadSize + paddingSize) % alignment == 0);
            const size_t requiredAllocSize = additionalPayloadSize + paddingSize + size;
            void*        base = Allocate<C>(requiredAllocSize, alignment);
            M3_ASSERT(reinterpret_cast<size_t>(base) % alignment == 0);

            Header* const header = reinterpret_cast<Header*>(static_cast<uint8*>(base) + paddingSize);
            *header = Header{.Base = base};

#if defined(DEBUG) || defined(_DEBUG)
            header->MagicNumber = magicNumber_;
#endif

            void* const p = static_cast<uint8*>(header->Base) + paddingSize + kHeaderSize;
            M3_ASSERT(reinterpret_cast<size_t>(static_cast<uint8*>(p) + offset) % alignment == 0);
            return p;
        }

        // @warning 전달 받은 ptr가 Allocator에 의해 할당된 메모리 공간인지 확실하게 보장 할 수는 없음.
        // 즉, Allocator에 의해 할당되지 않은 메모리 공간을 가르키는 포인터에 대한 deallocate 호출은 UB가 발생 할 수 있음
        void deallocate(void* const ptr, [[maybe_unused]] size_t n) {
            if (ptr == nullptr) { return; }
            const Header* const header = reinterpret_cast<const Header*>(static_cast<uint8*>(ptr) - kHeaderSize);

#if defined(DEBUG) || defined(_DEBUG)
            M3_ASSERT(header->MagicNumber == magicNumber_);
#endif
            return Deallocate<C>(header->Base);
        }

        const char* get_name() const { return C::Name.data(); }

        void set_name([[maybe_unused]] const char* name) {
        }

    private:
        static constexpr size_t kHeaderSize = sizeof(Header);
#if defined(DEBUG) || defined(_DEBUG)
        uint32_t magicNumber_ = 0;
#endif
    };
}
