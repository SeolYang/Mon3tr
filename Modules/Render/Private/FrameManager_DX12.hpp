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
#include <Mon3tr/Render/FrameManager.hpp>

namespace mon3tr::render {
    class FrameManager_DX12 : public FrameManager {
        friend class FrameManager;

    public:
        ~FrameManager_DX12() override = default;

        void Shutdown() override;

        void SignalAllWaitEvents() override;

    private:
        FrameManager_DX12() = default;

        void BeginFrame() override;
        // After SwapChain->Present
        void EndFrame() override;

        EFrameManagerInitializeResult Initialize_Impl() override;

    private:
        nvrhi::RefCountPtr<ID3D12Fence1>        frameFence_;
        Vector<HANDLE, M3_MEM_CATEGORY(Render)> frameEvents_; ///< Event per each back-buffer of SwapChain
    };
}
