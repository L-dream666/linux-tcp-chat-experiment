CC = gcc
# 多线程必须加 -pthread 参数
CFLAGS = -pthread

all: server client

# 编译服务端
server: server.c
	$(CC) server.c -o server $(CFLAGS)

# 编译客户端
client: client.c
	$(CC) client.c -o client $(CFLAGS)

# 清理编译生成的可执行文件
clean:
	rm -f server client
