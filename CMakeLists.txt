cmake_minimum_required(VERSION 3.15)
project(fortress_broker)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(
        nanoid_cpp
        GIT_REPOSITORY https://github.com/mcmikecreations/nanoid_cpp.git
)

FetchContent_MakeAvailable(nanoid_cpp)

add_executable(fortress_broker broker.cpp)

target_include_directories(fortress_broker PRIVATE
        ${nanoid_cpp_SOURCE_DIR}/inc
)

target_link_libraries(fortress_broker PRIVATE ws2_32)
