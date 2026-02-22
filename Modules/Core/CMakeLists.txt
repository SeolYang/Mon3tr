add_library(Mon3trCore STATIC)

target_include_directories(
        Mon3trCore
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        ${enkits_SOURCE_DIR}/src
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

set(Mon3trCorePrivate
        Private/CoreMinimal.cpp
        Private/Log.cpp
        Private/Window.cpp)
set(Mon3trCorePublic
        Public/Mon3tr/Core/Assertion.hpp
        Public/Mon3tr/Core/CoreMinimal.hpp
        Public/Mon3tr/Core/Handle.hpp
        Public/Mon3tr/Core/HandleManager.hpp
        Public/Mon3tr/Core/Memory.hpp
        Public/Mon3tr/Core/Platform.hpp
        Public/Mon3tr/Core/Types.hpp
        Public/Mon3tr/Core/Version.hpp
        Public/Mon3tr/Core/VirtualArray.hpp
        Public/Mon3tr/Core/VirtualMemory.hpp
        Public/Mon3tr/Core/Log.hpp
        Public/Mon3tr/Core/System.hpp
        Public/Mon3tr/Core/Window.hpp)

target_sources(Mon3trCore PRIVATE
        ${Mon3trCorePrivate})

target_sources(Mon3trCore PUBLIC
        ${Mon3trCorePublic})

# Dependencies
target_link_libraries(Mon3trCore PUBLIC
        spdlog::spdlog
        cereal::cereal
        simdutf::simdutf
        magic_enum
        flecs::flecs
        unordered_dense::unordered_dense
        Tracy::TracyClient
        SimpleMath
        enkiTS
        vfspp::vfspp
        SDL3::SDL3
)

target_compile_options(Mon3trCore PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

# target_precompile_headers(Mon3trCore PUBLIC "Public/Mon3tr/Core/CoreMinimal.hpp")