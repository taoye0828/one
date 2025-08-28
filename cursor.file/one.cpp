#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#include <tlhelp32.h>
#include <string>

#pragma comment(lib, "Shlwapi.lib")

// Configuration: path to v2rayN.exe
static const wchar_t* kV2rayNPath = L"G:\\v2\\v2N\\v2rayN.exe";

// Utility: get full path to current executable
static std::wstring GetSelfPath() {
    wchar_t buffer[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return std::wstring(buffer, len);
}

// Ensure app auto-starts on user logon via HKCU Run key
static void EnsureAutoStart() {
    HKEY hKey;
    const wchar_t* runKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    if (RegCreateKeyExW(HKEY_CURRENT_USER, runKey, 0, nullptr, 0, KEY_READ | KEY_SET_VALUE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        std::wstring selfPath = GetSelfPath();
        // Quote path to handle spaces
        std::wstring value = L"\"" + selfPath + L"\"";
        RegSetValueExW(hKey, L"StartV2rayOnChatGPT", 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
    }
}

// Hide console window if any (for console builds)
static void HideConsoleWindowIfPresent() {
    HWND hwnd = GetConsoleWindow();
    if (hwnd != nullptr) {
        ShowWindow(hwnd, SW_HIDE);
    }
}

// Check if a process with given exe name is running
static bool IsProcessRunning(const wchar_t* processName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    bool found = false;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, processName) == 0) {
                found = true;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return found;
}

// Launch v2rayN if not already running
static void LaunchV2rayNIfNeeded() {
    if (!PathFileExistsW(kV2rayNPath)) return; // path invalid; do nothing
    if (IsProcessRunning(L"v2rayN.exe")) return;

    SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = nullptr;
    sei.lpVerb = L"open";
    sei.lpFile = kV2rayNPath;
    sei.lpParameters = nullptr;
    sei.lpDirectory = nullptr;
    sei.nShow = SW_SHOWNORMAL;
    if (ShellExecuteExW(&sei)) {
        if (sei.hProcess) CloseHandle(sei.hProcess);
    }
}

// EnumWindows callback: detect any top-level window whose title contains "ChatGPT"
struct ChatGPTDetectState {
    bool found;
};

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    ChatGPTDetectState* state = reinterpret_cast<ChatGPTDetectState*>(lParam);
    if (!IsWindowVisible(hwnd)) return TRUE;

    wchar_t title[512];
    int len = GetWindowTextW(hwnd, title, 512);
    if (len <= 0) return TRUE;

    if (StrStrIW(title, L"ChatGPT") != nullptr) {
        state->found = true;
        return FALSE; // stop enumeration
    }
    return TRUE;
}

static bool IsChatGPTWindowPresent() {
    ChatGPTDetectState state{ false };
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&state));
    return state.found;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    // Optional: for console builds, hide the console
    HideConsoleWindowIfPresent();

    // Ensure the app starts on login
    EnsureAutoStart();

    // Simple loop: check every 2 seconds
    const DWORD kSleepMs = 2000;

    // Avoid repeated rapid launches: cool down flag
    bool coolDown = false;
    DWORD lastLaunchTick = 0;
    const DWORD kCooldownMs = 15000; // 15s between attempts

    while (true) {
        bool hasChatGPT = IsChatGPTWindowPresent();

        DWORD now = GetTickCount();
        if (coolDown && (now - lastLaunchTick > kCooldownMs)) {
            coolDown = false;
        }

        if (hasChatGPT && !coolDown) {
            LaunchV2rayNIfNeeded();
            lastLaunchTick = now;
            coolDown = true;
        }

        Sleep(kSleepMs);
    }

    return 0;
}

#else
// Non-Windows placeholder to avoid build errors on other platforms
int main() {
    return 0;
}
#endif


