#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    // 1. 创建 Socket
    // SOCK_DGRAM 代表 UDP
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // 2. 绑定端口 (挂邮筒)
    sockaddr_in servaddr{};
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(8081); // 用 8081 端口

    bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    
    std::cout << "[UDP Server] Waiting for data on 8081..." << std::endl;

    char buffer[1024];
    sockaddr_in cliaddr{}; // 用来存发送者的地址
    socklen_t len = sizeof(cliaddr);

    // 3. 循环收信
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        
        // recvfrom 会自动填充 cliaddr，告诉你谁发的
        int n = recvfrom(sockfd, buffer, 1024, 0, 
                         (struct sockaddr *)&cliaddr, &len);
        
        buffer[n] = '\0';
        std::cout << "Received: " << buffer 
                  << " from " << inet_ntoa(cliaddr.sin_addr) 
                  << ":" << ntohs(cliaddr.sin_port) << std::endl;

        // 4. 回信 (sendto)
        // 必须把刚才拿到的 cliaddr 放进去
        const char* response = "Message Received!";
        sendto(sockfd, response, strlen(response), 0, 
               (const struct sockaddr *)&cliaddr, len);
    }

    close(sockfd);
    return 0;
}