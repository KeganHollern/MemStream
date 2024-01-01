#include <cstdio>
#include <string>
#include <cstdint>
#include <chrono>
#include <thread>
#include <iostream>
#include <cassert>

#include <MemStream/FPGA.h>
#include <MemStream/Process.h>
#include <MemStream/Utils.h>
#include <MemStream/Windows/Input.h>
#include <MemStream/Windows/Registry.h>

using namespace memstream;
using namespace memstream::windows;

class Mono {
public:
    explicit Mono(const std::string& executable) : proc(executable) {
        mono_module = proc.GetModuleBase("mono-2.0-bdwgc.dll");
        if(!mono_module)
            throw std::runtime_error("unable to find mono-2.0.bdwgc.dll");
    }

    // ROOT_DOMAIN
    uint64_t GetRootDomain() {
        uint64_t result = 0;
        if(!proc.Read(mono_module + OFF_RootDomain, result)) return 0;
        return result;
    }
    // -- ROOT_DOMAIN GETTERS
    uint64_t RootDomain_GetDomainAssemblies(uint64_t root_domain) {
        uint64_t result = 0;
        if(!proc.Read(root_domain + OFF_DomainAssemblies, result)) return 0;
        return result;
    }
    int RootDomain_GetDomainID(uint64_t root_domain) {
        int result = 0;
        if(!proc.Read(root_domain + OFF_DomainID, result)) return 0;
        return result;
    }
    uint64_t RootDomain_GetJittedFunctionTable(uint64_t root_domain) {
        uint64_t result = 0;
        if(!proc.Read(root_domain + OFF_JittedFunctionTable, result)) return 0;
        return result;
    }
    //-- DOMAIN ASSEMBLIES
    uint64_t DomainAssembly_GetData(uint64_t assembly_base) {
        uint64_t result = 0;
        if(!proc.Read(assembly_base + OFF_AssemblyData, result)) return 0;
        return result;
    }
    uint64_t DomainAssembly_GetNext(uint64_t assembly_base) {
        uint64_t result = 0;
        if(!proc.Read(assembly_base + OFF_AssemblyNext, result)) return 0;
        return result;
    }
    //--- DATA
    std::string AssemblyData_GetName(uint64_t data_base) {
        uint64_t str_addr = 0;
        if(!proc.Read(data_base + OFF_DataName, str_addr)) return 0;
        char buffer[128] = {0};
        if(!proc.Read(str_addr, (uint8_t*)buffer, sizeof(buffer))) return {};
        buffer[sizeof(buffer)-1] = 0; // ensure null termination
        return buffer;
    }
    std::string AssemblyData_GetDirectory(uint64_t data_base) {
        char buffer[MAX_PATH] = {0};
        if(!proc.Read(data_base + OFF_DataDirectory, (uint8_t*)buffer, sizeof(buffer))) return std::string();
        buffer[sizeof(buffer)-1] = 0; // ensure null termination
        return std::string(buffer);
    }
    uint64_t AssemblyData_GetMonoImage(uint64_t data_base) {
        uint64_t result = 0;
        if(!proc.Read(data_base + OFF_AssemblyNext, result)) return 0;
        return result;

    }
    //--- MONO IMAGE
    int MonoImage_GetFlags(uint64_t image_base) {
        int result = 0;
        if(!proc.Read(image_base + OFF_MonoImageFlags, result)) return 0;
        return result;
    }
    uint64_t MonoImage_GetTableInfo(uint64_t image_base, uint32_t table_id) {
        if(table_id > 55) return 0;
        return (image_base + (0x10 * (table_id + 0xE)));
    }
    uint64_t MonoImage_GetHashTable(uint64_t image_base) {
        return image_base + OFF_HashTable;
    }
    uint64_t MonoImage_GetClass(uint64_t image_base, uint32_t type_id) {
        if ( ( type_id & 0xFF000000 ) != 0x2000000 )
            return 0;

        int flags = this->MonoImage_GetFlags(image_base);
        if((flags & 0x20) != 0)
            return 0;

        auto hashtable = this->MonoImage_GetHashTable(image_base);
        return this->HashTable_Lookup(hashtable, type_id);
    }

