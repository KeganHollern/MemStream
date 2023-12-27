# Object Structures

This example shows how to use the `memstream::dma::Object` class to read structures from remote processes.

## Result

This app will read the VTable start address from a global pointer.

## Note

It is possible to make child classes of Object for your specific needs.
This makes it possible to hide away the Push/Get procedures that enable dynamic classes.

```cpp
class World : public memstream::dma::Object {
    World(uint64_t addr, memestream::Process* proc) : memstream::dma::Object(proc) {
        this->Push(0x0, sizeof(uint64_t));
        this->Push(0x8, sizeof(int32_t));
        
        this->base = addr;
    }
    
    uint64_t GetVtable() {
        uint64_t result = 0;
        this->Get(0x0, result);
        return result;
    }
};


// World class comes with pre-pushed offsets at 0x0 and 0x8
void WorldExample() {
    World wrld(PROCESS);
    wrld.Read();
    uint64_t vtable = wrld.GetVtable();
    int32_t someInt;
    world.Get(0x8, someInt);
    
    std::cout << "0x0: 0x" << std::hex << vtable << std::endl;
    std::cout << "0x8: " << std::dec << someInt << std::endl;
}
```
