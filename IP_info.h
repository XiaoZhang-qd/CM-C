#ifndef LL_H
#define LL_H

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
static inline void init_network() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
}

// 清理网络
static inline void cleanup_network() {
#ifdef _WIN32
    WSACleanup();
#endif
}

/**
 * 通用 HTTP 请求函数（内部静态函数）
 */
static inline int http_get_data(const char *host, const char *path, int family, char *out_buf, size_t max_len) {
    int sock = -1;
    struct addrinfo hints, *res = NULL, *ptr = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;       // 指定 IPv4 或 IPv6
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, "80", &hints, &res) != 0) {
        return 0;
    }

    for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
        sock = (int)socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sock < 0) continue;

        if (connect(sock, ptr->ai_addr, (int)ptr->ai_addrlen) >= 0) {
            break; 
        }
        close(sock);
        sock = -1;
    }
    freeaddrinfo(res);

    if (sock < 0) return 0;

    char request[512];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n\r\n",
             path, host);

    send(sock, request, strlen(request), 0);

    char buffer[2048];
    char header_parse_buf[4096] = {0};
    size_t parse_len = 0;
    int header_finished = 0;
    int n;

    out_buf[0] = '\0';

    while ((n = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = 0;

        if (!header_finished) {
            if (parse_len + n < sizeof(header_parse_buf) - 1) {
                memcpy(header_parse_buf + parse_len, buffer, n);
                parse_len += n;
                header_parse_buf[parse_len] = 0;
            }

            char *body_start = strstr(header_parse_buf, "\r\n\r\n");
            if (body_start) {
                header_finished = 1;
                body_start += 4;
                strncpy(out_buf, body_start, max_len - 1);
                out_buf[max_len - 1] = '\0';
            }
        } else {
            size_t cur_len = strlen(out_buf);
            if (cur_len + n < max_len - 1) {
                strcat(out_buf, buffer);
            }
        }
    }

    close(sock);

    size_t out_len = strlen(out_buf);
    while (out_len > 0 && (out_buf[out_len - 1] == '\n' || out_buf[out_len - 1] == '\r')) {
        out_buf[out_len - 1] = '\0';
        out_len--;
    }

    return (out_len > 0);
}

/**
 * 供外部 main.c 调用的核心接口
 * 全部动态获取：国家全称、国家代码、IPv4、IPv6 并在内部完成拼接
 */
static inline void get_online_info(char *out_result, size_t max_len) {
    char country_name[64] = "N/A";
    char country_code[32] = "N/A";
    char ipv4_addr[64] = "N/A";
    char ipv6_addr[64] = "N/A";

    // 删掉这行，不要在子函数里重复初始化
    // init_network();

    // 1. 动态获取国家全称
    http_get_data("ifconfig.co", "/country", AF_INET, country_name, sizeof(country_name));

    // 2. 动态获取国家/地区代码
    http_get_data("ifconfig.co", "/country-iso", AF_INET, country_code, sizeof(country_code));

    // 3. 动态获取 IPv4 地址
    http_get_data("ifconfig.co", "/ip", AF_INET, ipv4_addr, sizeof(ipv4_addr));

    // 4. 动态获取 IPv6 地址
    http_get_data("ifconfig.co", "/ip", AF_INET6, ipv6_addr, sizeof(ipv6_addr));

    // 5. 动态拼接
    snprintf(out_result, max_len, "%s(%s)-%s-·-%s-", country_name, country_code, ipv4_addr, ipv6_addr);

    // 务必删掉这行！否则 Windows 下主连接 Socket 会直接断开
    // cleanup_network();
}

#endif // LL_H
