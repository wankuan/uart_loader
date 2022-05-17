cmake_minimum_required(VERSION 3.10)
set(CMAKE_C_COMPILER clang)
project(uart_loader)


# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -Wall -fdata-sections -ffunction-sections -g -m32 -fno-stack-protector")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fdata-sections -ffunction-sections -g -m32 -fno-stack-protector")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Og --std=gnu2x")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Og --std=c++2a")


SET(SRCS
    ./link_mac.c
    ./main.cpp
    ./bin_transfer/bin_transfer.cpp
    ./uart_port/uart_port.cpp
    ./uart_port/UartCore.cpp
    ./crc/crc32_mpeg2.c
)


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

add_executable(${PROJECT_NAME}.out ${SRCS})

target_include_directories(${PROJECT_NAME}.out PUBLIC
    ./
    ./bin_transfer
    ./uart_port
    ./crc
)