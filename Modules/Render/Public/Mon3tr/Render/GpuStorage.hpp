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
#include <Mon3tr/Render/RenderMinimal.hpp>
#include <Mon3tr/Core/HandleManager.hpp>
#include <Mon3tr/Core/TwoLevelSegregatedFit.hpp>

namespace mon3tr::render {
    struct GpuStorageDesc {
        nvrhi::DeviceHandle RenderDevice{};
        class SwapChain*    SwapChainInstance = nullptr;

        uint64 SizeOfStorage = 0;
        uint32 StructStride = 1;

        bool bIsRawBuffer = false; // else Structured Buffer

        //bool bAllowUnorderedAccess = false;

        std::string_view DebugName = "UnknownGpuStorage";
    };

    class GpuStorage {
    public:
        struct Alloc;
        using Block = TwoLevelSegregatedFit::Block;

        struct Range {
            uint64 Offset = 0;
            uint64 Size = 0;
        };

    public:
        explicit GpuStorage(const GpuStorageDesc& desc);

        virtual ~GpuStorage();

        GpuStorage(const GpuStorage&) = delete;

        GpuStorage(GpuStorage&&) noexcept = delete;

        GpuStorage& operator=(const GpuStorage&) = delete;

        GpuStorage& operator=(GpuStorage&&) noexcept = delete;

        Handle<Alloc> Allocate(const uint64 size);

        void Deallocate(Handle<Alloc> handle);

        std::optional<Range> QueryRange(const Handle<Alloc> handle) const;

        void BeginFrame();

        nvrhi::IBuffer* GetBuffer() { return buffer_.Get(); }

    private:
        void Grow();

        void Delete(Handle<Alloc> handle);

        void CreateBuffer();

    private:
        static constexpr uint64 kGrowFactor = 2;

        nvrhi::DeviceHandle renderDevice_{};
        SwapChain*          swapChain_ = nullptr;

        std::string debugName_{};

        uint64 sizeOfBuffer_ = 0;
        uint32 structStride_ = 1;

        bool bIsRawBuffer = false;

        nvrhi::BufferHandle buffer_{};

        HandleManager<const Block*>    handleManager_{};
        TwoLevelSegregatedFit          allocationStrategy;
        Vector<Queue<Handle<Alloc> > > deferredDeletionQueues_;
    };

    struct StructuredGpuStorageDesc {
        nvrhi::DeviceHandle RenderDevice{};
        class SwapChain*    SwapChainInstance = nullptr;

        uint64 NumElements = 0;

        std::string_view DebugName = "UnknownGpuStorage";
    };

    template<typename T>
    class StructuredGpuStorage : public GpuStorage {
    public:
        struct ElementRange {
            uint64 OffsetIndex = 0;
            uint64 NumElements = 0;
        };

    public:
        explicit StructuredGpuStorage(const StructuredGpuStorageDesc& desc) : GpuStorage(GpuStorageDesc{
                                                                                .RenderDevice = desc.RenderDevice,
                                                                                .SwapChainInstance = desc.SwapChainInstance,
                                                                                .SizeOfStorage = sizeof(T) * desc.NumElements,
                                                                                .StructStride = sizeof(T),
                                                                                .bIsRawBuffer = false,
                                                                                .DebugName = desc.DebugName
                                                                            }) {
        }

        ~StructuredGpuStorage() override = default;

        Handle<Alloc> AllocateElements(const uint64 numElements) {
            M3_ASSERT(numElements > 0);
            return Allocate(numElements * sizeof(T));
        }

        std::optional<ElementRange> QueryElementRange(const Handle<Alloc> handle) const {
            const std::optional<Range> rangeOpt = QueryRange(handle);
            if (!rangeOpt) {
                return std::nullopt;
            }

            return ElementRange{
                .OffsetIndex = rangeOpt->Offset / sizeof(T),
                .NumElements = rangeOpt->Size / sizeof(T)
            };
        }
    };
}
