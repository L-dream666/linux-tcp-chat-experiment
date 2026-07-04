#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_CLIENT 10
#define PORT 8888
#define BUF_SIZE 128

// 存储所有客户端文件描述符
int client_fd_list[MAX_CLIENT] = {0};
// 互斥锁，防止多线程同时操作客户端数组
pthread_mutex_t lock;

// 单个客户端处理线程函数
void *client_handle(void *arg)
{
    int fd = *(int *)arg;
    free(arg);
    char recv_buf[BUF_SIZE] = {0};
    int read_len;

    while (1)
    {
        memset(recv_buf, 0, sizeof(recv_buf));
        read_len = read(fd, recv_buf, BUF_SIZE - 1);

        // 客户端断开连接
        if (read_len <= 0)
        {
            pthread_mutex_lock(&lock);
            for (int i = 0; i < MAX_CLIENT; i++)
            {
                if (client_fd_list[i] == fd)
                    client_fd_list[i] = 0;
            }
            pthread_mutex_unlock(&lock);
            close(fd);
            printf("客户端 %d 已下线\n", fd);
            return NULL;
        }

        // 广播消息给所有在线客户端
        pthread_mutex_lock(&lock);
        for (int i = 0; i < MAX_CLIENT; i++)
        {
            if (client_fd_list[i] != 0)
            {
                write(client_fd_list[i], recv_buf, read_len);
            }
        }
        pthread_mutex_unlock(&lock);
    }
}

int main()
{
    pthread_mutex_init(&lock, NULL);

    // 创建socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket创建失败");
        exit(EXIT_FAILURE);
    }

    // 端口复用，避免重启服务端口被占用
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 绑定端口
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind绑定端口失败");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 开启监听
    listen(server_fd, 5);
    printf("聊天室服务端启动成功！监听端口：%d，最大在线人数：%d\n", PORT, MAX_CLIENT);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int new_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (new_fd < 0)
        {
            perror("accept等待连接失败");
            continue;
        }

        // 判断是否还有空位接入新客户端
        int have_slot = 0;
        pthread_mutex_lock(&lock);
        for (int i = 0; i < MAX_CLIENT; i++)
        {
            if (client_fd_list[i] == 0)
            {
                client_fd_list[i] = new_fd;
                have_slot = 1;
                break;
            }
        }
        pthread_mutex_unlock(&lock);

        if (!have_slot)
        {
            printf("在线人数已满，拒绝客户端 %d\n", new_fd);
            close(new_fd);
            continue;
        }

        printf("新客户端接入，fd=%d\n", new_fd);
        // 分配内存传给线程
        int *fd_ptr = malloc(sizeof(int));
        *fd_ptr = new_fd;
        pthread_t tid;
        pthread_create(&tid, NULL, client_handle, fd_ptr);
        pthread_detach(tid); // 分离线程，自动回收资源
    }

    close(server_fd);
    pthread_mutex_destroy(&lock);
    return 0;
}
