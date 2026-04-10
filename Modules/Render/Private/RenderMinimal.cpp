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
#include <Mon3tr/Render/RenderMinimal.hpp>
#include <dxgidebug.h>

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 619; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

M3_DEFINE_MEM_CATEGORY(Render)

namespace mon3tr::internal {
    // dxguid.lib required
    void ReportLiveObjects_D3D12() {
#ifdef _DEBUG
        nvrhi::RefCountPtr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)))) {
            [[maybe_unused]] const HRESULT result =
                    dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS) (DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        }
#endif
    }

    void ReportLiveRenderObjects(const render::EGraphicsAPI api) {
        switch (api) {
            case render::EGraphicsAPI::D3D12:
                internal::ReportLiveObjects_D3D12();
                break;
            default:
                M3_UNIMPLEMENTED();
        }
    }
}
