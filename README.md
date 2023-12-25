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

1. create a `memprocfs` folder in [deps](./deps)
2. create a `lib` folder inside `memprocfs
3. place MemProcFS libraries (vmm & leechcore) in there.
4. create an `include` folder inside `memprocfs`
5. place MemProcFS headers (vmm.h & leechcore.h) in there.

Use CMake :)

## TODO

Those marked `?` I am unsure about including.

- [ ] Build compatibility for all MemProcFS targets
  - [ ] Windows AMD64
  - [ ] Linux AMD64
  - [ ] Linux ARM64
- [ ] Refactor CMake projects
- [ ] MemProcFS submodule dependency / autosymbol stuff
- [ ] Docker toolchains for build targets
- [ ] Github Actions for automated builds
- [ ] Complete Features
  - [ ] Dump Process
  - [ ] Find Pattern
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
  - [ ] Driver example (keyboard? mouse?)
  - [ ] Performance test example
  - [ ] Real-world app example ?
- [ ] rework exceptions ?
