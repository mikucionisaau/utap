#include "utap/Library.hpp"

#include <filesystem>
#include <stdexcept>

#ifndef NDEBUG
#include <iostream>
#endif

namespace UTAP {

#if defined(__linux__)
#include <dlfcn.h>
const std::string& Library::file_extension()
{
    static const auto dll_extension = std::string{".so"};
    return dll_extension;
}
using dll_handle = void*;
#elif defined(__APPLE__) && defined(__MACH__)
#include <dlfcn.h>
const std::string& Library::file_extension()
{
    static const auto dll_extension = std::string{".dylib"};
    return dll_extension;
}
using dll_handle = void*;
#elif defined(_WIN32) || defined(__MINGW32__)
#include <sstream>
#include <windows.h>
const std::string& Library::file_extension()
{
    static const auto dll_extension = std::string{".dll"};
    return dll_extension;
}
using dll_handle = HMODULE;
#else
#error "Unsupported target OS"
#endif

struct Library::Impl
{
    dll_handle handle{};
    explicit Impl(const std::string& name);
    ~Impl() noexcept;
    Impl(const Impl&) = delete;
    Impl(Impl&&) noexcept = delete;
    Impl& operator=(const Impl&) = delete;
    Impl& operator=(Impl&&) noexcept = delete;
};

#if defined(__linux__) || (defined(__APPLE__) && defined(__MACH__))

inline Library::Impl::Impl(const std::string& name): handle{dlopen(name.c_str(), RTLD_NOW | RTLD_LOCAL)}
{
    if (handle == nullptr) {
        auto path = std::filesystem::path{name};
        if (path.extension().string() == file_extension())
            throw std::runtime_error(dlerror());
        else {
            path = name + file_extension();
            handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
            if (handle == nullptr)
                throw std::runtime_error(dlerror());
        }
    }
}

inline Library::Impl::~Impl() noexcept
{
    if (handle != nullptr) {
        auto res [[maybe_unused]] = dlclose(handle);
        handle = nullptr;
#ifndef NDEBUG
        if (res != 0)
            std::cerr << dlerror() << std::endl;
#endif
    }
}

void* Library::get_symbol(const std::string& name)
{
    auto* res = dlsym(pImpl->handle, name.c_str());
    if (res == nullptr)
        throw std::runtime_error{dlerror()};
    return res;
}

#elif defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__)

static std::string get_error_message(const std::string& msg, DWORD err)
{
    auto ss = std::ostringstream{};
    ss << msg << ": error " << err;
    LPTSTR lpMessage = nullptr;
    DWORD sz =
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                      nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMessage, 0, nullptr);
    if (sz > 0 && lpMessage != nullptr) {
        ss << ": " << lpMessage;
        LocalFree(lpMessage);
    }
    return ss.str();
}

inline Library::Impl::Impl(const std::string& name): handle{LoadLibrary(TEXT(name.c_str()))}
{
    if (handle == nullptr) {
        auto err = GetLastError();
        auto path = std::filesystem::path{name};
        if (path.extension().string() == file_extension())
            throw std::runtime_error(get_error_message("Failed to open dynamic library " + path.string(), err));
        else {
            path = name + file_extension();
            handle = LoadLibrary(TEXT(path.string().c_str()));
            if (handle == nullptr)
                throw std::runtime_error(
                    get_error_message("Failed to open dynamic library " + path.string(), GetLastError()));
        }
    }
}

inline Library::Impl::~Impl() noexcept
{
    if (handle) {
        BOOL res [[maybe_unused]] = FreeLibrary(handle);
#ifndef NDEBUG
        if (!res)
            std::cerr << get_error_message("Failed to close dynamic library", GetLastError()) << std::endl;
#endif
    }
}

void* Library::get_symbol(const std::string& name)
{
    void* res = (void*)GetProcAddress(pImpl->handle, name.c_str());
    if (res == nullptr)
        throw std::runtime_error(get_error_message("Failed to find symbol", GetLastError()));
    return res;
}

#else
#error "Unsupported target OS"
#endif

Library::Library(const std::string& name): pImpl{new Impl{name}} {}
Library::~Library() noexcept { delete pImpl; }

}  // namespace UTAP