    //--- TABLE INFO
    int TableInfo_GetRows(uint64_t table_base) {
        int result = 0;
        if(!proc.Read(table_base + OFF_TableRows, result)) return 0;
        return result & 0xFFFFFF; // chop first byte because it's packed weirdly :)
    }
    //--- HASH TABLE
    int HashTable_GetSize(uint64_t hash_table) {
        int result = 0;
        if(!proc.Read(hash_table + OFF_HashTableSize, result)) return 0;
        return result;
    }
    uint64_t HashTable_GetData(uint64_t hash_table) {
        uint64_t result = 0;
        if(!proc.Read(hash_table + OFF_HashTableData, result)) return 0;
        return result;
    }
    uint64_t HashTable_NextValue(uint64_t hash_table) {
        uint64_t result = 0;
        if(!proc.Read(hash_table + OFF_HashTableNext, result)) return 0;
        return result;
    }
    uint32_t HashTable_KeyExtract(uint64_t hash_table) {
        uint32_t result = 0;
        if(!proc.Read(hash_table + OFF_HashTableKeyExtract, result)) return 0;
        return result;
    }
    uint64_t HashTable_Lookup(uint64_t hash_table, uint32_t key) {
        auto data = HashTable_GetData(hash_table);
        auto size = HashTable_GetSize(hash_table);
        uint64_t v4 = 0;
        if(!proc.Read(data + (0x8 * (key % size)), v4))
            return 0;

        while(this->HashTable_KeyExtract(v4) != key) {
            v4 = this->HashTable_NextValue(v4);
            if(!v4) return 0;
        }

        return v4;
    }
    //--- CLASS
    int Class_NumFields(uint64_t class_base) {
        int result = 0;
        if(!proc.Read(class_base + 0x100, result)) return 0;
        return result;
    }
    uint64_t Class_RuntimeInfo(uint64_t class_base) {
        uint64_t result = 0;
        if(!proc.Read(class_base + 0xd0, result)) return 0;
        return result;
    }


    uint64_t FindAssembly(uint64_t root_domain, const std::string& name) {
        auto assembly = this->RootDomain_GetDomainAssemblies(root_domain);
        while(assembly) {
            uint64_t data = this->DomainAssembly_GetData(assembly);
            std::string assemblyName;
            if(!data)
                goto next_item;

            assemblyName = this->AssemblyData_GetName(data);
            std::cout << "\tFound: " << assemblyName << std::endl;
            if(std::equal(name.begin(), name.end(), assemblyName.begin()))
                return data;

        next_item:
            assembly = this->DomainAssembly_GetNext(assembly);
        }

        return 0;
    }
    uint64_t FindClass(const std::string& assembly_name, const std::string class_name) {
        auto root = this->GetRootDomain();
        if(!root) return 0;
        auto domain = this->FindAssembly(root, assembly_name);
        if(!domain) return 0;
        auto mono = this->AssemblyData_GetMonoImage(domain);
        if(!mono) return 0;
        auto table = this->MonoImage_GetTableInfo(mono, 2);
        if(!table) return 0;
        auto row_count = this->TableInfo_GetRows(table);
        auto hashtable = this->MonoImage_GetHashTable(mono);
        for(int i = 0; i < row_count; i++) {
            uint64_t mono_class = this->HashTable_Lookup(hashtable, 0x02000000 | i + 1);
            if(!mono_class) continue;

        }
        return 0;
    }

private:
    const static uint32_t OFF_RootDomain = 0x499c78;
    const static uint32_t OFF_DomainAssemblies = 0xC8;
    const static uint32_t OFF_DomainID = 0xBC;
    const static uint32_t OFF_JittedFunctionTable = 0x148;
    const static uint32_t OFF_AssemblyData = 0x0;
    const static uint32_t OFF_AssemblyNext = 0x8;
    const static uint32_t OFF_DataDirectory = 0x10;
    const static uint32_t OFF_DataName = 0x10;
    const static uint32_t OFF_DataMonoImage = 0x60;
    const static uint32_t OFF_MonoImageFlags = 0x1C;
    const static uint32_t OFF_HashTable = 0x4C0;
    const static uint32_t OFF_TableRows = 0x8;
    const static uint32_t OFF_HashTableData = 0x20;
    const static uint32_t OFF_HashTableSize = 0x18;
    const static uint32_t OFF_HashTableNext = 0x108;
    const static uint32_t OFF_HashTableKeyExtract = 0x58;

    uint64_t mono_module{};
    Process proc;
};

int main() {
    try {
        FPGA* fpga = GetDefaultFPGA();
        uint64_t maj, min, dev = 0;
        fpga->getVersion(maj, min);
        dev = fpga->getDeviceID();

        std::cout << "FPGA: Device #" << dev << " v" << maj << "." << min << std::endl;

        // TODO: stdin the process name to dump mono for....

        std::cout << "Enter Process Name: ";
        std::string procname;
        std::cin >> procname;

        std::cout << "Dumping Mono for " << procname << std::endl;
        Mono process(procname);

        uint64_t root = process.GetRootDomain();
        std::cout << "ROOT: 0x" << std::hex << root << std::endl;

        uint64_t asmcsharp = process.FindAssembly(root, "Assembly-CSharp");
        std::cout <<std::hex << "Assembly-CSharp: 0x" << asmcsharp << std::endl;


    } catch (std::exception& ex) {
        printf("except: %s", ex.what());
        return 1;
    }
}