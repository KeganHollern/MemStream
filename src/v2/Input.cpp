#include <MemStream/v2/_macros.h>
#include <MemStream/v2/Input.h>

#include <print>
#include <algorithm>
#include <stdexcept>

const std::string MODULE_WIN32KBASE = "win32kbase.sys";
const std::string MODULE_WIN32KSGD = "win32ksgd.sys";

using namespace memstream::v2;

#define READ_KEYSTATE(STATE, KEY) \
    (STATE[(KEY * 2 / 8)] & 1 << KEY % 4 * 2)

Input::Input(FPGA* fpga) {
    _fpga = fpga;

    // get windows version
    bool isW11 = getWindowsVersion() > 22000;

    // find our csrss session
    auto pids = _fpga->GetAllProcessIds("csrss.exe");
    std::sort(pids.begin(), pids.end()); 
    if(pids.size() >= 2) {
        pids.erase( pids.begin() ); // remove first pid (system)
    }

    for(const auto& pid : pids) {
        LOG_ERR("attaching to csrss.exe with pid {}", pid);
        _kernel.process = new Process(_fpga, pid, true);
        auto base = _kernel.process->GetModuleBase(MODULE_WIN32KBASE);
        if(base) {
            if(!isW11) {
                break; // W10 valid session process
            } else {
                base = _kernel.process->GetModuleBase(MODULE_WIN32KSGD);
                if(base) {
                    break; // W11 valid session process
                } else {
                    LOG_ERR("invalid csrss.exe with pid {} (wksgd)", pid);
                }
            }
        } else {
            LOG_ERR("invalid csrss.exe with pid {} (wkbase)", pid);
        }
        delete _kernel.process;
        _kernel.process = nullptr;
    }
    
    // not finding a valid session is WEIRD!
    if( !_kernel.process ) {
        LOG_ERR("failed to find kernel session");
        throw std::runtime_error("failed to find kernel session");
    }

    // find cursor address ( all win versions are at export in win32kbase.sys )
    _kernel.gptCursorAsync = _kernel.process->GetExport(MODULE_WIN32KBASE, "gptCursorAsync");
    if(!_kernel.gptCursorAsync) {
        LOG_ERR("failed to find gptCursorAsync");
        throw std::runtime_error("failed to find gptCursorAsync");
    }
    
    // find keyboard address
    // requires determining windows version
    // of victim machine for different algorithm on win11+
    if(isW11) {
        // check win32ksgd exists
        auto base = _kernel.process->GetModuleBase(MODULE_WIN32KSGD);
        if(base) {
            uint64_t r1, r2, r3 = 0;
            _fpga->Read(_kernel.process->GetPID(), base + 0x3110, r1);
            if(r1) {
                // NOTE: i expect 0 will always work                                                                            
                // but i've had mixed experience on w11 - kegan
                for(int i = 0; i < 8; i++) {    
                    _fpga->Read(_kernel.process->GetPID(), r1 + (i * 8), r2);       
                    if(r2) break;
                }
                if(r2) {
                    _fpga->Read( _kernel.process->GetPID(), r2, r3);
                    if(r3) {
                        // search win32kbase.sys for this pattern:
                        //  48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 8B F9 
                        // TODO: can we make a pattern for the value below ? will we be able to actually findpattern it ?
                        if(getWindowsVersion() >= 22631) {
                            // latest kernel value
                            _kernel.gafAsyncKeyStateAddr = r3 + 0x36A8;
                        } else {
                            // older windows 11 kernels
                            std::println("old windows 11 kernel detected!");
                            _kernel.gafAsyncKeyStateAddr = r3 + 0x3690; 
                        }
                    } else {
                        LOG_ERR("nullptr at win32ksgd.sys + 0x3110]]]");
                    }
                } else {
                    LOG_ERR("nullptr at win32ksgd.sys + 0x3110]]");
                }
            } else {
                LOG_ERR("nullptr at win32ksgd.sys + 0x3110]");
            }
        } else {
            LOG_ERR("failed to find win32ksgd.sys");
        }
    } else {
        // win32kbase
        _kernel.gafAsyncKeyStateAddr = _kernel.process->GetExport(MODULE_WIN32KBASE, "gafAsyncKeyState");
    }
    if(!_kernel.gafAsyncKeyStateAddr) {
        LOG_ERR("failed to find gafAsyncKeyState");
        throw std::runtime_error("failed to find gafAsyncKeyState");
    }

    // one last sanity check
    if( _kernel.gafAsyncKeyStateAddr <= 0x7FFFFFFFFFFF || _kernel.gptCursorAsync <= 0x7FFFFFFFFFFF ) {
        LOG_ERR("kernel addresses are invalid, keyboard: {}, cursor: {}", _kernel.gafAsyncKeyStateAddr, _kernel.gptCursorAsync);
        throw std::runtime_error("kernel addresses are invalid");
    }

    // read initial state
    std::println("kernel addresses - asyncKeyState: {}, cursorAsync: {}", (void*)_kernel.gafAsyncKeyStateAddr, _kernel.gptCursorAsync);
    _kernel.process->PrepareRead(_kernel.gafAsyncKeyStateAddr, _keyboard.current, sizeof(_keyboard.current));
    _kernel.process->PrepareRead(_kernel.gptCursorAsync, (uint8_t*)&_mouse.current, sizeof(_mouse.current));
    _kernel.process->Read();
}
Input::~Input() {
    delete _kernel.process;
    _kernel.process = nullptr;
}                                                                                                                   

