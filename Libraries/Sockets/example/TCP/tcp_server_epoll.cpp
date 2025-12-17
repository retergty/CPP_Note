#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>      // 用于设置非阻塞
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>

#define PORT 8080
#define MAX_EVENTS 1024
#define READ_BUF_SIZE 1024

// 辅助函数：将 Socket 设置为非阻塞模式 (Non-blocking)
// 这是一个必须的操作，配合 EPOLLET 使用
void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL error");
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL error");
    }
}

int main() {
    // 1. 创建监听 Socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("Socket creation failed");
        return -1;
    }

    // 端口复用 (避免重启时 bind 报错 "Address already in use")
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 2. 绑定
    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        return -1;
    }

    // 3. 监听
    if (listen(listen_fd, SOMAXCONN) == -1) {
        perror("Listen failed");
        return -1;
    }

    // 4. 创建 epoll 实例
    int epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1 failed");
        return -1;
    }

    // 5. 将监听 Socket 加入 epoll
    struct epoll_event ev{};
    ev.events = EPOLLIN; // 监听读事件 (对于 listen_fd，读事件意味着有新连接)
    ev.data.fd = listen_fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
        perror("epoll_ctl: listen_fd");
        return -1;
    }

    // 准备事件数组
    std::vector<struct epoll_event> events(MAX_EVENTS);

    std::cout << "Epoll Server running on port " << PORT << "..." << std::endl;

    while (true) {
        // 6. 等待事件发生 (阻塞)
        int nfds = epoll_wait(epfd, events.data(), MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            break;
        }

        // 7. 处理所有活跃的 socket
        for (int i = 0; i < nfds; ++i) {
            int current_fd = events[i].data.fd;

            // --- 情况 A: 监听 socket 有动静，说明有新连接 ---
            if (current_fd == listen_fd) {
                sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
                
                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }

                // [关键步骤] 设为非阻塞
                setNonBlocking(client_fd);

                // [关键步骤] 注册到 epoll，使用 ET 模式 (EPOLLET)
                struct epoll_event client_ev{};
                client_ev.events = EPOLLIN | EPOLLET; 
                client_ev.data.fd = client_fd;
                
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_ev) == -1) {
                    perror("epoll_ctl: add client");
                    close(client_fd);
                } else {
                    std::cout << "New client connected: FD=" << client_fd 
                              << " IP=" << inet_ntoa(client_addr.sin_addr) << std::endl;
                }

            } 
            // --- 情况 B: 客户端 socket 有动静，说明发数据来了 ---
            else {
                // 因为是 ET 模式，数据可能很多，必须循环读取直到读空 (EAGAIN)
                char buffer[READ_BUF_SIZE];
                bool close_conn = false;

                while (true) {
                    memset(buffer, 0, READ_BUF_SIZE);
                    ssize_t n = read(current_fd, buffer, READ_BUF_SIZE - 1);

                    if (n > 0) {
                        // 正常读到数据
                        std::cout << "[Client " << current_fd << "]: " << buffer << std::endl;
                        // 回声 (Echo)
                        send(current_fd, buffer, n, 0); 
                    } 
                    else if (n == 0) {
                        // 客户端断开了连接
                        close_conn = true;
                        break;
                    } 
                    else { // n < 0
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // [关键] 数据读完了！缓冲区空了，跳出循环等待下一次通知
                            break;
                        } else {
                            // 真的出错了
                            perror("read error");
                            close_conn = true;
                            break;
                        }
                    }
                }

                if (close_conn) {
                    // 关闭连接，epoll 会自动将其移除
                    close(current_fd);
                    std::cout << "Client " << current_fd << " disconnected." << std::endl;
                }
            }
        }
    }

    close(listen_fd);
    close(epfd);
    return 0;
}