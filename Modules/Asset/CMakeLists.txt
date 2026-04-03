add_library(Mon3trAsset STATIC)

target_include_directories(
        Mon3trAsset
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# set(Mon3trAssetPrivate)
set(Mon3trAssetPublic
        Public/Mon3tr/Asset/Asset.hpp)

target_sources(Mon3trAsset PRIVATE
        ${Mon3trAssetPrivate})

target_sources(Mon3trAsset PUBLIC
        ${Mon3trAssetPublic})

# Dependencies
target_link_libraries(Mon3trAsset PUBLIC
        Mon3trCore
)

target_compile_options(Mon3trAsset PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)