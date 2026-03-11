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
#include <Mon3tr/Render/SwapChain.hpp>
#include "SwapChain_DX12.hpp"

namespace mon3tr::render {
    ESwapChainInitializeResult SwapChain::Initialize(const SwapChainDependency& dependency, const SwapChainDesc& desc) {
        renderDevice_ = dependency.RenderDevice;
        window_ = dependency.WindowSystem;
        desc_ = desc;

        const ESwapChainInitializeResult result = Initialize_Impl();
        if (result == ESwapChainInitializeResult::Success) {
            MarkAsInitialized();
        }

        return result;
    }

    void SwapChain::Shutdown() {
        window_ = nullptr;
        desc_ = {};
        renderTargets_.clear();

        System::Shutdown();
    }

    Ptr<SwapChain, M3_MEM_CATEGORY(Render)> SwapChain::Create(const EGraphicsAPI graphicsAPI) {
        switch (graphicsAPI) {
            case EGraphicsAPI::D3D12:
                // @todo 현재 상태로는 너무 난잡하고, 다른 카테고리로 지정된 경우를 감지할 수 없음.
                // 다만 이 경우 Private 또는 Protected로 생성자를 숨긴 경우에만 복잡하고, 실제 Create<T, C>의 경우엔 더 간략화된 형태의 할당을 진행할 수 있으므로
                // 추후 커스텀 Ptr 클래스를 추가하여, debug 모드일 때 magic number 등을 검증하는 방식 등으로 대처할수있도록 고려할것.
                return Ptr<SwapChain, M3_MEM_CATEGORY(Render)>(
                    new(internal::CreateWithoutConstruct<SwapChain_DX12, M3_MEM_CATEGORY(Render)>()) SwapChain_DX12());
            default:
                M3_UNIMPLEMENTED();
                return nullptr;
        }
    }

    ESwapChainResizeResult SwapChain::Resize() {
        const ESwapChainResizeResult result = Resize_Impl();
        return result;
    }
}
