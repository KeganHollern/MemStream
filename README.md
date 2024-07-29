# MemStream (v2)

MemStream is a wrapper for [MemProcFS](https://github.com/ufrisk/MemProcFS) 
providing a simplified C++ interface for FPGA-based DMA application development.

## Usage

```c++
// connect to FPGA device
auto fpga = new memstream::v2::FPGA();

// print device information (version, configspace ect)
std::cout << fpga->getConfiguration()->to_string() << std::endl;
std::cout << "config space: " << std::endl;
memstream::v2::log::buffer(fpga->getConfiguration()->configSpace, 0x1000);
std::cout << std::endl;
 
// initialize v2 memstream input (more stable!)
input = new memstream::v2::Input(fpga);
input->OnKeyStateChange(victim_key_change);
input->OnMouseMove(victim_mouse_move);
```

> Also see the [example](./example) directory.

## TODO List

- [ ] Build compatibility for all MemProcFS targets
  - [x] Windows AMD64
  - [ ] Linux AMD64
  - [ ] Linux ARM64
- [ ] MemProcFS submodule
- [ ] Docker toolchains for build targets
- [ ] Github Actions for automated builds
- [ ] Complete Features
  - [x] Input Capture
  - [ ] Dump Process
  - [ ] Caching features (EAT/IAT/ect. - things that do not change)
  - [x] Find Pattern
  - [ ] Find Code Cave
  - [ ] Shellcode Injection
    - [ ] Function Calling
    - [ ] Inline Hooking
    - [ ] Library Manual Mapping
    - [ ] Thread Hijacking
    - [ ] Kernel Module Manual Mapping
  - [ ] Mono Dissection Utils
- [ ] Create Example Apps
  - [x] Basic example
  - [ ] Input example
  - [ ] [ReClass.NET](https://github.com/ReClassNET/ReClass.NET) Plugin
  - [ ] Performance test example
- [ ] MemProcFS optimizations
  - [ ] Improve cache refresh rate
  