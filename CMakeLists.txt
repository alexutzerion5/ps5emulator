cmake_minimum_required(VERSION 3.20)
project(PS5Emulator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Find Vulkan
find_package(Vulkan REQUIRED)

# Core library
add_library(ps5_core
    core/cpu/cpu.cpp
    core/cpu/jit/jit.cpp
    core/cpu/instructions.cpp
    core/mmu/mmu.cpp
    core/mmu/page_table.cpp
    core/threading/thread_pool.cpp
    core/threading/semaphore.cpp
    core/memory/memory.cpp
    core/memory/allocator.cpp
    core/system/system.cpp
    core/system/kernel.cpp
    core/system/vsh.cpp
    core/io/nvme.cpp
    core/io/dualsense.cpp
    core/io/io_manager.cpp
    loader/pkg_decrypt.cpp
    loader/elf_loader.cpp
    loader/ps5_firmware.cpp
    gpu/vulkan/renderer.cpp
    gpu/vulkan/shader_compiler.cpp
    gpu/vulkan/rop.cpp
    gpu/vulkan/command_buffer.cpp
    gpu/rasterizer.cpp
    main.cpp
)

target_include_directories(ps5_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ps5_core
    Vulkan::Vulkan
    ${CMAKE_DL_LIBS}
)

if(WIN32)
    target_link_libraries(ps5_core ws2_32)
else()
    target_link_libraries(ps5_core pthread)
endif()

# Emulator executable
add_executable(ps5_emulator
    main.cpp
)

target_link_libraries(ps5_emulator ps5_core)

# Installation
install(TARGETS ps5_emulator DESTINATION bin)