uint32_t Input::getWindowsVersion() {
    std::wstring windowsVersion = L"";
    auto payload = _fpga->QueryWindowsRegistry("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\CurrentBuild", REG_SZ);
    if(payload) {
        windowsVersion = std::wstring(reinterpret_cast<wchar_t*>(payload));
    } else {
        // NOTE: we can't except here because
        //       old firmware/OSs won't let us query winreg ?
        // throw std::exception("Failed to query windows version");
        LOG_ERR("Failed to query windows version");
    }

    if (windowsVersion.empty()) {
        LOG_ERR("windows version registry key empty");
        return 0;
    }

    return std::stoi( windowsVersion );
}

void Input::Read() {
    // 1. update 'previous' state
    memcpy(_keyboard.previous, _keyboard.current, sizeof(_keyboard.current));
    memcpy(&_mouse.previous, &_mouse.current, sizeof(_mouse.current));

    // 2. read 'new' state from destination
    // TODO: maybe we can make it so we don't need to continuously _prepare_ the scatter
    // and can just build it and loop call scatter ?
    // maybe via raw scatters ?
    _kernel.process->PrepareRead(_kernel.gafAsyncKeyStateAddr, _keyboard.current, sizeof(_keyboard.current));
    _kernel.process->PrepareRead(_kernel.gptCursorAsync, (uint8_t*)&_mouse.current, sizeof(_mouse.current));
    _kernel.process->Read();

    // 3. run callbacks for state changes
    if(_keyboard.callback) {
        // loop through all virtual keys
        for (int vk = 0; vk < 256; vk++) {
            // if state has changed
            if(READ_KEYSTATE(_keyboard.current, vk) != READ_KEYSTATE(_keyboard.previous, vk)) {
                // run callback
                _keyboard.callback(vk, READ_KEYSTATE(_keyboard.current, vk));
            }
        } 
    }

    if(_mouse.callback) {
        POINT delta = {
            _mouse.current.x - _mouse.previous.x,
            _mouse.current.y - _mouse.previous.y,
        };

        _mouse.callback(delta, _mouse.current);
    }
}

// use VIRTUAL_KEY_CODE to check if key is pressed
bool Input::IsKeyDown(uint32_t vk, bool once) {
    bool down = READ_KEYSTATE(_keyboard.current, vk);
    if(down && once) {
        down = down && !READ_KEYSTATE(_keyboard.previous, vk);
    }

    return down;
}
// get current cursor position
POINT Input::GetCursorPos() {
    return _mouse.current;
}

// set a callback to be run on key state change
//  Callback(VIRTUAL_KEY_CODE, IS_DOWN)
void Input::OnKeyStateChange(fncOnKeyStateChange callback) {
    _keyboard.callback = callback;
}
// set a callback to be run on mouse pos change
// Callback(CHANGE_IN_MOUSE_POSITION, NEW_MOUSE_POSITION)
void Input::OnMouseMove(fncOnMouseMove callback) {
    _mouse.callback = callback;
}