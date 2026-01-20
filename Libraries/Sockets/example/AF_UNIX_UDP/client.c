#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SERVER_PATH "/tmp/rk3568_dgram_safe.sock"
#define BUFFER_SIZE 1024

// 全局变量用于清理
int client_sockfd = -1;
char client_path[108]; // 保存客户端自己的 socket 路径

// 【核心】客户端的信号处理
void handle_exit(int sig) {
    printf("\n[Client] Interrupted! Cleaning up...\n");
    if (client_sockfd != -1) close(client_sockfd);
    
    // 删除客户端自己的 Socket 文件
    unlink(client_path);
    exit(0);
}

int main(int argc, char *argv[]) {
    struct sockaddr_un server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    char *msg = "Hello Safe World";

    if (argc > 1) msg = argv[1];

    // 注册信号
    signal(SIGINT, handle_exit);
    signal(SIGTERM, handle_exit);

    // 1. 创建 Socket
    if ((client_sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // 2. 【关键】生成唯一的客户端路径 (使用 PID)
    // 避免多个客户端运行时冲突
    memset(client_path, 0, sizeof(client_path));
    snprintf(client_path, sizeof(client_path), "/tmp/client_%d.sock", getpid());

    // 3. 绑定客户端地址
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sun_family = AF_UNIX;
    strncpy(client_addr.sun_path, client_path, sizeof(client_addr.sun_path) - 1);

    unlink(client_path); // 防御性清理
    if (bind(client_sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr)) == -1) {
        perror("client bind");
        exit(1);
    }
    
    // 权限设宽松点，防止 Server 回复时没权限写
    chmod(client_path, 0666); 

    printf("[Client] Bound to %s\n", client_path);

    // 4. 准备服务端地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SERVER_PATH, sizeof(server_addr.sun_path) - 1);

    // 5. 发送
    ssize_t sent = sendto(client_sockfd, msg, strlen(msg), 0,
                          (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (sent == -1) {
        perror("sendto");
        handle_exit(1); // 出错也走清理流程
    }

    printf("[Client] Sent message, waiting for reply...\n");

    // 6. 接收 (设置一个 3秒 超时，避免 server 没开导致客户端死等)
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(client_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    socklen_t server_len = sizeof(server_addr);
    ssize_t recved = recvfrom(client_sockfd, buffer, BUFFER_SIZE - 1, 0,
                              (struct sockaddr *)&server_addr, &server_len);

    if (recved > 0) {
        printf("[Client] Server Replied: %s\n", buffer);
    } else {
        perror("recvfrom (timeout or error)");
    }

    // 7. 正常退出清理
    // 这里不需要调用 handle_exit，直接手动清理更清晰
    close(client_sockfd);
    unlink(client_path); // 删除文件
    printf("[Client] Exiting normally.\n");

    return 0;
}
