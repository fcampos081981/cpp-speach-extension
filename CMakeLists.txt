cmake_minimum_required(VERSION 3.16)
project(untitled)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(untitled main.cpp)
target_include_directories(untitled PRIVATE ${CMAKE_SOURCE_DIR})

# Expose source and build dirs to the program for robust resource lookup
target_compile_definitions(untitled PRIVATE
        PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
        PROJECT_BINARY_DIR="${CMAKE_BINARY_DIR}"
)

# If puppy.png exists in the source root, copy it next to the built executable
set(PUPPY_PNG "${CMAKE_SOURCE_DIR}/puppy.png")
if(EXISTS "${PUPPY_PNG}")
    add_custom_command(TARGET untitled POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PUPPY_PNG}"
            "$<TARGET_FILE_DIR:untitled>/puppy.png"
            COMMENT "Copying puppy.png next to the executable"
    )
endif()