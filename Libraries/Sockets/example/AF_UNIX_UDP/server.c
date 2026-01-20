#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#define SERVER_PATH "/tmp/rk3568_dgram_safe.sock"
#define BUFFER_SIZE 1024

// 全局变量，方便信号处理函数访问
int server_sockfd = -1;

// 【核心】信号处理函数
void handle_exit(int sig) {
    printf("\n[Server] Catch signal %d, cleaning up...\n", sig);
    
    // 1. 关闭句柄
    if (server_sockfd != -1) {
        close(server_sockfd);
    }

    // 2. 【关键】删除 Socket 文件
    // 如果不删，下次启动 bind 会报错 "Address already in use"
    unlink(SERVER_PATH);
    
    printf("[Server] Cleanup done. Bye!\n");
    exit(0);
}

int main() {
    struct sockaddr_un server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t client_addr_len;

    // 注册信号处理：按 Ctrl+C 时触发 handle_exit
    signal(SIGINT, handle_exit);
    signal(SIGTERM, handle_exit);

    // 1. 创建 Socket
    if ((server_sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // 2. 准备地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SERVER_PATH, sizeof(server_addr.sun_path) - 1);

    // 3. 启动前先尝试清理残留 (防御性编程)
    unlink(SERVER_PATH);

    // 4. 绑定
    if (bind(server_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(1);
    }

    // 5. 放开权限 (允许普通用户客户端发送数据)
    if (chmod(SERVER_PATH, 0666) == -1) {
        perror("chmod");
    }

    printf("[Server] Listening on %s ... (Press Ctrl+C to stop)\n", SERVER_PATH);

    while (1) {
        client_addr_len = sizeof(client_addr);
        memset(buffer, 0, BUFFER_SIZE);

        // 接收数据 (阻塞)
        ssize_t n = recvfrom(server_sockfd, buffer, BUFFER_SIZE - 1, 0,
                             (struct sockaddr *)&client_addr, &client_addr_len);

        if (n == -1) {
            perror("recvfrom");
            continue;
        }

        printf("[Server] Recv from [%s]: %s\n", client_addr.sun_path, buffer);

        // 回复数据
        if (strlen(client_addr.sun_path) > 0) {
            char *reply = "ACK from Server";
            sendto(server_sockfd, reply, strlen(reply), 0,
                   (struct sockaddr *)&client_addr, client_addr_len);
        }
    }

    return 0;
}
