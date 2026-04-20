add_library(Mon3trRender STATIC)

target_include_directories(
        Mon3trRender
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_BINARY_DIR}/AgilitySDK/build/native/include
)

set(Mon3trRenderPrivate
        Private/RenderMinimal.cpp
        Private/DeviceBuilder.cpp
        Private/SwapChain.cpp
        Private/SwapChain_DX12.hpp
        Private/SwapChain_DX12.cpp
        Private/FrameManager.cpp
        Private/FrameManager_DX12.hpp
        Private/FrameManager_DX12.cpp
        Private/RenderGraph.cpp
        Private/ShaderConstants.hpp
        Private/Shader.cpp
        Private/ShaderCompiler.cpp
        Private/ShaderLoader.cpp
        Private/GpuStorage.cpp
        Private/DescriptorManager.cpp)
set(Mon3trRenderPublic
        Public/Mon3tr/Render/RenderMinimal.hpp
        Public/Mon3tr/Render/DeviceBuilder.hpp
        Public/Mon3tr/Render/SwapChain.hpp
        Public/Mon3tr/Render/FrameManager.hpp
        Public/Mon3tr/Render/Renderer.hpp
        Public/Mon3tr/Render/RenderGraph.hpp
        Public/Mon3tr/Render/Shader.hpp
        Public/Mon3tr/Render/ShaderCompiler.hpp
        Public/Mon3tr/Render/ShaderLoader.hpp
        Public/Mon3tr/Render/Quantization.hpp
        Public/Mon3tr/Render/Vertex.hpp
        Public/Mon3tr/Render/GpuStorage.hpp
        Public/Mon3tr/Render/DescriptorManager.hpp)

target_sources(Mon3trRender PRIVATE
        ${Mon3trRenderPrivate})

target_sources(Mon3trRender PUBLIC
        ${Mon3trRenderPublic})

FetchContent_Declare(
        AgilitySDK
        URL "https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.619.1"
        SOURCE_DIR ${CMAKE_BINARY_DIR}/AgilitySDK
)
FetchContent_MakeAvailable(AgilitySDK)

function(CopyAgilitySdkBinsToTarget TARGET_NAME)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${TARGET_NAME}>/D3D12
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/AgilitySDK/build/native/bin/x64/D3D12Core.dll"
            "${CMAKE_BINARY_DIR}/AgilitySDK/build/native/bin/x64/d3d12sdklayers.dll"
            $<TARGET_FILE_DIR:${TARGET_NAME}>/D3D12/
            COMMENT "Copying Agility SDK binaries to output directory..."
    )
endfunction()

# Dependencies
target_link_libraries(Mon3trRender PUBLIC
        Mon3trCore
        Mon3trAsset
        nvrhi
        nvrhi_d3d12
        nvrhi_vk

        dxgi.lib
        dxguid.lib

        PRIVATE
        DXC::dxcompiler
        meshoptimizer
        assimp
)

target_compile_options(Mon3trRender PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)