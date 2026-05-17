#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 跨平台头文件
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define close closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

// 初始化网络（Windows 需要）
static void init_network() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
}

// 清理网络
static void cleanup_network() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// 获取网页内容（host: 域名, path: 路径）
void http_get(const char *host, const char *path) {
    int sock;
    struct hostent *server;
    struct sockaddr_in serv_addr;

    init_network();

    // 创建 socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("Socket 创建失败\n");
        return;
    }

    // 解析域名
    server = gethostbyname(host);
    if (server == NULL) {
        printf("域名解析失败\n");
        close(sock);
        return;
    }

    // 填充地址
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(80); // HTTP 端口

    // 连接
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("连接失败\n");
        close(sock);
        return;
    }

    // 发送 HTTP 请求
    char request[1024];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n\r\n",
             path, host);

    send(sock, request, strlen(request), 0);

    // 读取返回内容
    char buffer[4096];
    int n;
    while ((n = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = 0;
        printf("%s", buffer);
    }

    close(sock);
    cleanup_network();
}

int main() {
    // 注意：用 http 不是 https！
    http_get("ifconfig.io", "/country_code");
    return 0;
}