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
#include <Mon3tr/Core/Platform.hpp>
#include <Mon3tr/Core/Types.hpp>
#include <Mon3tr/Core/Assertion.hpp>

#define M3_DECLARE_MEM_CATEGORY(MEM_CATEGORY) \
namespace mon3tr::internal::memory { \
struct MEM_CATEGORY { \
constexpr static std::string_view  Name = #MEM_CATEGORY; \
static std::atomic<mon3tr::uint64> NumAllocations; \
static std::atomic<mon3tr::uint64> AllocationSize; \
}; \
}

#define M3_DEFINE_MEM_CATEGORY(MEM_CATEGORY) \
namespace mon3tr::internal::memory { \
std::atomic<mon3tr::uint64> MEM_CATEGORY::NumAllocations = 0; \
std::atomic<mon3tr::uint64> MEM_CATEGORY::AllocationSize = 0; \
}

#define M3_MEM_CATEGORY(MEM_CATEGORY) mon3tr::internal::memory::MEM_CATEGORY

M3_DECLARE_MEM_CATEGORY(Unspecified)

namespace mon3tr {
    constexpr uint64 kCacheLineSize = 64;

    template<typename Category>
    concept MemoryCategory = requires()
    {
        { Category::Name };
        std::convertible_to<decltype(Category::Name), std::string_view>;
        std::same_as<decltype(Category::NumAllocations), std::atomic<mon3tr::uint64> >;
        std::same_as<decltype(Category::AllocationSize), std::atomic<mon3tr::uint64> >;
    };

    namespace internal {
        void* Allocate(uint64 size, uint64 alignment, std::string_view categoryName);

        void Deallocate(void* ptr, std::string_view categoryName);

        void DumpMemoryLeaks();

        uint64 GetAllocationSize(const void* ptr);

        /**
         * @warning 해당 함수는 호출 시, 타입에 대한 메모리 공간을 확보하고 그 공간에 대한 메모리 주소를 반환하지만,
         * 생성자를 호출하지 않아 초기화 되지 않은 메모리 공간을 반환 합니다. 만약 별도로 생성자를 호출하지 않고
         * 접근하는 경우 UB가 발생 할 수 있습니다.
         * @tparam T 타입
         * @tparam C 메모리 카테고리
         * @return 생성자가 호출되지 않은 인스턴스에 대한 메모리 주소
         */
        template<typename T, MemoryCategory C>
        T* CreateWithoutConstruct() {
            T* ptr = static_cast<T*>(internal::Allocate(sizeof(T), alignof(T), C::Name));
            if (ptr != nullptr) {
                C::NumAllocations.fetch_add(1);
                C::AllocationSize.fetch_add(internal::GetAllocationSize(ptr));
            }
            return ptr;
        }

        // snmalloc::libc wrapper
        namespace libc {
            void* Malloc(const uint64 size);

            void Free(void* ptr);

            void* Realloc(void* ptr, const uint64 size);

            void* Calloc(const uint64 size);
        }
    }

    template<MemoryCategory C = M3_MEM_CATEGORY(Unspecified)>
    void* Allocate(const uint64 size, const uint64 alignment) {
        void* ptr = internal::Allocate(size, alignment, C::Name);
        if (ptr != nullptr) {
            C::NumAllocations.fetch_add(1);
            C::AllocationSize.fetch_add(internal::GetAllocationSize(ptr));
        }
        return ptr;
    }

    template<typename T, MemoryCategory C = M3_MEM_CATEGORY(Unspecified), typename... Args>
    T* Create(Args&&... args) {
        T* ptr = internal::CreateWithoutConstruct<T, C>();
        if (ptr != nullptr) {
            std::construct_at(ptr, std::forward<Args>(args)...);
        }
        return ptr;
    }

    template<MemoryCategory C = M3_MEM_CATEGORY(Unspecified)>
    void Deallocate(void* ptr) {
        M3_PRE_COND(ptr != nullptr);
        M3_PRE_COND(C::NumAllocations > 0 && C::AllocationSize > 0);

        const uint64 allocSize = internal::GetAllocationSize(ptr);
        internal::Deallocate(ptr, C::Name);

        C::NumAllocations.fetch_sub(1);
        C::AllocationSize.fetch_sub(allocSize);
    }

    template<typename T, MemoryCategory C = M3_MEM_CATEGORY(Unspecified)>
    void Destroy(T* const ptr) {
        M3_PRE_COND(ptr != nullptr);
        M3_PRE_COND(C::NumAllocations > 0 && C::AllocationSize > 0);

        ptr->~T();
        Deallocate<C>(ptr);
    }

    template<typename T, MemoryCategory C = M3_MEM_CATEGORY(Unspecified)>
    struct PtrDeleter {
        void operator()(T* const ptr) const {
            Destroy<T, C>(ptr);
        }
    };

    /**
     * @warning Create/CreateWithoutConstruct/Allocate 등, 엔진 하부에서 지원하는 메모리 관리 체계 외
     * 메모리 할당(ex. new, malloc 등) 방식을 사용한 포인터를 전달하는 경우, 해제 단계에서 정의되지않은 행동을 할 수 있음.
     * @todo 추후 아예 별도의 스마트 포인터 클래스로의 전환을 고려 할 것. (ex. Raw Pointer 전달 방식을 완전히 차단)
     */
    template<typename T, MemoryCategory C = M3_MEM_CATEGORY(Unspecified)>
    using Ptr = std::unique_ptr<T, PtrDeleter<T, C> >;

    template<typename T, MemoryCategory C = M3_MEM_CATEGORY(Unspecified), typename... Args>
    auto MakePtr(Args&&... args) {
        return Ptr<T, C>{Create<T, C>(std::forward<Args>(args)...)};
    }

    template<typename T>
    constexpr bool IsPowerOf2(T x) noexcept { return ((x != 0) && !(x & (x - 1))); }

    constexpr uint64 AlignUp(const uint64 value, const uint64 alignment) noexcept {
        if (alignment == 0) {
            return value;
        }

        M3_ASSERT(IsPowerOf2(alignment));
        return (value + (alignment - 1)) & ~(alignment - 1);
    }

    class Page {
    public:
        static uint64 GetSize() {
            static uint64 cachedPageSize = []() {
#ifdef M3_PLATFORM_WINDOWS
                SYSTEM_INFO sysInfo;
                GetSystemInfo(&sysInfo);
                return sysInfo.dwPageSize;
#else
                M3_UNIMPLEMENTED();
                return 0x1000ui64;
#endif
            }();

            return cachedPageSize;
        }
    };
}
