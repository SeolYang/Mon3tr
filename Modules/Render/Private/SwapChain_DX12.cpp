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
#include "SwapChain_DX12.hpp"
#include <Mon3tr/Render/RenderMinimal.hpp>
#include <Mon3tr/Core/Window.hpp>

#ifdef M3_PLATFORM_WINDOWS
namespace mon3tr::render {
    ESwapChainInitializeResult SwapChain_DX12::Initialize_Impl() {
        M3_PRE_COND(renderDevice_ != nullptr);
        M3_PRE_COND(renderDevice_->getGraphicsAPI() == EGraphicsAPI::D3D12);
        M3_PRE_COND(window_ != nullptr);

        uint32 factoryCreateFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
        factoryCreateFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
        HRESULT hr = CreateDXGIFactory2(factoryCreateFlags, IID_PPV_ARGS(&dxgiFactory_));
        if (FAILED(hr)) {
            return ESwapChainInitializeResult::FailedToCreateDXGIFactory;
        }

        ZeroMemory(&swapChainDesc_, sizeof(swapChainDesc_));
        swapChainDesc_.Width = static_cast<uint32>(window_->GetWidth());
        swapChainDesc_.Height = static_cast<uint32>(window_->GetHeight());
        swapChainDesc_.SampleDesc = {.Count = desc_.SampleCount, .Quality = desc_.SampleQuality};
        swapChainDesc_.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc_.BufferCount = desc_.BufferCount;
        swapChainDesc_.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc_.Flags = 0; // SDL에 mode 전환 완전 위임 //desc_.bAllowModeSwitch ? DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH : 0ui32;
        switch (desc_.Format) {
            case EFormat::SRGBA8_UNORM:
                swapChainDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                break;
            case EFormat::SBGRA8_UNORM:
                swapChainDesc_.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                break;
            default:
                swapChainDesc_.Format = nvrhi::d3d12::convertFormat(desc_.Format);
                break;
        }

        BOOL bTearingSupported = FALSE;
        if (FAILED(dxgiFactory_->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &bTearingSupported, sizeof(bTearingSupported)))) {
            // @todo warning/info log 메세지 추가?
        }

        if (bTearingSupported) {
            swapChainDesc_.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
            bSupportedTearing_ = true;
        }

        const nvrhi::RefCountPtr<ID3D12CommandQueue> graphicsQueue = static_cast<ID3D12CommandQueue*>(renderDevice_->getNativeQueue(
            nvrhi::ObjectTypes::D3D12_CommandQueue, nvrhi::CommandQueue::Graphics));

        nvrhi::RefCountPtr<IDXGISwapChain1> dxgiSwapChain1;
        hr = dxgiFactory_->CreateSwapChainForHwnd(graphicsQueue,
                                                  static_cast<HWND>(window_->GetNative()),
                                                  &swapChainDesc_,
                                                  nullptr, // Fullscreen Mode는 전적으로 SDL로 위임
                                                  nullptr,
                                                  &dxgiSwapChain1);
        if (FAILED(hr)) {
            return ESwapChainInitializeResult::FailedToCreateDXGISwapChain;
        }

        dxgiFactory_->MakeWindowAssociation(static_cast<HWND>(window_->GetNative()),
                                            DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_PRINT_SCREEN);

        hr = dxgiSwapChain1->QueryInterface(IID_PPV_ARGS(&dxgiSwapChain_));
        if (FAILED(hr)) {
            return ESwapChainInitializeResult::FailedToQueryDXGISwapChain3;
        }

        if (!CreateRenderTargets()) {
            return ESwapChainInitializeResult::FailedToCreateBackBufferRenderTargets;
        }

        return ESwapChainInitializeResult::Success;
    }

    ESwapChainResizeResult SwapChain_DX12::Resize_Impl() {
        M3_PRE_COND(window_ != nullptr);
        M3_PRE_COND(dxgiSwapChain_ != nullptr);
        M3_PRE_COND(!renderTargets_.empty());
        M3_PRE_COND(!nativeRenderTargets_.empty());

        renderTargets_.clear();
        nativeRenderTargets_.clear();

        const HRESULT hr = dxgiSwapChain_->ResizeBuffers(desc_.BufferCount,
                                                   window_->GetWidth(), window_->GetHeight(),
                                                   swapChainDesc_.Format,
                                                   swapChainDesc_.Flags);
        if (FAILED(hr)) {
            return ESwapChainResizeResult::FailedToResizeSwapChain;
        }
        dxgiSwapChain_->GetDesc1(&swapChainDesc_);
        M3_ASSERT(swapChainDesc_.Width == static_cast<uint32>(window_->GetWidth()) && swapChainDesc_.Height == static_cast<uint32>(window_->GetHeight()));

        if (!CreateRenderTargets()) {
            return ESwapChainResizeResult::FailedToCreateBackBufferRenderTargets;
        }

        return ESwapChainResizeResult::Success;
    }

    // @ref https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays
    // @ref donut framework @nvidia
    bool SwapChain_DX12::Present() {
        M3_PRE_COND(dxgiSwapChain_ != nullptr);
        M3_PRE_COND(!renderTargets_.empty());

        currentBackBufferIdx_ = dxgiSwapChain_->GetCurrentBackBufferIndex();

        uint32 presentFlags = 0;
        if (!desc_.bVsyncEnabled && bSupportedTearing_) {
            presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
        }

        const HRESULT hr = dxgiSwapChain_->Present(desc_.bVsyncEnabled ? 1 : 0, presentFlags);
        return SUCCEEDED(hr);
    }

    void SwapChain_DX12::Shutdown() {
        dxgiSwapChain_.Reset();
        nativeRenderTargets_.clear();
        bSupportedTearing_ = false;

        ZeroMemory(&swapChainDesc_, sizeof(swapChainDesc_));
        SwapChain::Shutdown();
    }

    bool SwapChain_DX12::CreateRenderTargets() {
        M3_PRE_COND(nativeRenderTargets_.empty());
        M3_PRE_COND(renderTargets_.empty());
        nativeRenderTargets_.resize(desc_.BufferCount);
        renderTargets_.reserve(desc_.BufferCount);

        const nvrhi::TextureDesc backBufferDesc = nvrhi::TextureDesc()
                .setDimension(nvrhi::TextureDimension::Texture2D)
                .setFormat(desc_.Format)
                .setWidth(window_->GetWidth())
                .setHeight(window_->GetHeight())
                .setIsRenderTarget(true)
                .setSampleCount(desc_.SampleCount)
                .setSampleQuality(desc_.SampleQuality)
                .setInitialState(nvrhi::ResourceStates::Present)
                .setKeepInitialState(true)
                .setDebugName("BackBufferTex");

        for (uint32 idx = 0; idx < desc_.BufferCount; ++idx) {
            const HRESULT hr = dxgiSwapChain_->GetBuffer(idx, IID_PPV_ARGS(&nativeRenderTargets_[idx]));
            if (FAILED(hr)) {
                return false;
            }

            const nvrhi::TextureHandle backBuffer = renderDevice_->createHandleForNativeTexture(
                nvrhi::ObjectTypes::D3D12_Resource, nvrhi::Object(nativeRenderTargets_[idx]), backBufferDesc);
            if (backBuffer == nullptr) {
                return false;
            }
            renderTargets_.emplace_back(backBuffer);
        }

        return true;
    }
}
#endif
