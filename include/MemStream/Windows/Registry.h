//
// Created by Kegan Hollern on 12/23/23.
//

#ifndef MEMSTREAM_REGISTRY_H
#define MEMSTREAM_REGISTRY_H

#if defined(_WIN32)
#if defined(MEMSTREAM_EXPORTS)
#define MEMSTREAM_API __declspec(dllexport)
#else
#define MEMSTREAM_API __declspec(dllimport)
#endif
#else
#if __GNUC__ >= 4
#define MEMSTREAM_API __attribute__ ((visibility ("default")))
#else
#define MEMSTREAM_API
#endif
#endif


#ifndef REG_NONE // on LINUX we use these hardcoded defs (on WIN we'll use winnt.h!)
#define REG_NONE		0	/* no type */
#define REG_SZ			1	/* string type (ASCII) */
#define REG_EXPAND_SZ		2	/* string, includes %ENVVAR% (expanded by caller) (ASCII) */
#define REG_BINARY		3	/* binary format, callerspecific */
/* YES, REG_DWORD == REG_DWORD_LITTLE_ENDIAN */
#define REG_DWORD		4	/* DWORD in little endian format */
#define REG_DWORD_LITTLE_ENDIAN	4	/* DWORD in little endian format */
#define REG_DWORD_BIG_ENDIAN	5	/* DWORD in big endian format  */
#define REG_LINK		6	/* symbolic link (UNICODE) */
#define REG_MULTI_SZ		7	/* multiple strings, delimited by \0, terminated by \0\0 (ASCII) */
#define REG_RESOURCE_LIST	8	/* resource list? huh? */
#define REG_FULL_RESOURCE_DESCRIPTOR	9	/* full resource descriptor? huh? */
#define REG_RESOURCE_REQUIREMENTS_LIST	10
#define REG_QWORD		11	/* QWORD in little endian format */
#define REG_QWORD_LITTLE_ENDIAN	11	/* QWORD in little endian format */
#endif

#include <cstdint>
#include <string>

#include <vmmdll.h>

#include <MemStream/FPGA.h>

namespace memstream::windows {

    enum class RegistryType {
        none = REG_NONE,
        sz = REG_SZ,
        expand_sz = REG_EXPAND_SZ,
        binary = REG_BINARY,
        dword = REG_DWORD,
        dword_little_endian = REG_DWORD_LITTLE_ENDIAN,
        dword_big_endian = REG_DWORD_BIG_ENDIAN,
        link = REG_LINK,
        multi_sz = REG_MULTI_SZ,
        resource_list = REG_RESOURCE_LIST,
        full_resource_descriptor = REG_FULL_RESOURCE_DESCRIPTOR,
        resource_requirements_list = REG_RESOURCE_REQUIREMENTS_LIST,
        qword = REG_QWORD,
        qword_little_endian = REG_QWORD_LITTLE_ENDIAN
    };

    class MEMSTREAM_API Registry {
    public:
        explicit Registry(FPGA *pFPGA);

        Registry();

        virtual ~Registry();

        bool Query(const std::string& path, const RegistryType& type, std::wstring& value);

    private:
        FPGA* pFPGA;
    };

}

#endif //MEMSTREAM_REGISTRY_H
