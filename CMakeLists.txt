cmake_minimum_required(VERSION 3.10)
project(uart_loader)

set(CMAKE_C_COMPILER clang)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -Wall -fdata-sections -ffunction-sections -g -m32 -fno-stack-protector")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fdata-sections -ffunction-sections -g -m32 -fno-stack-protector")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 --std='gnu2x")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --std=c++2a")


SET(SRCS
    ./link_mac.c
    ./main.c
)

include_directories(
    ./
)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

add_executable(${PROJECT_NAME}.out ${SRCS})


# IF (${RUN_PLATFORM} STREQUAL "ANDROID")
#     target_link_libraries(${PROJECT_NAME}.out ${pthread})
# ELSEIF(${RUN_PLATFORM} STREQUAL "LINUX")
    # target_link_libraries(${PROJECT_NAME}.out pthread)
# ENDIF()
