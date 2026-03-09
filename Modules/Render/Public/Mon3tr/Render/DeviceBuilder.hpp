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
#include <Mon3tr/Render/RenderMinimal.hpp>

M3_DECLARE_LOG_CATEGORY(RenderDeviceBuilder)

namespace mon3tr {
    class Window;
}

namespace mon3tr::render {
    enum class EDeviceCreateResult : uint8 {
        Success,
        GraphicsAPINotSupportedFromPlatform,
        UnimplementedGraphicsAPI,
        FailedToCreateRHIDevice,

        /* begin D3D12 */
        FailedToCreateDXGIFactory,
        FailedToGetDXGIAdapter,
        FailedToCreateD3D12Device,
        FailedToCreateGraphicsCommandQueue,
        FailedToCreateComputeCommandQueue,
        FailedToCreateCopyCommandQueue,
        /* end D3D12 */
    };

    struct DeviceDesc {
        EGraphicsAPI TargetAPI = EGraphicsAPI::D3D12;
        bool bEnableValidationLayer = false;
    };

    class DeviceBuilder {
    public:
        static std::expected<nvrhi::DeviceHandle, EDeviceCreateResult> CreateDevice(const DeviceDesc& desc);

    private:
        static std::expected<nvrhi::DeviceHandle, EDeviceCreateResult> CreateDeviceD3D12(const DeviceDesc& desc);
    };
}
