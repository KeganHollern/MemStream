#include <cstdio>
#include <string>
#include <cstdint>
#include <chrono>
#include <thread>
#include <cassert>

#include <MemStream/Process.h>
#include <MemStream/DMA/Object.h>

using namespace memstream::dma;
using namespace memstream;

int main() {
    try {
        Process game("game.exe");
        Object obj(&game);
        obj.Push(0x0, 0x8); // push vtable offset

        // first our object should start as null
        assert(obj.IsNull());

        // get the object address - `somemod.dll+0x1000]`
        uint32_t object_offset = 0x10000;
        uint64_t object_address = game.GetModuleBase("somemod.dll")+object_offset;
        game.Read(object_address, obj.base);

        // obj should no longer be null as we read into obj.base...
        // it would be null if somemod.dll+0x1000] == NULL
        assert(!obj.IsNull());

        // read all offsets previously pushed (vtable)
        obj.Read();
        uint64_t vtable = 0;
        obj.Get(0x0, vtable);
    } catch (std::exception& ex) {
        printf("except: %s", ex.what());
        return 1;
    }
}
