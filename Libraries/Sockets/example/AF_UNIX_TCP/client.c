#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/rk3568_permission.sock"

int main(int argc, char *argv[]) {
    int client_fd;
    struct sockaddr_un server_addr;
    char *msg = "Hello from standard user!";

    if (argc > 1) msg = argv[1];

    // 1. 创建 Socket
    if ((client_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // 2. 准备地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    // 3. 连接
    // 如果 Server 没做 chmod 0666，且 Client 是普通用户，这里会报 Permission denied
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect failed (Check permissions!)");
        exit(1);
    }

    // 4. 发送与接收
    write(client_fd, msg, strlen(msg));
    char buf[32] = {0};
    read(client_fd, buf, sizeof(buf));
    printf("[Client] Sent: '%s', Server replied: '%s'\n", msg, buf);

    close(client_fd);
    return 0;
}
