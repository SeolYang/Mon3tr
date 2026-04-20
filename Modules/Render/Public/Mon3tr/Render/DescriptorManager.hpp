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
#include <Mon3tr/Core/System.hpp>
#include <Mon3tr/Render/RenderMinimal.hpp>

namespace mon3tr::render {
    struct DescriptorManagerDependency {
        nvrhi::DeviceHandle RenderDevice{};
        class SwapChain*    SwapChainInstance = nullptr;
    };

    struct DescriptorManagerDesc {
        uint32 NumDescriptors = 0;
    };

    enum class EDescriptorManagerInitializeResult : uint8 {
        Success,
    };

    enum class EDescriptorCreateResult : uint8 {
        Success,
        EmptyPool,
        FailedToWriteDescriptorTable,
    };

    // Bindless Resources Descriptor Management
    struct Descriptor {
        friend class DescriptorManager;

    public:
        [[nodiscard]] uint32 GetSlotIndex() const noexcept { return slotIdx_; }
        [[nodiscard]] bool   IsNull() const noexcept { return slotIdx_ == kNullSlotIndex; }

    private:
        static constexpr uint32 kNullSlotIndex = ~0ui32;
        uint32                  slotIdx_ = kNullSlotIndex;
    };

    class DescriptorManager : public System {
    public:
        ~DescriptorManager() override = default;

        [[nodiscard]] EDescriptorManagerInitializeResult Initialize(const DescriptorManagerDependency& dependency, const DescriptorManagerDesc& desc);

        void Shutdown() override;

        // thread safety 보장 필요; slot ignored
        std::expected<Descriptor, EDescriptorCreateResult> Create(nvrhi::BindingSetItem bindingSetItem);

        // thread safety 보장 필요; deferred deletion 필요
        void Destroy(Descriptor descriptor);

        // Execute on the main thread at the beginning of every single frame.
        void FlushDeletionQueue();

    private:
        nvrhi::DeviceHandle renderDevice_{};
        SwapChain* swapChain_ = nullptr;

        nvrhi::DescriptorTableHandle descriptorTable_{};
        nvrhi::BindingLayoutHandle   bindlessLayout_{};

        std::mutex                             slotPoolMutex_{};
        Stack<uint32, M3_MEM_CATEGORY(Render)> slotPool_{};
#if defined(DEBUG) || defined(_DEBUG)
        Vector<bool> slotUsed_{};
#endif

        struct DeferredDeletionQueue {
            Vector<Descriptor, M3_MEM_CATEGORY(Render)> DeletionQueue{};
            std::mutex                                 Mutex{};
        };

        constexpr static uint64 kMaxDeferredDeletionQueues = 3;
        Array<DeferredDeletionQueue, kMaxDeferredDeletionQueues> deferredDeletionQueues_{};
        Vector<Descriptor, M3_MEM_CATEGORY(Render)> finalizedDeletionQueues_{};
    };
}
