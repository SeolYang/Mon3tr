add_library(Mon3trRender STATIC)

target_include_directories(
        Mon3trRender
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

set(Mon3trRenderPrivate
        Private/RenderMinimal.cpp
        Private/DeviceBuilder.cpp
        Private/SwapChain.cpp
        Private/SwapChain_DX12.hpp
        Private/SwapChain_DX12.cpp
        Private/FrameManager.cpp
        Private/FrameManager_DX12.hpp
        Private/FrameManager_DX12.cpp)
set(Mon3trRenderPublic
        Public/Mon3tr/Render/RenderMinimal.hpp
        Public/Mon3tr/Render/DeviceBuilder.hpp
        Public/Mon3tr/Render/SwapChain.hpp
        Public/Mon3tr/Render/FrameManager.hpp
        Public/Mon3tr/Render/Renderer.hpp)

target_sources(Mon3trRender PRIVATE
        ${Mon3trRenderPrivate})

target_sources(Mon3trRender PUBLIC
        ${Mon3trRenderPublic})

# Dependencies
target_link_libraries(Mon3trRender PUBLIC
        Mon3trCore
        nvrhi
        nvrhi_d3d12
        nvrhi_vk

        dxgi.lib
)

target_compile_options(Mon3trRender PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)