add_library(Mon3trRender STATIC)

target_include_directories(
        Mon3trRender
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

set(Mon3trRenderPrivate
        Private/DeviceBuilder.cpp)
set(Mon3trRenderPublic
        Public/Mon3tr/Render/DeviceBuilder.hpp)

target_sources(Mon3trRender PRIVATE
        ${Mon3trRenderPrivate})

target_sources(Mon3trRender PUBLIC
        ${Mon3trRenderPublic})

# Dependencies
target_link_libraries(Mon3trRender PUBLIC
        Mon3trCore
        nvrhi
)

target_compile_options(Mon3trRender PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)