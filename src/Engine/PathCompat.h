#ifndef SRC_PATH_COMPAT_H_
#define SRC_PATH_COMPAT_H_

#include <string>
#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>

namespace McBopomofo {
namespace fcitx5_compat {

// Minimal adaptation for Windows
inline std::string locate(const std::string& path) {
    // For now, assume data is in the same directory as the executable or a 'data' subfolder
    if (std::filesystem::exists(path)) return path;
    if (std::filesystem::exists("data/" + path)) return "data/" + path;
    return path;
}

inline std::string userDirectory() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        std::filesystem::path p(path);
        p /= "WinMcBopomofo";
        std::filesystem::create_directories(p);
        return p.string();
    }
    return "";
}

}  // namespace fcitx5_compat
}  // namespace McBopomofo

#endif  // SRC_PATH_COMPAT_H_
