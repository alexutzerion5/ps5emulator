# PS5 Emulator

A functional PS5 emulator written in C++17 with Vulkan backend.

## Features

- **CPU**: AMD Zen 2 x86-64 with AVX2/SSE4.2 support and JIT dynarec
- **GPU**: RDNA2 rasterizer with Vulkan backend
- **Memory**: 16GB GDDR6 emulation with page tables
- **I/O**: NVMe SSD and DualSense controller emulation
- **Firmware**: PKG decrypt and ELF loader support

## Requirements

- C++17 compatible compiler (GCC 9+, Clang 9+, MSVC 2019+)
- Vulkan SDK 1.2+
- CMake 3.20+
- 16GB RAM minimum

## Build Instructions

### Linux

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install build-essential cmake vulkan-sdk

# Clone and build
git clone https://github.com/yourusername/ps5-emulator.git
cd ps5-emulator
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./ps5_emulator
```

### Windows

```powershell
# Install dependencies
# - Visual Studio 2019 or later with C++ development tools
# - Vulkan SDK 1.2+
# - CMake 3.20+

# Build
mkdir build
cd build
cmake -G "Visual Studio 16 2019" -A x64 ..
cmake --build . --config Release

# Run
.\Release\ps5_emulator.exe
```

## Project Structure

```
ps5-emulator/
├── CMakeLists.txt
├── README.md
├── main.cpp
├── include/
│   ├── core/
│   │   ├── cpu/
│   │   │   ├── cpu.h
│   │   │   └── jit/
│   │   │       └── jit.h
│   │   ├── mmu/
│   │   │   └── mmu.h
│   │   ├── memory/
│   │   │   └── memory.h
│   │   ├── threading/
│   │   │   └── thread_pool.h
│   │   └── system/
│   │       ├── kernel.h
│   │       └── vsh.h
│   ├── core/io/
│   │   ├── nvme.h
│   │   └── dualsense.h
│   ├── loader/
│   │   ├── pkg_decrypt.h
│   │   └── elf_loader.h
│   └── gpu/
│       ├── vulkan/
│       │   ├── renderer.h
│       │   ├── shader_compiler.h
│       │   └── rop.h
│       └── rasterizer.h
└── core/
    ├── cpu/
    │   ├── cpu.cpp
    │   └── jit/
    │       └── jit.cpp
    ├── mmu/
    │   └── mmu.cpp
    ├── memory/
    │   └── memory.cpp
    ├── threading/
    │   └── thread_pool.cpp
    ├── system/
    │   ├── kernel.cpp
    │   └── vsh.cpp
    ├── io/
    │   ├── nvme.cpp
    │   └── dualsense.cpp
    ├── loader/
    │   ├── pkg_decrypt.cpp
    │   └── elf_loader.cpp
    └── gpu/
        ├── vulkan/
        │   ├── renderer.cpp
        │   ├── shader_compiler.cpp
        │   └── rop.cpp
        └── rasterizer.cpp
```

## Performance

- **Target**: >30 FPS on RTX 4090 + i9-13900K
- **Optimizations**: AVX512 support for JIT compilation

## License

MIT License

## Acknowledgments

- PS5 reverse engineering community
- Open source emulator projects