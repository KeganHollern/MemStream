#include "memstream.h"

#include <stdint.h>
#include <Zydis/Zydis.h>

#pragma pack(push, 1)
struct run_struct {
    uint8_t do_exec;
    uint64_t function_address;
    uint64_t arguments_pointer;
    uint64_t return_value;
    uint64_t single_arg_argument;
};
#pragma pack(pop)

HRESULT MSS_Runner_Malloc(PMSSFunctionRunner runner, size_t size, uint64_t* pAllocation) {
    if(!runner || !runner->process || !runner->process->ctx || !runner->process->ctx->hVMM)
        return E_UNEXPECTED;

    if(!size) return E_INVALIDARG;
    if(!pAllocation) return E_INVALIDARG;

    uint64_t mallocAddr = 0;
    HRESULT hr = MSS_GetModuleExport(runner->process,
                        "ucrtbase.dll",
                        "malloc",
                        &mallocAddr);
    if(FAILED(hr)) return hr;

    return MSS_Runner_Run(runner, mallocAddr, pAllocation, 1, size);
}


HRESULT MSS_Runner_Free(PMSSFunctionRunner runner, uint64_t address) {
    if(!runner || !runner->process || !runner->process->ctx || !runner->process->ctx->hVMM)
        return E_UNEXPECTED;

    if(!address) return E_INVALIDARG;

    uint64_t freeAddr = 0;
    HRESULT hr = MSS_GetModuleExport(runner->process,
                                     "ucrtbase.dll",
                                     "free",
                                     &freeAddr);
    if(FAILED(hr)) return hr;

    return MSS_Runner_Run(runner, freeAddr, NULL, 1, freeAddr);
}
HRESULT MSS_Runner_Run(PMSSFunctionRunner runner, uint64_t function, uint64_t* pReturn, uint32_t count, ...) {
    if(!runner || !runner->process || !runner->process->ctx || !runner->process->ctx->hVMM)
        return E_UNEXPECTED;

    if(!function) return E_INVALIDARG;

    uint64_t temp;
    if(!pReturn) {
        pReturn = &temp;
    }

    //TODO: run function by writing data properly


}


HRESULT MSS_InitRunner(
        PMSSProcess process,
        uint64_t data_address,
        uint64_t call_address,
        uint64_t text_address,
        PMSSFunctionRunner* pOutRunner) {

    if(!process || !process->ctx || !process->ctx->hVMM)
        return E_UNEXPECTED;

    if(!data_address) return E_INVALIDARG;
    if(!call_address) return E_INVALIDARG;
    if(!text_address) return E_INVALIDARG;
    if(!pOutRunner) return E_INVALIDARG;

    *pOutRunner = (PMSSFunctionRunner)malloc(sizeof(MSSFunctionRunner));
    (*pOutRunner)->process = process;
    (*pOutRunner)->call_address = call_address;
    (*pOutRunner)->text_address = text_address;
    (*pOutRunner)->data_address = data_address;

    // TODO: use zydis to decode the relcall address
    // ensure its a 5 byte call instruction

    // TODO: write default .data

    // TODO: use zydis to generate the .text code
    // write .text code

    // TODO: use zydis to generate the jmp code
    // write jmp code

    // generate code to JMP from call_address to text_address
    ZydisEncoderRequest req;
    ZeroMemory(&req, sizeof(ZydisEncoderRequest));
    req.mnemonic = ZYDIS_MNEMONIC_JMP;
    req.machine_mode = ZYDIS_MACHINE_MODE_LONG_64;
    req.operand_count = 1;
    req.operands[0].type = ZYDIS_OPERAND_TYPE_IMMEDIATE;
    req.operands[0].imm.u = text_address;

    ZyanU8 encoded_instruction[ZYDIS_MAX_INSTRUCTION_LENGTH];
    ZyanUSize encoded_length = sizeof(encoded_instruction);
    if (ZYAN_FAILED(
            ZydisEncoderEncodeInstructionAbsolute(
                    &req,
                    encoded_instruction,
                    &encoded_length,
                    call_address)))
    {
        return E_FAIL;
    }
    if(encoded_length != 5) {
        // i am retarded and encoded something wrong or something
        return E_UNKNOWN;
    }
    // TODO: write encoded_instructions to call_address


    return E_NOTIMPL;
}
