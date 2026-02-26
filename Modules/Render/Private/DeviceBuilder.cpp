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
#include <Mon3tr/Render/DeviceBuilder.hpp>
#if defined(M3_PLATFORM_WINDOWS)
#include <nvrhi/d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3d12sdklayers.h>
#endif

M3_DEFINE_LOG_CATEGORY(RenderDeviceBuilder)

namespace mon3tr::render {
    std::expected<nvrhi::DeviceHandle, EDeviceCreationResult> DeviceBuilder::CreateDevice(const DeviceDesc& desc) {
        switch (desc.TargetAPI) {
            case EGraphicsAPI::D3D12:
#if defined(M3_PLATFORM_WINDOWS)
                return CreateDeviceD3D12(desc);
#else
                return std::unexpected(EDeviceCreationResult::GraphicsAPINotSupportedFromPlatform);
#endif
            default:
                return std::unexpected(EDeviceCreationResult::UnimplementedGraphicsAPI);
        }
    }

#if defined(M3_PLATFORM_WINDOWS)
    std::expected<nvrhi::DeviceHandle, EDeviceCreationResult> DeviceBuilder::CreateDeviceD3D12([[maybe_unused]] const DeviceDesc& desc) {
        uint32 factoryCreationFlags = 0;
        {
#if defined(DEBUG) || defined(_DEBUG)
            factoryCreationFlags |= DXGI_CREATE_FACTORY_DEBUG;
            nvrhi::RefCountPtr<ID3D12Debug6> debugController;
            const bool bDebugControllerAcquired = SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
            if (!bDebugControllerAcquired)
            {
                M3_LOG(RenderDeviceBuilder, Warning, "Failed to get debug controller.");
            }
            if (bDebugControllerAcquired)
            {
                debugController->EnableDebugLayer();
                M3_LOG(RenderDeviceBuilder, Info, "D3D12 debug layer enabled.");
                /* @ref https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-d3d12-debug-layer-gpu-based-validation */
                debugController->SetEnableGPUBasedValidation(true);
                M3_LOG(RenderDeviceBuilder, Info, "D3D12 gpu based validation enabled.");
            }
#endif
        }

        nvrhi::RefCountPtr<IDXGIFactory6> factory;
        const bool bFactoryCreated = SUCCEEDED(CreateDXGIFactory2(factoryCreationFlags, IID_PPV_ARGS(&factory)));
        if (!bFactoryCreated)
        {
            return std::unexpected(EDeviceCreationResult::FailedToCreateDXGIFactory);
        }

        nvrhi::RefCountPtr<IDXGIAdapter> adapter;
        const bool bIsAdapterAcquired =
            SUCCEEDED(factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)));
        if (!bIsAdapterAcquired)
        {
            return std::unexpected(EDeviceCreationResult::FailedToGetDXGIAdapter);
        }

        // 최소 NVIDIA Turing 아키텍처 이후 GPU를 타겟하므로, 가능하다고 가정.
        constexpr D3D_FEATURE_LEVEL kMinimumFeatureLevel = D3D_FEATURE_LEVEL_12_2;
        nvrhi::RefCountPtr<ID3D12Device14> nativeDevice;
        if (!SUCCEEDED(D3D12CreateDevice(adapter.Get(), kMinimumFeatureLevel, IID_PPV_ARGS(&nativeDevice))))
        {
            return std::unexpected(EDeviceCreationResult::FailedToCreateD3D12Device);
        }

#if defined(_DEBUG) || defined(ENABLE_GPU_VALIDATION)
        nvrhi::RefCountPtr<ID3D12InfoQueue> infoQueue;
        if (SUCCEEDED(nativeDevice->QueryInterface(IID_PPV_ARGS(&infoQueue))))
        {
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
        }
        else
        {
            M3_LOG(RenderDeviceBuilder, Warning, "Failed to query a info queue from the device.");
        }
#endif

        D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {
            .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
            .NodeMask = 0x1
        };

        nvrhi::RefCountPtr<ID3D12CommandQueue> graphicsCommandQueue;
        if (!SUCCEEDED(nativeDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&graphicsCommandQueue)))) {
            return std::unexpected(EDeviceCreationResult::FailedToCreateGraphicsCommandQueue);
        }

        nvrhi::RefCountPtr<ID3D12CommandQueue> computeCommandQueue;
        commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        if (!SUCCEEDED(nativeDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&computeCommandQueue)))) {
            return std::unexpected(EDeviceCreationResult::FailedToCreateComputeCommandQueue);
        }

        nvrhi::RefCountPtr<ID3D12CommandQueue> copyCommandQueue;
        commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
        if (!SUCCEEDED(nativeDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&copyCommandQueue)))) {
            return std::unexpected(EDeviceCreationResult::FailedToCreateCopyCommandQueue);
        }

        const nvrhi::d3d12::DeviceDesc deviceDescD3D12 {
        .errorCB = nullptr,
            .pDevice =  nativeDevice.Get(),
            .pGraphicsCommandQueue = graphicsCommandQueue.Get(),
            .pComputeCommandQueue = computeCommandQueue.Get(),
            .pCopyCommandQueue = copyCommandQueue.Get(),
            .enableHeapDirectlyIndexed = true, // 최소 NVIDIA Turing 아키텍처 이후 GPU를 타겟하므로, 지원한다고 가정.
        };

        nvrhi::DeviceHandle device = nvrhi::d3d12::createDevice(deviceDescD3D12);
        if (device == nullptr) {
            return std::unexpected(EDeviceCreationResult::FailedToCreateRHIDevice);
        }
        return device;
    }
#endif
}
