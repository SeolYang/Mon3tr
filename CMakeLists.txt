cmake_minimum_required(VERSION 4.1)
project(Mon3tr
        VERSION 0.1.0
        DESCRIPTION "High-performance minimal api set modular game engine"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin)

# Disable Exceptions
if(MSVC)
    string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    add_compile_options(
            "/EHs-"
            "/EHc-"
    )
    add_definitions(-D_HAS_EXCEPTIONS=0)

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options("-fno-exceptions")
endif()

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
        GIT_REPOSITORY  https://github.com/gabime/spdlog.git
        GIT_TAG v1.17.0
)

FetchContent_Declare(
        magic_enum
        GIT_REPOSITORY  https://github.com/Neargye/magic_enum.git
        GIT_TAG v0.9.7
)

FetchContent_Declare(
        imgui
        GIT_REPOSITORY  https://github.com/ocornut/imgui.git
        GIT_TAG v1.92.5-docking
)

FetchContent_Declare(
        flecs
        GIT_REPOSITORY  https://github.com/SanderMertens/flecs.git
        GIT_TAG v4.1.4
)

FetchContent_Declare(
        DirectXTK12
        GIT_REPOSITORY  https://github.com/microsoft/DirectXTK12.git
        GIT_TAG oct2025
)

add_definitions(-DTF_DISABLE_EXCEPTION_HANDLING)
set(TF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TF_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        taskflow
        GIT_REPOSITORY  https://github.com/taskflow/taskflow.git
        GIT_TAG v4.0.0
)

option(TRACY_ENABLE "" ON)
FetchContent_Declare(
        tracy
        GIT_REPOSITORY  https://github.com/wolfpld/tracy.git
        GIT_TAG v0.13.1
)

FetchContent_Declare(
        JoltPhysics
        GIT_REPOSITORY  https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG v5.5.0
)

FetchContent_Declare(
        SDL
        GIT_REPOSITORY  https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.0
)

FetchContent_Declare(
        unordered_dense
        GIT_REPOSITORY  https://github.com/martinus/unordered_dense.git
        GIT_TAG v4.8.1
)

FetchContent_MakeAvailable(simdutf cereal spdlog magic_enum imgui flecs DirectXTK12 taskflow tracy JoltPhysics SDL unordered_dense)

# Modules & Sub-projects
add_subdirectory(Modules/Core)
add_subdirectory(Sandbox)