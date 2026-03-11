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
#include "FrameManager_DX12.hpp"
#include <Mon3tr/Render/SwapChain.hpp>

namespace mon3tr::render {
    EFrameManagerInitializeResult FrameManager_DX12::Initialize_Impl() {
        M3_PRE_COND(renderDevice_ != nullptr);
        M3_PRE_COND(swapChain_ != nullptr);

        ID3D12Device* nativeDevice = renderDevice_->getNativeObject(nvrhi::ObjectTypes::D3D12_Device);
        const HRESULT hr = nativeDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frameFence_));
        if (FAILED(hr)) {
            return EFrameManagerInitializeResult::FailedToCreateFrameFence;
        }

        frameEvents_.resize(swapChain_->GetBackBufferCount());
        for (uint32 backBufferIdx = 0; backBufferIdx < frameEvents_.size(); ++backBufferIdx) {
            // Signaled
            frameEvents_[backBufferIdx] = CreateEvent(nullptr, false, true, nullptr);
        }

        return EFrameManagerInitializeResult::Success;
    }

    void FrameManager_DX12::Shutdown() {
        for (const HANDLE frameEvent: frameEvents_) {
            WaitForSingleObject(frameEvent, INFINITE);
            CloseHandle(frameEvent);
        }
        frameEvents_.clear();

        frameFence_.Reset();

        FrameManager::Shutdown();
    }

    void FrameManager_DX12::SignalAllWaitEvents() {
        for (const HANDLE frameEvent: frameEvents_) {
            SetEvent(frameEvent);
        }
    }

    void FrameManager_DX12::BeginFrame() {
        M3_PRE_COND(IsInitialized());
        M3_PRE_COND(frameFence_ != nullptr);
        M3_PRE_COND(!frameEvents_.empty());

        WaitForSingleObject(frameEvents_[swapChain_->GetCurrentBackBufferIndex()], INFINITE);
    }

    void FrameManager_DX12::EndFrame() {
        M3_PRE_COND(IsInitialized());
        M3_PRE_COND(renderDevice_ != nullptr);
        M3_PRE_COND(frameFence_ != nullptr);

        const HANDLE targetEvent = frameEvents_[swapChain_->GetCurrentBackBufferIndex()];
        frameFence_->SetEventOnCompletion(renderFrameIdx_, targetEvent);
        ID3D12CommandQueue* graphicsQueue = renderDevice_->getNativeQueue(nvrhi::ObjectTypes::D3D12_CommandQueue, nvrhi::CommandQueue::Graphics);
        graphicsQueue->Signal(frameFence_.Get(), renderFrameIdx_);

        FrameManager::EndFrame();
    }
}
