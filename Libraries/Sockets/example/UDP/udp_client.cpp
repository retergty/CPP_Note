#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() 
{
    // 1. 创建 Socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // 2. 准备目标地址
    sockaddr_in servaddr{};
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8081);
    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);

    // 3. 直接发送 (sendto) - 不需要 connect
    const char* msg = "Hello UDP World!";
    std::cout << "[UDP Client] Sending message..." << std::endl;
    
    sendto(sockfd, msg, strlen(msg), 0, 
           (const struct sockaddr *)&servaddr, sizeof(servaddr));

    // 4. 等待回信
    char buffer[1024];
    socklen_t len = sizeof(servaddr);
    
    int n = recvfrom(sockfd, buffer, 1024, 0, 
                     (struct sockaddr *)&servaddr, &len);
    
    buffer[n] = '\0';
    std::cout << "Server reply: " << buffer << std::endl;

    close(sockfd);
    return 0;
}