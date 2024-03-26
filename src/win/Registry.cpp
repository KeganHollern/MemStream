#include <vmmdll.h>
#include <stdexcept>

#include "MemStream/FPGA.h"
#include "MemStream/Windows/Registry.h"



std::string memstream::windows::QueryValue(const char* path, e_registry_type type, memstream::FPGA* pFPGA)
{
	if (!pFPGA)
		return "";

	BYTE buffer[0x128];
	DWORD _type = static_cast<DWORD>(type);
	DWORD size = sizeof(buffer);

	if (!VMMDLL_WinReg_QueryValueExU(pFPGA->getVmm(), const_cast<LPSTR>(path), &_type, buffer, &size))
	{
        throw std::runtime_error("VMMDLL_WinReg_QueryValueExU FAILED");
	}

	std::wstring wstr = std::wstring(reinterpret_cast<wchar_t*>(buffer));
	return std::string(wstr.begin(), wstr.end());
}