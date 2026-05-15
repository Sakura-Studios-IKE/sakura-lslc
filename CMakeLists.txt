cmake_minimum_required(VERSION 3.10)
project(lslc C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

file(GLOB LSLC_SRC src/*.c)

add_executable(lslc ${LSLC_SRC})

if(MSVC)
    target_compile_options(lslc PRIVATE /W4 /wd4244 /wd4267 /wd4996 /wd4100)
else()
    target_compile_options(lslc PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -Wno-unused-but-set-variable
        -Wno-unused-variable
        -Wno-unused-function)
endif()

install(TARGETS lslc RUNTIME DESTINATION bin)
