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
#include <Mon3tr/Render/FrameManager.hpp>
#include "FrameManager_DX12.hpp"

namespace mon3tr::render {
    Ptr<FrameManager, M3_MEM_CATEGORY(Render)> FrameManager::Create(EGraphicsAPI graphicsAPI) {
        switch (graphicsAPI) {
            case EGraphicsAPI::D3D12:
                return Ptr<FrameManager, M3_MEM_CATEGORY(Render)>{
                    new(internal::CreateWithoutConstruct<FrameManager_DX12, M3_MEM_CATEGORY(Render)>()) FrameManager_DX12()
                };
            default:
                M3_UNIMPLEMENTED();
                return nullptr;
        }
    }

    EFrameManagerInitializeResult FrameManager::Initialize(const FrameManagerDependency& dependency, [[maybe_unused]] const FrameManagerDesc& desc) {
        M3_PRE_COND(dependency.RenderDevice != nullptr);
        M3_PRE_COND(dependency.SwapChainInstance != nullptr);

        renderDevice_ = dependency.RenderDevice;
        swapChain_ = dependency.SwapChainInstance;

        const EFrameManagerInitializeResult result = Initialize_Impl();
        if (result == EFrameManagerInitializeResult::Success) {
            MarkAsInitialized();
        }

        return result;
    }

    void FrameManager::Shutdown() {
        swapChain_ = nullptr;
        renderFrameIdx_ = 1;

        System::Shutdown();
    }
}
