#ifndef UTAP_LIBRARY_HPP
#define UTAP_LIBRARY_HPP

#include <string>
#include <utility>  // move, swap

namespace UTAP {

/// OS detection for `if constexpr` code
enum class OS { Linux, Windows, macOS };
#if defined(__linux__)
constexpr auto CURRENT_OS = OS::Linux;
#elif defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__)
constexpr auto CURRENT_OS = OS::Windows;
#elif defined(__APPLE__) && defined(__MACH__)
constexpr auto CURRENT_OS = OS::macOS;
#else
#error "Unsupported target operating system"
#endif  // OS
/// Returns true iff os is the current (target) OS.
constexpr bool is(OS os) { return CURRENT_OS == os; }

/// Wraps dynamically loaded library (DLL) calls to with a unified interface across OSes.
class Library
{
    struct Impl;
    Impl* pImpl{nullptr};

public:
    /** Loads a dynamically linked library with specified name (path).
     * If loading fails, it will attempt to load with an OS-specific extension if no extension is given.
     * Throws std::runtime_error upon errors.
     */
    Library(const std::string& name);

    /// Unloads the library. Outputs errors into stderr if any, when compiled with debug info.
    ~Library() noexcept;

    Library(const Library&) = delete;

    Library(Library&& other) noexcept { *this = std::move(other); }

    Library& operator=(const Library&) = delete;

    Library& operator=(Library&& other) noexcept
    {
        std::swap(pImpl, other.pImpl);
        return *this;
    }

    /// Finds the symbol in the library. Throws std::runtime_error upon errors.
    void* get_symbol(const std::string& name);

    /// OS-specific file extension used by dynamic libraries, e.g. ".so"
    static const std::string& file_extension();
};

}  // namespace UTAP
#endif  // UTAP_LIBRARY_HPP
