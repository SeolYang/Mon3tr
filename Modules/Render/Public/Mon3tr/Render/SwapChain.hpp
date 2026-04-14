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

namespace mon3tr {
    class Window;
}

namespace mon3tr::render {
    struct SwapChainDependency {
        Window*             WindowSystem = nullptr;
        nvrhi::DeviceHandle RenderDevice = nullptr;
    };

    struct SwapChainDesc {
        DXGI_USAGE Usage = DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT;
        uint32     BufferCount = 2;
        EFormat    Format = EFormat::SRGBA8_UNORM;
        // @ref https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_standard_multisample_quality_levels
        uint32 SampleCount = 1;   ///< MSAA Sample Counts
        uint32 SampleQuality = 0; ///< MSAA Sample Quality
        // bool   bAllowModeSwitch = false;

        uint32 RefreshRate = 60;

        bool bVsyncEnabled = false;
    };

    enum class ESwapChainInitializeResult : uint8 {
        Success,
        FailedAcquireNativeDevice,
        FailedToCreateBackBufferRenderTargets,

        /** D3D12 **/
        FailedToCreateDXGIFactory,
        FailedToCreateDXGISwapChain,
        FailedToQueryDXGISwapChain3,
    };

    enum class ESwapChainResizeResult : uint8 {
        Success,

        FailedToResizeSwapChain,
        FailedToCreateBackBufferRenderTargets,
    };

    class SwapChain : public System {
    public:
        ~SwapChain() override = default;

        [[nodiscard]] ESwapChainInitializeResult Initialize(const SwapChainDependency& dependency, const SwapChainDesc& desc);

        void Shutdown() override;

        virtual bool Present() = 0;

        static Ptr<SwapChain, M3_MEM_CATEGORY(Render)> Create(EGraphicsAPI graphicsAPI);

        // Handle Window Events -> bWindowResized = ?
        // 0. FrameManager::BeginFrame
        // 1. SwapChain::BeginFrame
        // 2. bWindowResized == true
        // 3. Device의 모든 작업 완료 까지 대기
        // 4. 스왑체인 백버퍼 리사이즈
        // 5. 렌더러 OnBackBufferResized (내부 렌더패스들의 리소스들 필요 시 재할당)
        // 6. FrameManager::SignalFrameEvents (3번에서 모든 작업이 완료되었기에, 만약을 위해)
        // (이 시점에서 모든 리사이즈 작업 완료)
        // 7. Renderer::Render
        // 8. SwapChain::EndFrame (Present)
        // 9. FrameManager::EndFrame
        [[nodiscard]] ESwapChainResizeResult Resize();

        [[nodiscard]] uint32 GetBackBufferCount() const noexcept { return desc_.BufferCount; }
        [[nodiscard]] uint32 GetCurrentBackBufferIndex() const noexcept { return currentBackBufferIdx_; }

        [[nodiscard]] nvrhi::ITexture*     GetCurrentBackBufferTexture() const { return renderTargets_[currentBackBufferIdx_]; }
        [[nodiscard]] nvrhi::IFramebuffer* GetCurrentBackBufferFramebuffer() const { return framebuffers_[currentBackBufferIdx_]; }

    protected:
        SwapChain() = default;

        virtual ESwapChainInitializeResult Initialize_Impl() = 0;

        virtual ESwapChainResizeResult Resize_Impl() = 0;

    protected:
        nvrhi::DeviceHandle                                       renderDevice_ = nullptr;
        Window*                                                   window_ = nullptr;
        SwapChainDesc                                             desc_;
        Vector<nvrhi::TextureHandle, M3_MEM_CATEGORY(Render)>     renderTargets_;
        Vector<nvrhi::FramebufferHandle, M3_MEM_CATEGORY(Render)> framebuffers_;
        uint32                                                    currentBackBufferIdx_ = 0;
    };
}
