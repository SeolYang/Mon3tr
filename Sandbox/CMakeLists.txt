add_executable(Mon3trSandbox Private/Main.cpp)

target_include_directories(
        Mon3trSandbox
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

set(Mon3trSandboxPrivate
        Private/Application.cpp)
set(Mon3trSandboxPublic
        Public/Mon3tr/Sandbox/Application.hpp)

target_link_libraries(Mon3trSandbox
        PRIVATE
        Mon3trCore
        Mon3trRender
        Mon3trAsset
)

target_compile_options(Mon3trSandbox PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)