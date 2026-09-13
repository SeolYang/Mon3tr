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
#include <Mon3tr/Render/DescriptorManager.hpp>
#include <Mon3tr/Render/SwapChain.hpp>

namespace mon3tr::render {
    EDescriptorManagerInitializeResult DescriptorManager::Initialize(const DescriptorManagerDependency& dependency, const DescriptorManagerDesc& desc) {
        M3_ASSERT(dependency.RenderDevice != nullptr);
        M3_ASSERT(dependency.SwapChainInstance != nullptr);
        M3_ASSERT(desc.NumDescriptors > 0);

        this->renderDevice_ = dependency.RenderDevice;
        this->swapChain_ = dependency.SwapChainInstance;
        M3_ASSERT(swapChain_->GetBackBufferCount() <= kMaxDeferredDeletionQueues);

        nvrhi::BindingLayoutHandle bindlessLayout = renderDevice_->createBindlessLayout(nvrhi::BindlessLayoutDesc{
            .visibility = nvrhi::ShaderType::All,
            .firstSlot = 0,
            .maxCapacity = desc.NumDescriptors,
            .layoutType = nvrhi::BindlessLayoutDesc::LayoutType::MutableSrvUavCbv
        });
        if (bindlessLayout_ == nullptr) {
            return EDescriptorManagerInitializeResult::FailedToCreateBindlessLayout;
        }

        nvrhi::DescriptorTableHandle descriptorTable = renderDevice_->createDescriptorTable(bindlessLayout_.Get());
        if (descriptorTable_ == nullptr) {
            return EDescriptorManagerInitializeResult::FailedToCreateDescriptorTable;
        }

        bindlessLayout_ = std::move(bindlessLayout);
        descriptorTable_ = std::move(descriptorTable);

        for (uint32 slotIdx = desc.NumDescriptors; slotIdx > 0; --slotIdx) {
            slotPool_.push(slotIdx - 1);
        }

#if defined(DEBUG) || defined(_DEBUG)
        slotUsed_.resize(desc.NumDescriptors);
#endif

        M3_ASSERT(bindlessLayout_ != nullptr && descriptorTable_ != nullptr);
        MarkAsInitialized();
        return EDescriptorManagerInitializeResult::Success;
    }

    void DescriptorManager::Shutdown() {
        renderDevice_ = nullptr;
        swapChain_ = nullptr;

        descriptorTable_ = nullptr;
        bindlessLayout_ = nullptr;

        slotPool_.get_container().clear();

        finalizedDeletionQueues_.clear();

        System::Shutdown();
    }

    std::expected<Descriptor, EDescriptorCreateResult> DescriptorManager::Create(nvrhi::BindingSetItem bindingSetItem) {
        Descriptor newDescriptor{};
        {
            std::unique_lock lock{slotPoolMutex_};
            if (slotPool_.empty()) {
                return std::unexpected{EDescriptorCreateResult::EmptyPool};
            }

            newDescriptor.slotIdx_ = slotPool_.top();
            slotPool_.pop();

#if defined(DEBUG) || defined(_DEBUG)
            M3_ASSERT(!slotUsed_[newDescriptor.slotIdx_]);
            slotUsed_[newDescriptor.slotIdx_] = true;
#endif
        }

        M3_ASSERT(!newDescriptor.IsNull());
        bindingSetItem.slot = newDescriptor.slotIdx_;
        if (!renderDevice_->writeDescriptorTable(descriptorTable_.Get(), bindingSetItem)) {
            std::unique_lock lock{slotPoolMutex_};
            slotPool_.push(newDescriptor.slotIdx_);
#if defined(DEBUG) || defined(_DEBUG)
            slotUsed_[newDescriptor.slotIdx_] = false;
#endif
            return std::unexpected{EDescriptorCreateResult::FailedToWriteDescriptorTable};
        }

        return newDescriptor;
    }

    void DescriptorManager::Destroy(const Descriptor descriptor) {
        M3_ASSERT(swapChain_ != nullptr);
        std::unique_lock lock{deferredDeletionQueues_[swapChain_->GetCurrentBackBufferIndex()].Mutex};
        deferredDeletionQueues_[swapChain_->GetCurrentBackBufferIndex()].DeletionQueue.push_back(descriptor);
    }

    void DescriptorManager::FlushDeletionQueue() {
        // Critical Section
        {
            DeferredDeletionQueue& currentDeletionQueue = deferredDeletionQueues_[swapChain_->GetCurrentBackBufferIndex()];
            std::scoped_lock       lock{slotPoolMutex_, currentDeletionQueue.Mutex};
            finalizedDeletionQueues_.reserve(currentDeletionQueue.DeletionQueue.size());
            for (const Descriptor descriptor: currentDeletionQueue.DeletionQueue) {
                finalizedDeletionQueues_.push_back(descriptor);
                slotPool_.push(descriptor.slotIdx_);
#if defined(DEBUG) || defined(_DEBUG)
                M3_ASSERT(slotUsed_[descriptor.slotIdx_]);
                slotUsed_[descriptor.slotIdx_] = false;
#endif
            }
            currentDeletionQueue.DeletionQueue.clear();
        }

        for (const Descriptor finalizedDescriptor: finalizedDeletionQueues_) {
            renderDevice_->writeDescriptorTable(descriptorTable_.Get(), nvrhi::BindingSetItem::None(finalizedDescriptor.slotIdx_));
        }
        finalizedDeletionQueues_.clear();
    }
}
