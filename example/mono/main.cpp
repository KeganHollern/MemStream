#include <cstdio>
#include <string>
#include <cstdint>
#include <chrono>
#include <thread>
#include <iostream>
#include <cassert>
#include <codecvt>
#include <algorithm>
#include <tuple>
#include <vector>
#include <sstream>
#include <iomanip>

#include <MemStream/FPGA.h>
#include <MemStream/Process.h>
#include <MemStream/Utils.h>
#include <MemStream/Windows/Input.h>

using namespace memstream;
using namespace memstream::windows;

#define PRNT(x) std::cout << std::hex << #x << ": 0x" << x << std::endl

inline unsigned short utf8_to_utf16( const char* val ) {
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    std::u16string dest = convert.from_bytes( val );
    return *reinterpret_cast<unsigned short*>( &dest[0] );
}

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
    uint64_t Domain_GetDomainAssemblies(uint64_t root_domain) {
        uint64_t result = 0;
        if(!proc.Read(root_domain + OFF_DomainAssemblies, result)) return 0;
        return result;
    }
    int Domain_GetDomainID(uint64_t root_domain) {
        int result = 0;
        if(!proc.Read(root_domain + OFF_DomainID, result)) return 0;
        return result;
    }
    uint64_t Domain_GetJittedFunctionTable(uint64_t root_domain) {
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
        uint64_t str_addr = 0;
        if(!proc.Read(data_base + OFF_DataDirectory, str_addr)) return {};

        char buffer[MAX_PATH] = {0};
        if(!proc.Read(str_addr, (uint8_t*)buffer, sizeof(buffer))) return {};
        buffer[sizeof(buffer)-1] = 0; // ensure null termination
        return buffer;
    }
    uint64_t AssemblyData_GetMonoImage(uint64_t data_base) {
        uint64_t result = 0;
        if(!proc.Read(data_base + OFF_DataMonoImage, result)) return 0;
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

        auto tableKey = this->HashTable_KeyExtract(v4);
        while(tableKey != key) {
            v4 = this->HashTable_NextValue(v4);
            if(!v4) return 0;
            tableKey = this->HashTable_KeyExtract(v4);
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
    std::string Class_Fullname(uint64_t class_base) {
        std::string fullName;
        std::string name = this->Class_Name(class_base);
        std::string ns = this->Class_Namespace(class_base);

        if(!ns.empty()) {
            fullName = ns.append(".").append(name);
        } else {
            fullName = name;
        }
        return fullName;
    }
    std::string Class_Name(uint64_t class_base) {
        uint64_t str_addr = 0;
        if(!proc.Read(class_base + 0x48, str_addr)) return {};

        char buffer[128] = {0};
        if(!proc.Read(str_addr, (uint8_t*)buffer, sizeof(buffer))) return {};
        buffer[sizeof(buffer)-1] = 0; // ensure null termination

        if((uint8_t)buffer[0] == 0xEE) { // if unicode...
            char name_buff[ 32 ] = {0};
            sprintf_s( name_buff, 32, "\\u%04X", utf8_to_utf16( const_cast<char*>( buffer ) ) );
            return name_buff;
        }

        return buffer;
    }
    std::string Class_Namespace(uint64_t class_base) {
        uint64_t str_addr = 0;
        if(!proc.Read(class_base + 0x50, str_addr)) return {};

        char buffer[128] = {0};
        if(!proc.Read(str_addr, (uint8_t*)buffer, sizeof(buffer))) return {};
        buffer[sizeof(buffer)-1] = 0; // ensure null termination

        if((uint8_t)buffer[0] == 0xEE) { // if unicode...
            char name_buff[ 32 ];
            sprintf_s( name_buff, 32, "\\u%04X", utf8_to_utf16( const_cast<char*>( buffer ) ) );
            return name_buff;
        }

        return buffer;
    }
    uint64_t Class_GetField(uint64_t class_base, uint32_t idx) {
        uint64_t fields_array = 0;
        if(!proc.Read(class_base + 0x98, fields_array)) return 0;
        return fields_array + (0x20 * idx);
    }
    //--- RUNTIME INFO
    int RuntimeInfo_MaxDomain(uint64_t runtimeinfo_base) {
        int result = 0;
        if(!proc.Read(runtimeinfo_base + 0x0, result)) return 0;
        return result;
    }
    uint64_t RuntimeInfo_GetVtable(uint64_t runtimeinfo_base, int domain) {
        uint64_t result = 0;
        if(!proc.Read(runtimeinfo_base + 0x8 + (0x8 * domain), result)) return 0;
        return result;
    }
    //--- VTABLE
    uint8_t VTable_GetFlags (uint64_t vtable_base) {
        uint8_t result = 0;
        if(!proc.Read(vtable_base + 0x30, result)) return 0;
        return result;
    }
    uint64_t VTable_GetStaticFieldData(uint64_t vtable_base) {
        uint64_t r1 = 0;
        if(!proc.Read(vtable_base, r1) || !r1) return 0;
        int r2 = 0;
        if(!proc.Read(r1 + 0x5c, r2)) return 0;
        uint64_t r3 = 0;
        if(!proc.Read(vtable_base + 0x40 + (0x8 * r2), r3)) return 0;
        return r3;
    }
    //--- FIELDS
    uint64_t Field_GetType(uint64_t field_base) {
        uint64_t result = 0;
        if(!proc.Read(field_base + 0x0, result)) return 0;
        return result;
    }
    int Field_GetOffset(uint64_t field_base) {
        int result = 0;
        if(!proc.Read(field_base + 0x18, result)) return 0;
        return result;
    }
    std::string Field_GetName(uint64_t field_base) {
        uint64_t str_addr = 0;
        if(!proc.Read(field_base + 0x8, str_addr)) return {};

        char buffer[128] = {0};
        if(!proc.Read(str_addr, (uint8_t*)buffer, sizeof(buffer))) return {};
        buffer[sizeof(buffer)-1] = 0; // ensure null termination

        if((uint8_t)buffer[0] == 0xEE) { // if unicode...
            char name_buff[ 32 ];
            sprintf_s( name_buff, 32, "\\u%04X", utf8_to_utf16( const_cast<char*>( buffer ) ) );
            return name_buff;
        }

        return buffer;
    }
    //--- Type Info
    uint64_t Type_GetClass(uint64_t type_base) {
        uint64_t result = 0;
        if(!proc.Read(type_base + 0x0, result)) return 0;
        return result;
    }
    uint64_t Type_GetAttribs(uint64_t type_base) {
        uint64_t result = 0;
        if(!proc.Read(type_base + 0x8, result)) return 0;
        return result;
    }

    // actually returns assembly->data
    uint64_t FindAssembly(uint64_t root_domain, const std::string& name) {
        auto assembly = this->Domain_GetDomainAssemblies(root_domain);
        while(assembly) {
            uint64_t data = this->DomainAssembly_GetData(assembly);
            std::string assemblyName;
            if(!data)
                goto next_item;

            assemblyName = this->AssemblyData_GetName(data);
            if(assemblyName == name)
                return data;

        next_item:
            assembly = this->DomainAssembly_GetNext(assembly);
        }

        return 0;
    }


    // user facing functions...
    std::vector<std::string> GetAssemblyNames() {
        std::vector<std::string> results;
        auto assembly = this->Domain_GetDomainAssemblies(this->GetRootDomain());
        while(assembly) {
            uint64_t data = this->DomainAssembly_GetData(assembly);
            std::string assemblyName;
            if(!data)
                goto next_item;

            assemblyName = this->AssemblyData_GetName(data);
            results.emplace_back(assemblyName);

            next_item:
            assembly = this->DomainAssembly_GetNext(assembly);
        }

        return results;
    }
    //TODO: avoid duplicate code and take in a uint64_t ASSEMBLY address
    std::vector<std::string> GetClasses(const std::string& assembly_name) {
        std::vector<std::string> results;
        auto root = this->GetRootDomain();
        if(!root) return results;
        auto assembly = this->FindAssembly(root, assembly_name);
        if(!assembly) return results;
        auto mono = this->AssemblyData_GetMonoImage(assembly);
        if(!mono) return results;
        auto table = this->MonoImage_GetTableInfo(mono, 2);
        if(!table) return results;
        auto row_count = this->TableInfo_GetRows(table);
        auto hashtable = this->MonoImage_GetHashTable(mono);
        for(int i = 0; i < row_count; i++) {
            uint64_t mono_class = this->HashTable_Lookup(hashtable, 0x02000000 | i + 1);
            if(!mono_class) continue;

            std::string fullName = this->Class_Fullname(mono_class);

            results.emplace_back(fullName);
        }
        return results;
    }

    uint64_t FindClass(const std::string& assembly_name, const std::string& class_name) {
        auto root = this->GetRootDomain();
        if(!root) return 0;
        auto assembly = this->FindAssembly(root, assembly_name);
        if(!assembly) return 0;
        auto mono = this->AssemblyData_GetMonoImage(assembly);
        if(!mono) return 0;
        auto table = this->MonoImage_GetTableInfo(mono, 2);
        if(!table) return 0;
        auto row_count = this->TableInfo_GetRows(table);
        auto hashtable = this->MonoImage_GetHashTable(mono);
        for(int i = 0; i < row_count; i++) {
            uint64_t mono_class = this->HashTable_Lookup(hashtable, 0x02000000 | i + 1);
            if(!mono_class) continue;

            std::string fullName;
            std::string name = this->Class_Name(mono_class);
            std::string ns = this->Class_Namespace(mono_class);

            if(!ns.empty()) {
                fullName = ns.append(".").append(name);
            } else {
                fullName = name;
            }

            if(fullName == class_name)
            {
                return mono_class;
            }
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
        std::cout << "Dumping EFT.Player from Tarkov..." << std::endl;
        Mono process("EscapeFromTarkov.exe");

        // get root domain
        auto root = process.GetRootDomain();
        auto domain_id = process.Domain_GetDomainID(root);
        // find hard settings
        uint64_t hardsettings_mono = process.FindClass("Assembly-CSharp", "EFTHardSettings");
        // extract RTI and ensure max_domain is >= domain_id
        auto rti = process.Class_RuntimeInfo(hardsettings_mono);
        if(!rti) throw std::runtime_error("unable to find hardsettings RTI");
        auto max_domain = process.RuntimeInfo_MaxDomain(rti);
        if(domain_id > max_domain) throw std::runtime_error("invalid vtable domain id!");

        auto vtable = process.RuntimeInfo_GetVtable(rti, domain_id);
        auto vt_flags = process.VTable_GetFlags(vtable);
        if((vt_flags & 4) == 0) throw std::runtime_error("bad rti flags!");
        auto static_data = process.VTable_GetStaticFieldData(vtable);
        std::cout << std::hex << "EFTHardSettings: 0x" << static_data << std::endl;


        // some debugging stuff
        uint64_t player_mono = process.FindClass("Assembly-CSharp", "EFT.Player");
        std::cout <<std::hex << "EFT.Player: 0x" << player_mono << std::endl;
        if(!player_mono) throw std::runtime_error("failed to find EFT.Player");


        // offset, name, type
        std::vector<std::tuple<uint64_t, std::string, std::string, uint64_t>> class_info;

        auto num_fields = process.Class_NumFields(player_mono);
        for(int i = 0; i < num_fields;i++) {
            auto field = process.Class_GetField(player_mono, i);
            if(!field) throw std::runtime_error("invalid field");
            auto name = process.Field_GetName(field);
            auto offset = process.Field_GetOffset(field);
            auto type = process.Field_GetType(field);



            std::string displayType = "Unknown Native Type";
            uint64_t attribs = 0;
            if(type) {
                auto type_class = process.Type_GetClass(type);
                attribs = process.Type_GetAttribs(type);
                if(type_class) {
                    displayType = process.Class_Fullname(type_class);
                }

            }

            class_info.emplace_back(offset, name, displayType, attribs);
        }


        std::sort(class_info.begin(), class_info.end(),
                  [](const std::tuple<uint64_t, std::string, std::string, uint64_t>& a,
                     const std::tuple<uint64_t, std::string, std::string, uint64_t>& b) -> bool {
                      // Compare first fields
                      if (std::get<0>(a) != std::get<0>(b))
                          return std::get<0>(a) < std::get<0>(b);
                      // If first fields are equal, compare second fields
                      return std::get<1>(a) < std::get<1>(b);
                  });

        for (const auto& tuple : class_info) {
            std::cout << "\t0x" << std::hex << std::get<0>(tuple) << ": " << std::get<1>(tuple) << " - " << std::get<2>(tuple)  << " - " << std::get<3>(tuple) << std::endl;
        }


    } catch (std::exception& ex) {
        printf("except: %s", ex.what());
        return 1;
    }
}