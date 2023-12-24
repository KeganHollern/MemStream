# MemStream

MemStream is a wrapper for [MemProcFS](#) providing a simplified C++ interface for FPGA-based DMA application development.

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
