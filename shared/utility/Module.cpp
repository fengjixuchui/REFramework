#include <fstream>

#include <shlwapi.h>

#include "String.hpp"
#include "Module.hpp"

using namespace std;

namespace utility {
    optional<size_t> get_module_size(const string& module) {
        return get_module_size(GetModuleHandle(module.c_str()));
    }

    optional<size_t> get_module_size(HMODULE module) {
        if (module == nullptr) {
            return {};
        }

        // Get the dos header and verify that it seems valid.
        auto dosHeader = (PIMAGE_DOS_HEADER)module;

        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            return {};
        }

        // Get the nt headers and verify that they seem valid.
        auto ntHeaders = (PIMAGE_NT_HEADERS)((uintptr_t)dosHeader + dosHeader->e_lfanew);

        if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
            return {};
        }

        // OptionalHeader is not actually optional.
        return ntHeaders->OptionalHeader.SizeOfImage;
    }

    std::optional<HMODULE> get_module_within(Address address) {
        HMODULE module = nullptr;
        if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, address.as<LPCSTR>(), &module)) {
            return module;
        }

        return {};
    }

    std::optional<uintptr_t> get_dll_imagebase(Address dll) {
        if (dll == nullptr) {
            return {};
        }

        // Get the dos header and verify that it seems valid.
        auto dosHeader = dll.as<PIMAGE_DOS_HEADER>();

        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            return {};
        }

        // Get the nt headers and verify that they seem valid.
        auto ntHeaders = (PIMAGE_NT_HEADERS)((uintptr_t)dosHeader + dosHeader->e_lfanew);

        if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
            return {};
        }

        return ntHeaders->OptionalHeader.ImageBase;
    }

    std::optional<uintptr_t> get_imagebase_va_from_ptr(Address dll, Address base, void* ptr) {
        auto file_imagebase = get_dll_imagebase(dll);

        if (!file_imagebase) {
            return {};
        }

        return *file_imagebase + ((uintptr_t)ptr - base.as<uintptr_t>());
    }


    std::optional<std::string> get_module_path(HMODULE module) {
        wchar_t filename[MAX_PATH]{0};
        if (GetModuleFileNameW(module, filename, MAX_PATH) >= MAX_PATH) {
            return {};
        }

        return utility::narrow(filename);
    }

    std::optional<std::string> get_module_directory(HMODULE module) {
        wchar_t filename[MAX_PATH]{ 0 };
        if (GetModuleFileNameW(module, filename, MAX_PATH) >= MAX_PATH) {
            return {};
        }

        PathRemoveFileSpecW(filename);

        return utility::narrow(filename);
    }

    std::vector<uint8_t> read_module_from_disk(HMODULE module) {
        auto path = get_module_path(module);

        if (!path) {
            return {};
        }
        
        // read using std utilities like ifstream and tellg, etc.
        auto file = std::ifstream{path->c_str(), std::ios::binary | std::ios::ate};

        if (!file.is_open()) {
            return {};
        }

        auto size = file.tellg();
        file.seekg(0, std::ios::beg);

        // don't brace initialize std::vector because it won't
        // call the right constructor.
        auto data = std::vector<uint8_t>((size_t)size);
        file.read((char*)data.data(), size);

        return data;
    }

    std::optional<std::vector<uint8_t>> get_original_bytes(Address address) {
        auto module_within = get_module_within(address);

        if (!module_within) {
            return {};
        }

        return get_original_bytes(*module_within, address);
    }

    std::optional<std::vector<uint8_t>> get_original_bytes(HMODULE module, Address address) {
        auto disk_data = read_module_from_disk(module);

        if (disk_data.empty()) {
            return std::nullopt;
        }

        auto module_base = get_dll_imagebase(module);

        if (!module_base) {
            return std::nullopt;
        }

        auto module_rva = address.as<uintptr_t>() - *module_base;

        // obtain the file offset of the address now
        auto disk_ptr = ptr_from_rva(disk_data.data(), module_rva);

        if (!disk_ptr) {
            return std::nullopt;
        }

        auto original_bytes = std::vector<uint8_t>{};

        auto module_bytes = address.as<uint8_t*>();
        auto disk_bytes = (uint8_t*)*disk_ptr;

        // copy the bytes from the disk data to the original bytes
        // copy only until the bytes start to match eachother
        for (auto i = 0; ; ++i) {
            if (module_bytes[i] == disk_bytes[i]) {
                break;
            }

            original_bytes.push_back(disk_bytes[i]);
        }

        if (original_bytes.empty()) {
            return std::nullopt;
        }

        return original_bytes;
    }

    HMODULE get_executable() {
        return GetModuleHandle(nullptr);
    }

    optional<uintptr_t> ptr_from_rva(uint8_t* dll, uintptr_t rva) {
        // Get the first section.
        auto dosHeader = (PIMAGE_DOS_HEADER)&dll[0];
        auto ntHeaders = (PIMAGE_NT_HEADERS)&dll[dosHeader->e_lfanew];
        auto section = IMAGE_FIRST_SECTION(ntHeaders);

        // Go through each section searching for where the rva lands.
        for (uint16_t i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i, ++section) {
            auto size = section->Misc.VirtualSize;

            if (size == 0) {
                size = section->SizeOfRawData;
            }

            if (rva >= section->VirtualAddress && rva < ((uintptr_t)section->VirtualAddress + size)) {
                auto delta = section->VirtualAddress - section->PointerToRawData;

                return (uintptr_t)(dll + (rva - delta));
            }
        }

        return {};
    }
}
