#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    // 1. 创建 Socket (就像买手机)
    // SOCK_STREAM 代表 TCP
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // 2. 绑定地址和端口 (插电话线)
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; // 监听本机所有 IP
    addr.sin_port = htons(8080);       // 端口 8080 (注意字节序转换)

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));

    // 3. 监听 (开机等待)
    listen(server_fd, 5);
    std::cout << "[TCP Server] Listening on 8080..." << std::endl;

    // 4. 接受连接 (接电话)
    // accept 是阻塞的，直到有客户端连上来
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

    std::cout << "[TCP Server] Client connected!" << std::endl;

    // 5. 通信循环
    char buffer[1024] = {0};
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        // 从 client_fd 读取数据
        int valread = read(client_fd, buffer, 1024);
        
        // 如果 read 返回 0，代表客户端断开了连接 (FIN)
        if (valread <= 0) {
            std::cout << "[TCP Server] Client disconnected." << std::endl;
            break;
        }

        std::cout << "Received: " << buffer << std::endl;

        // 原样发回去
        std::string reply = "Server echoes: " + std::string(buffer);
        send(client_fd, reply.c_str(), reply.length(), 0);
    }

    // 6. 挂断
    close(client_fd);
    close(server_fd);
    return 0;
}