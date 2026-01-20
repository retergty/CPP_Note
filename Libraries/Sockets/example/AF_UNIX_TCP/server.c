#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h> // chmod 需要这个头文件

#define SOCKET_PATH "/tmp/rk3568_permission.sock"
#define BUFFER_SIZE 128

int server_fd;

// 信号处理函数：捕获 Ctrl+C，清理文件
void handle_sigint(int sig) {
    printf("\n[Server] Catch SIGINT, cleaning up...\n");
    if (server_fd > 0) close(server_fd);
    unlink(SOCKET_PATH); // 关键：删除文件
    exit(0);
}

int main() {
    int client_fd;
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];

    // 注册信号处理
    signal(SIGINT, handle_sigint);

    // 1. 创建 Socket
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // 2. 准备地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    // 3. 绑定前清理旧文件
    unlink(SOCKET_PATH);

    // 4. 绑定 (此时会在文件系统创建文件)
    // 默认情况下，如果我是 root 运行，文件权限可能是 srwxr-x--- (750)
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (chmod(SOCKET_PATH, 0666) == -1) {
        perror("chmod");
        // 这里不退出，只是报个警，因为如果也是 root 连接则不需要这个
    } else {
        printf("[Server] Socket permissions set to 0666 (Global Access).\n");
    }

    // 5. 监听
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        exit(1);
    }

    printf("[Server] Listening on %s ... (Press Ctrl+C to stop)\n", SOCKET_PATH);

    // 6. 循环处理
    while (1) {
        // Accept 会阻塞
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        // 读取数据
        memset(buffer, 0, BUFFER_SIZE);
        int len = read(client_fd, buffer, BUFFER_SIZE - 1);
        if (len > 0) {
            printf("[Server] Recv: %s\n", buffer);
            // 回复
            write(client_fd, "OK", 2);
        }

        close(client_fd);
    }

    return 0;
}
