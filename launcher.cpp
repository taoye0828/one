#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <cstdlib>

// 平台相关头文件
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#endif

// 函数声明
void startProgramA(const std::string& path, 
#ifdef _WIN32
    HANDLE& hProcessA, DWORD& pidA
#else
    pid_t& pidA
#endif
);

void startProgramB(const std::string& path, 
#ifdef _WIN32
    HANDLE& hProcessB, DWORD& pidB
#else
    pid_t& pidB
#endif
);

void waitForProgramB(
#ifdef _WIN32
    HANDLE hProcessB
#else
    pid_t pidB
#endif
);

void closeProgramA(
#ifdef _WIN32
    HANDLE hProcessA, DWORD pidA
#else
    pid_t pidA
#endif
);

int main() {
    // 设置程序A和程序B的路径
    std::string programAPath = "G:\\v2\\v2N\\v2rayN.exe";  // Windows
    // std::string programAPath = "./program_a";        // Linux
    
    std::string programBPath = "\"C:\\Program Files\\Google\\Chrome\\Application\\chrome_proxy.exe\" --profile-directory=\"Default\" --ignore-profile-directory-if-not-exists https://chatgpt.com/";  // Windows
    // std::string programBPath = "./program_b";        // Linux

#ifdef _WIN32
    HANDLE hProcessA = NULL, hProcessB = NULL;
    DWORD pidA = 0, pidB = 0;
#else
    pid_t pidA = 0, pidB = 0;
#endif

    try {
        // 启动程序A
        std::cout << "启动程序A..." << std::endl;
        startProgramA(programAPath, 
#ifdef _WIN32
            hProcessA, pidA
#else
            pidA
#endif
        );
        std::cout << "程序A启动成功，进程ID: " << pidA << std::endl;

        // 等待5秒
        std::cout << "等待5秒..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // 启动程序B
        std::cout << "启动程序B..." << std::endl;
        startProgramB(programBPath, 
#ifdef _WIN32
            hProcessB, pidB
#else
            pidB
#endif
        );
        std::cout << "程序B启动成功，进程ID: " << pidB << std::endl;

        // 等待程序B结束
        std::cout << "等待程序B关闭..." << std::endl;
        waitForProgramB(
#ifdef _WIN32
            hProcessB
#else
            pidB
#endif
        );
        std::cout << "程序B已关闭" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "发生错误: " << e.what() << std::endl;
    }
    {
        // 关闭程序A
#ifdef _WIN32
        if (hProcessA != NULL) {
            DWORD exitCode;
            if (GetExitCodeProcess(hProcessA, &exitCode) && exitCode == STILL_ACTIVE) {
#else
        if (pidA > 0) {
            int status;
            if (waitpid(pidA, &status, WNOHANG) == 0) {
#endif
                std::cout << "关闭程序A..." << std::endl;
                closeProgramA(
#ifdef _WIN32
                    hProcessA, pidA
#else
                    pidA
#endif
                );
                std::cout << "程序A已关闭" << std::endl;
            }
#ifdef _WIN32
            CloseHandle(hProcessA);
#endif
        }

#ifdef _WIN32
        if (hProcessB != NULL) {
            CloseHandle(hProcessB);
        }
#endif
    }

    return 0;
}

// 启动程序A的实现
void startProgramA(const std::string& path, 
#ifdef _WIN32
    HANDLE& hProcessA, DWORD& pidA
#else
    pid_t& pidA
#endif
) {
#ifdef _WIN32
    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};

    if (!CreateProcessA(NULL, (LPSTR)path.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        throw std::runtime_error("无法启动程序A，错误代码: " + std::to_string(GetLastError()));
    }

    hProcessA = pi.hProcess;
    pidA = pi.dwProcessId;
    CloseHandle(pi.hThread);
#else
    pidA = fork();
    if (pidA == -1) {
        throw std::runtime_error("fork失败，无法启动程序A");
    }
    else if (pidA == 0) {
        // 子进程 - 执行程序A
        execl(path.c_str(), path.c_str(), (char*)NULL);
        exit(EXIT_FAILURE); // 如果execl返回，则表示出错
    }
#endif
}

// 启动程序B的实现
void startProgramB(const std::string& path, 
#ifdef _WIN32
    HANDLE& hProcessB, DWORD& pidB
#else
    pid_t& pidB
#endif
) {
#ifdef _WIN32
    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};

    if (!CreateProcessA(NULL, (LPSTR)path.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        throw std::runtime_error("无法启动程序B，错误代码: " + std::to_string(GetLastError()));
    }

    hProcessB = pi.hProcess;
    pidB = pi.dwProcessId;
    CloseHandle(pi.hThread);
#else
    pidB = fork();
    if (pidB == -1) {
        throw std::runtime_error("fork失败，无法启动程序B");
    }
    else if (pidB == 0) {
        // 子进程 - 执行程序B
        execl(path.c_str(), path.c_str(), (char*)NULL);
        exit(EXIT_FAILURE); // 如果execl返回，则表示出错
    }
#endif
}

// 等待程序B结束
void waitForProgramB(
#ifdef _WIN32
    HANDLE hProcessB
#else
    pid_t pidB
#endif
) {
#ifdef _WIN32
    WaitForSingleObject(hProcessB, INFINITE);
#else
    int status;
    waitpid(pidB, &status, 0);
#endif
}

// 关闭程序A
void closeProgramA(
#ifdef _WIN32
    HANDLE hProcessA, DWORD pidA
#else
    pid_t pidA
#endif
) {
#ifdef _WIN32
    // 先尝试优雅关闭
    if (!TerminateProcess(hProcessA, 0)) {
        throw std::runtime_error("无法关闭程序A，错误代码: " + std::to_string(GetLastError()));
    }
#else
    // 先尝试SIGTERM
    kill(pidA, SIGTERM);
    
    // 等待5秒
    for (int i = 0; i < 50; ++i) {
        int status;
        if (waitpid(pidA, &status, WNOHANG) != 0) {
            return; // 程序已关闭
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 如果仍未关闭，使用SIGKILL强制关闭
    kill(pidA, SIGKILL);
#endif
}
    