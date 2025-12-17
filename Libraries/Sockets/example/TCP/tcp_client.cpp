#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    // 1. 创建 Socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    // 2. 准备连接目标的地址
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    // 将字符串 IP 转换为二进制
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    // 3. 发起连接 (拨号 - 三次握手)
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed!" << std::endl;
        return -1;
    }
    std::cout << "[TCP Client] Connected to server." << std::endl;

    // 4. 发送数据
    const char* msg = "Hello TCP World!";
    send(sock, msg, strlen(msg), 0);
    std::cout << "Sent: " << msg << std::endl;

    // 5. 接收回音
    char buffer[1024] = {0};
    read(sock, buffer, 1024);
    std::cout << "Server replied: " << buffer << std::endl;

    // 6. 关闭
    close(sock);
    return 0;
}