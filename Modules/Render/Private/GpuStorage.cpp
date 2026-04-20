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
#include <Mon3tr/Render/GpuStorage.hpp>
#include <Mon3tr/Render/SwapChain.hpp>

namespace mon3tr::render {
    GpuStorage::GpuStorage(const GpuStorageDesc& desc) : renderDevice_(desc.RenderDevice)
                                                       , swapChain_(desc.SwapChainInstance)
                                                       , debugName_(desc.DebugName)
                                                       , sizeOfBuffer_(desc.SizeOfStorage)
                                                       , structStride_(desc.StructStride)
                                                       , bIsRawBuffer(desc.bIsRawBuffer)
                                                       , allocationStrategy(desc.SizeOfStorage, 4Ui64) {
        M3_ASSERT(desc.RenderDevice != nullptr);
        M3_ASSERT(desc.SwapChainInstance != nullptr);
        M3_ASSERT(desc.SizeOfStorage > 0);
        M3_ASSERT(desc.StructStride > 0);

        CreateBuffer();

        deferredDeletionQueues_.resize(swapChain_->GetBackBufferCount());
    }

    Handle<GpuStorage::Alloc> GpuStorage::Allocate(const uint64 size) {
        M3_ASSERT(size > 0);
        M3_ASSERT(buffer_ != nullptr);

        const Block* newBlock = allocationStrategy.Allocate(size);
        if (newBlock == nullptr) {
            Grow();
            newBlock = allocationStrategy.Allocate(size);
        }

        if (newBlock == nullptr) {
            return Handle<GpuStorage::Alloc>{};
        }

        return Handle<Alloc>{handleManager_.Create(newBlock).Raw};
    }

    void GpuStorage::Deallocate(const Handle<Alloc> handle) {
        deferredDeletionQueues_[swapChain_->GetCurrentBackBufferIndex()].push(handle);
    }

    std::optional<GpuStorage::Range> GpuStorage::QueryRange(const Handle<Alloc> handle) const {
        if (handle.IsNull()) {
            return std::nullopt;
        }

        const Handle<const Block*> blockHandle{handle.Raw};
        const Block* const *       blockPtr = handleManager_.Get(blockHandle);
        if (blockPtr == nullptr) {
            return std::nullopt;
        }

        return Range{.Offset = (*blockPtr)->Offset, .Size = (*blockPtr)->Size};
    }

    void GpuStorage::BeginFrame() {
        Queue<Handle<Alloc> >& deferredDeletionQueue = deferredDeletionQueues_[swapChain_->GetCurrentBackBufferIndex()];
        while (!deferredDeletionQueue.empty()) {
            Delete(deferredDeletionQueue.front());
            deferredDeletionQueue.pop();
        }
    }

    void GpuStorage::Grow() {
        const uint64 oldSize = sizeOfBuffer_;
        sizeOfBuffer_ *= kGrowFactor;
        allocationStrategy.Grow(sizeOfBuffer_);

        const nvrhi::BufferHandle oldBuffer = buffer_;
        CreateBuffer();

        const nvrhi::CommandListHandle cmdList = renderDevice_->createCommandList(nvrhi::CommandListParameters{
            .queueType = nvrhi::CommandQueue::Copy
        });

        cmdList->open();
        cmdList->copyBuffer(buffer_.Get(), 0, oldBuffer.Get(), 0, oldSize);
        cmdList->close();

        const nvrhi::EventQueryHandle eventQuery = renderDevice_->createEventQuery();
        renderDevice_->executeCommandList(cmdList);
        renderDevice_->setEventQuery(eventQuery.Get(), nvrhi::CommandQueue::Copy);
        // 동기 방식, pollEventQuery로 비동기로 확인 가능! 추후 비동기 에셋 로딩/스트리밍에 활용할것!
        renderDevice_->waitEventQuery(eventQuery.Get());
    }

    void GpuStorage::Delete(const Handle<Alloc> handle) {
        M3_ASSERT(!handle.IsNull());
        const Handle<const Block*> blockHandle{handle.Raw};
        const Block* const *       blockPtr = handleManager_.Get(blockHandle);
        if (blockPtr == nullptr) {
            return;
        }
        M3_ASSERT(*blockPtr != nullptr);
        allocationStrategy.Deallocate(*blockPtr);
        handleManager_.Destroy(blockHandle);
    }

    void GpuStorage::CreateBuffer() {
        buffer_ = renderDevice_->createBuffer(nvrhi::BufferDesc{
            .byteSize = sizeOfBuffer_, .structStride = structStride_,
            .debugName = debugName_,
            .canHaveUAVs = false,
            .canHaveTypedViews = true, .canHaveRawViews = true,
            .initialState = nvrhi::ResourceStates::ShaderResource,
            .keepInitialState = true
        });
    }
}
