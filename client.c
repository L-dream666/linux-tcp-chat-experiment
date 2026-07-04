#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE 128

// 子线程：持续接收服务器广播消息
void *recv_msg_thread(void *arg)
{
    int sock_fd = *(int *)arg;
    free(arg);
    char buf[BUF_SIZE] = {0};
    int recv_len;

    while (1)
    {
        memset(buf, 0, sizeof(buf));
        recv_len = read(sock_fd, buf, BUF_SIZE - 1);
        if (recv_len <= 0)
        {
            printf("\n服务器断开连接，程序退出！\n");
            close(sock_fd);
            exit(0);
        }
        // 打印群聊消息
        printf("\n【群消息】%s\n请输入发送内容：", buf);
    }
}

int main(int argc, char *argv[])
{
    // 校验启动参数
    if (argc != 3)
    {
        printf("使用格式：./client 服务端IP 端口号\n");
        printf("示例：./client 127.0.0.1 8888\n");
        return -1;
    }

    int port = atoi(argv[2]);
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        perror("socket 创建失败");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    // 转换IP地址
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0)
    {
        perror("IP地址格式错误");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // 连接服务端
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("连接服务器失败");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    printf("成功连接聊天室服务端！可以开始聊天\n");

    // 创建接收消息子线程
    int *fd_ptr = malloc(sizeof(int));
    *fd_ptr = sock_fd;
    pthread_t tid;
    pthread_create(&tid, NULL, recv_msg_thread, fd_ptr);
    pthread_detach(tid);

    // 主线程：读取键盘输入并发送
    char send_buf[BUF_SIZE] = {0};
    while (1)
    {
        printf("请输入发送内容：");
        fgets(send_buf, BUF_SIZE, stdin);
        write(sock_fd, send_buf, strlen(send_buf));
        memset(send_buf, 0, sizeof(send_buf));
    }

    close(sock_fd);
    return 0;
}
