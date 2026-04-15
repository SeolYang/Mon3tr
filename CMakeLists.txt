cmake_minimum_required(VERSION 3.29)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(Mon3tr
        VERSION 0.1.0
        DESCRIPTION "High-performance minimal api set modular game engine"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin)

# Disable Exceptions
if (MSVC)
    string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    add_compile_options(
            "/EHs-"
            "/EHc-"
            "/Zc:preprocessor"
    )
    add_definitions(-D_HAS_EXCEPTIONS=0)

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options("-fno-exceptions")
endif ()

# Third-party
include(FetchContent)
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/ThirdParty")

set(SIMDUTF_TESTS OFF CACHE BOOL "" FORCE)
set(SIMDUTF_TOOLS OFF CACHE BOOL "" FORCE)
set(SIMDUTF_ICONV OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        simdutf
        GIT_REPOSITORY https://github.com/simdutf/simdutf.git
        GIT_TAG v8.0.0
)

set(WITH_WERROR OFF CACHE INTERNAL "")
set(JUST_INSTALL_CEREAL ON CACHE INTERNAL "")
set(SKIP_PORTABILITY_TEST ON CACHE INTERNAL "")
FetchContent_Declare(
        cereal
        GIT_REPOSITORY https://github.com/USCiLab/cereal.git
        GIT_TAG v1.3.2
)

add_definitions(-DSPDLOG_NO_EXCEPTIONS)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.17.0
)

FetchContent_Declare(
        magic_enum
        GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
        GIT_TAG v0.9.7
)

FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.92.5-docking
)

set(FLECS_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        flecs
        GIT_REPOSITORY https://github.com/SanderMertens/flecs.git
        GIT_TAG v4.1.4
)

option(TRACY_ENABLE "" ON)
FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG v0.13.1
)

FetchContent_Declare(
        JoltPhysics
        GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG v5.5.0
)

set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
set(SDL_GPU OFF CACHE BOOL "" FORCE)
set(SDL_OPENGL OFF CACHE BOOL "" FORCE)
set(SDL_OPENGLES OFF CACHE BOOL "" FORCE)
set(SDL_RENDER OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_D3D OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_D3D11 OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_D3D12 OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_GPU OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_VULKAN OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_VULKAN OFF CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
FetchContent_Declare(
        SDL
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.0
)

FetchContent_Declare(
        unordered_dense
        GIT_REPOSITORY https://github.com/martinus/unordered_dense.git
        GIT_TAG v4.8.1
)

FetchContent_Declare(
        directxmath
        GIT_REPOSITORY https://github.com/microsoft/DirectXMath.git
        GIT_TAG apr2025
)

FetchContent_Declare(
        vfspp
        GIT_REPOSITORY https://github.com/nextgeniuspro/vfspp.git
        GIT_TAG v2.1.1
)

set(NVRHI_INSTALL OFF CACHE BOOL "" FORCE)
set(NVRHI_WITH_DX11 OFF CACHE BOOL "" FORCE)
set(NVRHI_WITH_D3D11 OFF CACHE BOOL "" FORCE)
set(NVRHI_WITH_D3D12 ON CACHE BOOL "" FORCE)
set(NVRHI_WITH_VULKAN ON CACHE BOOL "" FORCE)
FetchContent_Declare(
        nvrhi
        GIT_REPOSITORY https://github.com/NVIDIA-RTX/NVRHI.git
        GIT_TAG 2ac4b58c355f53827c2e1d0740849ce4d09d5dd6
)

FetchContent_Declare(
        EASTL
        GIT_REPOSITORY https://github.com/electronicarts/EASTL.git
        GIT_TAG 3.27.01
)

set(SNMALLOC_HEADER_ONLY_LIBRARY ON CACHE BOOL "" FORCE)
set(SNMALLOC_STATIC_LIBRARY OFF CACHE BOOL "" FORCE)
set(SNMALLOC_BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        snmalloc
        GIT_REPOSITORY https://github.com/microsoft/snmalloc.git
        GIT_TAG 0.7.3
)

set(CROSSGUID_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        CrossGuid
        GIT_REPOSITORY https://github.com/graeme-hill/crossguid.git
        GIT_TAG ca1bf4b810e2d188d04cb6286f957008ee1b7681
)

FetchContent_Declare(
        simdjson
        GIT_REPOSITORY https://github.com/simdjson/simdjson.git
        GIT_TAG v4.6.0
)

FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
)

FetchContent_Declare(
        meshoptimizer
        GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
        GIT_TAG v1.1
)

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v6.0.4
)

FetchContent_MakeAvailable(simdutf cereal spdlog magic_enum imgui flecs tracy JoltPhysics SDL unordered_dense directxmath vfspp nvrhi EASTL snmalloc CrossGuid simdjson nlohmann_json meshoptimizer assimp)

FetchContent_Declare(
        dxc_binaries
        URL https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.9.2602/dxc_2026_02_20.zip
        URL_HASH SHA256=a1e89031421cf3c1fca6627766ab3020ca4f962ac7e2caa7fab2b33a8436151e # 실제 파일의 SHA256 해시 권장
)
FetchContent_MakeAvailable(dxc_binaries)

add_library(DXC::dxcompiler INTERFACE IMPORTED GLOBAL)

set(DXC_INCLUDE_DIR "${dxc_binaries_SOURCE_DIR}/inc")
set(DXC_LIB_DIR "${dxc_binaries_SOURCE_DIR}/lib/x64")
set(DXC_BIN_DIR "${dxc_binaries_SOURCE_DIR}/bin/x64")

set_target_properties(DXC::dxcompiler PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${DXC_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${DXC_LIB_DIR}/dxcompiler.lib;${DXC_LIB_DIR}/dxil.lib"
)

function(CopyDxcBinsToTarget TARGET_NAME)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DXC_BIN_DIR}/dxcompiler.dll"
            "${DXC_BIN_DIR}/dxil.dll"
            $<TARGET_FILE_DIR:${TARGET_NAME}>
            COMMENT "Copying DXC DLLs to binary directory for ${TARGET_NAME}..."
    )
endfunction()

# Third-party
add_subdirectory(ThirdParty/SimpleMath)

# Modules & Sub-projects
add_subdirectory(Modules/Core)
add_subdirectory(Modules/Render)
add_subdirectory(Modules/Asset)
add_subdirectory(Sandbox)