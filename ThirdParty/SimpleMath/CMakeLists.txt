add_library(SimpleMath STATIC)

target_include_directories(
        SimpleMath
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

set(SimpleMathPrivate
        Private/SimpleMath.cpp
)

set(SimpleMathPublic
        Public/SimpleMath/SimpleMath.h
        Public/SimpleMath/SimpleMath.inl)

target_sources(SimpleMath PRIVATE
        ${SimpleMathPrivate})

target_sources(SimpleMath PUBLIC
        ${SimpleMathPublic})
