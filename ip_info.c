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

static void init_network() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
}

static void cleanup_network() {
#ifdef _WIN32
    WSACleanup();
#endif
}

/**
 * 通用 HTTP 请求函数
 * @param host     域名
 * @param path     路径
 * @param family   AF_INET (IPv4) 或 AF_INET6 (IPv6)
 * @param out_buf  用于存储返回的正文内容的缓冲区
 * @param max_len  缓冲区最大长度
 */
int http_get_data(const char *host, const char *path, int family, char *out_buf, size_t max_len) {
    int sock = -1;
    struct addrinfo hints, *res = NULL, *ptr = NULL;
    int success = 0;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;       // 指定 IPv4 或 IPv6
    hints.ai_socktype = SOCK_STREAM;

    // 使用 getaddrinfo 实现双栈解析
    if (getaddrinfo(host, "80", &hints, &res) != 0) {
        return 0;
    }

    // 遍历地址尝试连接
    for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
        sock = (int)socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sock < 0) continue;

        if (connect(sock, ptr->ai_addr, (int)ptr->ai_addrlen) >= 0) {
            break; // 连接成功
        }
        close(sock);
        sock = -1;
    }
    freeaddrinfo(res);

    if (sock < 0) return 0;

    // 发送 HTTP 请求
    char request[512];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n\r\n",
             path, host);

    send(sock, request, strlen(request), 0);

    // 接收并解析数据
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

    // 去除返回结果末尾可能带有的换行符
    size_t out_len = strlen(out_buf);
    while (out_len > 0 && (out_buf[out_len - 1] == '\n' || out_buf[out_len - 1] == '\r')) {
        out_buf[out_len - 1] = '\0';
        out_len--;
    }

    return (out_len > 0);
}

int main() {
    char ipv4_addr[64] = "N/A";
    char ipv6_addr[64] = "N/A";

    init_network();

    // 1. 强制使用 IPv4 链路获取 IPv4 地址
    http_get_data("ifconfig.io", "/ip", AF_INET, ipv4_addr, sizeof(ipv4_addr));

    // 2. 强制使用 IPv6 链路获取 IPv6 地址
    // 注意：如果你的本地网络或编译环境不支持 IPv6，这里会返回失败，保持默认的 "N/A"
    http_get_data("ifconfig.io", "/ip", AF_INET6, ipv6_addr, sizeof(ipv6_addr));

    // 3. 严格按照要求的格式拼接输出
    printf("China(CN)-%s-·-%s-\n", ipv4_addr, ipv6_addr);

    cleanup_network();
    return 0;
}