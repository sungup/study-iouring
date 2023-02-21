cmake_minimum_required(VERSION 3.7)
project(study-iouring)

set(CMAKE_C_STANDARD 11)

set(KERNEL_DIR "" CACHE STRING "Path to the kernel build directory")
if ("${KERNEL_DIR}" STREQUAL "")
    execute_process(COMMAND uname -r OUTPUT_VARIABLE KERNEL_VER
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(KERNEL_DIR "/usr/src/linux-headers-${KERNEL_VER}")
endif()

find_file(KERNEL_MAKEFILE NAMES Makefile
                          PATHS ${KERNEL_DIR} NO_DEFAULT_PATH)

if (NOT KERNEL_MAKEFILE)
    message(FATAL_ERROR "There is no makefile in kernel headers: ${KERNEL_DIR}")
endif()

# include_directories("${KERNEL_DIR}/include")

link_libraries(uring)

add_executable(iouring-c csrc/main.c)
