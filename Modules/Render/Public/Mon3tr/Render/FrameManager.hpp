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
#include <Mon3tr/Core/System.hpp>
#include <Mon3tr/Render/RenderMinimal.hpp>

namespace mon3tr::render {
    struct FrameManagerDependency {
        nvrhi::DeviceHandle RenderDevice = nullptr;
        class SwapChain*    SwapChainInstance = nullptr;
    };

    struct FrameManagerDesc {
    };

    enum class EFrameManagerInitializeResult : uint8 {
        Success,
        FailedToCreateFrameFence
    };

    class FrameManager : public System {
    public:
        ~FrameManager() override = default;

        virtual void BeginFrame() = 0;

        virtual void EndFrame() { ++renderFrameIdx_; }

        [[nodiscard]] static Ptr<FrameManager, M3_MEM_CATEGORY(Render)> Create(EGraphicsAPI graphicsAPI);

        [[nodiscard]] EFrameManagerInitializeResult Initialize(const FrameManagerDependency& dependency, const FrameManagerDesc& desc);

        virtual void Shutdown() override;

        virtual void SignalAllWaitEvents() = 0;

    protected:
        FrameManager() = default;

        virtual EFrameManagerInitializeResult Initialize_Impl() = 0;

    protected:
        nvrhi::DeviceHandle renderDevice_ = nullptr;
        SwapChain* swapChain_ = nullptr;
        uint64 renderFrameIdx_ = 1;
    };
}
