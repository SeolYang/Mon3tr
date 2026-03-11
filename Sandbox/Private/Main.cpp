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

#include <Mon3tr/Core/CoreMinimal.hpp>
#include <Mon3tr/Core/HandleManager.hpp>
#include <Mon3tr/Core/VirtualArray.hpp>
#include <Mon3tr/Core/Log.hpp>
#include <Mon3tr/Core/Window.hpp>
#include <Mon3tr/Core/GlobalTimer.hpp>
#include <Mon3tr/Render/DeviceBuilder.hpp>

#include <EASTL/vector.h>
#include <Mon3tr/Core/Allocator.hpp>
#include <Mon3tr/Core/Container.hpp>
#include <Mon3tr/Render/SwapChain.hpp>
#include <Mon3tr/Render/FrameManager.hpp>

M3_DECLARE_LOG_CATEGORY(TestLog);

M3_DEFINE_LOG_CATEGORY(TestLog);

int main() {
    namespace m3 = mon3tr;
    {
        mon3tr::GlobalTimer::GetInstance().BeginNewFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(550));
        mon3tr::GlobalTimer::GetInstance().BeginNewFrame();
        M3_LOG(TestLog, Trace, "DT: {}, {}", m3::GlobalTimer::GetInstance().GetDeltaTimeMilli(), m3::GlobalTimer::GetInstance().GetDeltaTime());

        m3::Ptr<m3::Window> window = m3::MakePtr<m3::Window>();
        const auto          windowInitResult = window->Initialize(m3::WindowDesc{.Title = "test", .Width = 1920, .Height = 1080, .bBorderless = false});
        if (windowInitResult != m3::EWindowInitializeResult::Success) {
            M3_LOG(TestLog, Fatal, "Failed to init window : {}", magic_enum::enum_name(windowInitResult));
        }

        auto deviceExpected = m3::render::DeviceBuilder::CreateDevice(m3::render::DeviceDesc{.TargetAPI = m3::render::EGraphicsAPI::D3D12});
        M3_ASSERT(deviceExpected.has_value());

        m3::render::SwapChainDependency swapChainDependency{.WindowSystem = window.get(), .RenderDevice = deviceExpected.value()};
        m3::render::SwapChainDesc       swapChainDesc{};
        auto                            swapChain = m3::render::SwapChain::Create(m3::render::EGraphicsAPI::D3D12);
        const auto                      swapChainInitResult = swapChain->Initialize(swapChainDependency, swapChainDesc);
        if (swapChainInitResult != m3::render::ESwapChainInitializeResult::Success) {
            M3_LOG(TestLog, Fatal, "Failed to init swap chain : {}", magic_enum::enum_name(swapChainInitResult));
        }

        m3::render::FrameManagerDependency frameManagerDependency{.RenderDevice = deviceExpected.value(), .SwapChainInstance = swapChain.get()};
        m3::render::FrameManagerDesc       frameManagerDesc{};
        auto                               frameManager = m3::render::FrameManager::Create(m3::render::EGraphicsAPI::D3D12);
        const auto                         frameManagerInitResult = frameManager->Initialize(frameManagerDependency, frameManagerDesc);
        if (frameManagerInitResult != m3::render::EFrameManagerInitializeResult::Success) {
            M3_LOG(TestLog, Fatal, "Failed to init frame manager: {}", magic_enum::enum_name(frameManagerInitResult));
        }

        M3_LOG(TestLog, Info, "Unspecified Category Memory Usage: {} bytes, Num Alloc: {}", M3_MEM_CATEGORY(Unspecified)::AllocationSize.load(),
               M3_MEM_CATEGORY(Unspecified)::NumAllocations.load());
        M3_LOG(TestLog, Info, "Core Category Memory Usage: {} bytes, Num Alloc: {}", M3_MEM_CATEGORY(Core)::AllocationSize.load(),
               M3_MEM_CATEGORY(Core)::NumAllocations.load());
        M3_LOG(TestLog, Info, "Render Category Memory Usage: {} bytes, Num Alloc: {}", M3_MEM_CATEGORY(Render)::AllocationSize.load(),
               M3_MEM_CATEGORY(Render)::NumAllocations.load());

        window->Resize(1280, 720);

        while (true) {
            m3::GlobalTimer::GetInstance().BeginNewFrame();
            window->SetTitle(std::format("Mon3tr Sandbox FPS: {} ({} ms)",
                                                        m3::GlobalTimer::GetInstance().GetFramesPerSecond(),
                                                        m3::GlobalTimer::GetInstance().GetDeltaTimeMilli()));

            bool bShouldExit = false;

            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    bShouldExit = true;
                    break;
                }
            }

            if (bShouldExit) {
                break;
            }

            if (window->HandleResize()) {
                // GPU 상에서 실행 중인, 또는 실행 대기중인 모든 명령어의 실행이 완료되어 리소스의 재할당이 안전해질때 까지 대기
                deviceExpected.value()->waitForIdle();
                deviceExpected.value()->runGarbageCollection();

                // 기존 백버퍼 렌더 타겟 해제->스왑체인 백버퍼 리사이즈->리사이즈된 백버퍼에 대한 신규 렌더 타겟 할당
                m3::render::ESwapChainResizeResult resizeResult = swapChain->Resize();
                M3_LOG(TestLog, Info, "Resize Result: {}", magic_enum::enum_name(resizeResult));
                // 그 외, 렌더 파이프라인 내에 리사이즈가 필요한 리소스들에 대한 조정

                // 앞서, 모든 명령어의 실행이 완료될때 까지 대기하였으므로, 혹여나 최종적으로 signal을 제때 받지 못한 이벤트에 대해
                // 대기하지 않도록 모든 프레임 이벤트에 신호를 준다.
                frameManager->SignalAllWaitEvents();
            }

            // game logic, etc..

            frameManager->BeginFrame();
            // render
            swapChain->Present();
            frameManager->EndFrame();
        } // End Main Loop

        // required for safe quit!!!!!
        deviceExpected.value()->waitForIdle();
        frameManager->SignalAllWaitEvents();

        // 앞선 처리가 없는 경우, 내부에서 frame event를 기다리느라 무한 대기 가능성 있음.
        frameManager->Shutdown();
        swapChain->Shutdown();
        window->Shutdown();
    }

    M3_LOG(TestLog, Info, "(After Exit) Unspecified Category Memory Usage: {} bytes, Num Alloc: {}", M3_MEM_CATEGORY(Unspecified)::AllocationSize.load(),
           M3_MEM_CATEGORY(Unspecified)::NumAllocations.load());
    M3_LOG(TestLog, Info, "(After Exit) Core Category Memory Usage: {} bytes, Num Alloc: {}", M3_MEM_CATEGORY(Core)::AllocationSize.load(),
           M3_MEM_CATEGORY(Core)::NumAllocations.load());
    M3_LOG(TestLog, Info, "(After Exit) Render Category Memory Usage: {} bytes, Num Alloc: {}", M3_MEM_CATEGORY(Render)::AllocationSize.load(),
           M3_MEM_CATEGORY(Render)::NumAllocations.load());

    m3::internal::ReportLiveRenderObjects(m3::render::EGraphicsAPI::D3D12);
    m3::internal::DumpMemoryLeaks();

    return 0;
}
