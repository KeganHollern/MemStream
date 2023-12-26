# MemStream

MemStream is a wrapper for [MemProcFS](#) providing a simplified C++ interface for FPGA-based DMA application development.

## Usage

```c++
#include <cstdint>
#include <Process.h>

void example() {
    Process notepad("notepad.exe");
    uint64_t base = notepad.GetModuleBase("notepad.exe");
    uint8_t data = 0;
    if(!notepad.Read(base, &data, 1)) {
        printf("???");
    }
    printf("%x", data);
}
```

> Also see the [example](./example) directory.

## Building

Building for Linux is done via a docker toolchain. Run:

```shell
make linux
```

> TODO: windows x64 builds via windows docker toolchain...

## TODO

Those marked `?` I am unsure about including.

- [ ] Build compatibility for all MemProcFS targets
  - [ ] Windows AMD64
  - [x] Linux AMD64
  - [x] Linux ARM64
- [x] Refactor CMake projects
- [ ] MemProcFS submodule dependency / autosymbol stuff
- [x] Docker toolchains for build targets
- [ ] Github Actions for automated builds
- [ ] Complete Features
  - [ ] Dump Process
  - [x] Find Pattern
  - [ ] Find Code Cave
  - [ ] Refactor "Driver" Logic
  - [ ] Shellcode Injection ?
  - [ ] Function Calling ?
  - [ ] Inline Hooking ?
  - [ ] Library Manual Mapping ?
  - [ ] Thread Hijacking ?
  - [ ] Kernel Module Manual Mapping ?
  - [ ] Mono Dissection Utils ?
- [ ] Create Example Apps
  - [x] Basic example
  - [x] Input example
  - [ ] Performance test example
  - [ ] Real-world app example ?
- [ ] rework exceptions ?
