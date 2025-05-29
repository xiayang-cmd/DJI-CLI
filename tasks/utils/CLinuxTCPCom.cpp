#include "CLinuxTCPCom.h"
#include <mutex>

std::mutex send_mutex;

CLinuxTCPCom::CLinuxTCPCom()
{
    listen_fd = -1;
    comm_fd = -1;
    memset(&ser_addr, 0, sizeof(ser_addr));
    memset(&cli_addr, 0, sizeof(cli_addr));
    memset(recv_buf, 0, sizeof(recv_buf));
}

CLinuxTCPCom::~CLinuxTCPCom()
{
    CloseFd();
}

int CLinuxTCPCom::TCPInitServer(const char *ip_str, uint16_t port)
{
    // 1. 创建TCP套接字
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        printf("Create socket fail! errno=%d\n", errno);
        return -1;
    }

    // 允许地址端口重用（可选）
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. 绑定IP和端口
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = inet_addr(ip_str);
    ser_addr.sin_port = htons(port);

    if (bind(listen_fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) < 0)
    {
        printf("Bind fail! errno=%d\n", errno);
        close(listen_fd);
        listen_fd = -1;
        return -1;
    }

    // 3. 开始监听
    if (listen(listen_fd, 5) < 0) // backlog=5
    {
        printf("Listen fail! errno=%d\n", errno);
        close(listen_fd);
        listen_fd = -1;
        return -1;
    }

    printf("TCP Server Init Success! Listening on %s:%d\n", ip_str, port);
    return 0;
}

int CLinuxTCPCom::TCPAccept()
{
    if (listen_fd < 0)
    {
        printf("TCPAccept fail! No valid listen fd.\n");
        return -1;
    }

    socklen_t cli_len = sizeof(cli_addr);
    int new_fd = accept(listen_fd, (struct sockaddr *)&cli_addr, &cli_len); // 
    if (new_fd < 0)
    {
        printf("Accept fail! errno=%d\n", errno);
        return -1;
    }

    // 将通信描述符设置为新产生的套接字
    comm_fd = new_fd;
    printf("Client connected: %s:%d\n",
           inet_ntoa(cli_addr.sin_addr),
           ntohs(cli_addr.sin_port));
    return comm_fd;
}

int CLinuxTCPCom::TCPInitClient(const char *ip_str, uint16_t port)
{
    // 1. 创建TCP套接字
    comm_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (comm_fd < 0)
    {
        printf("Create socket fail! errno=%d\n", errno);
        return -1;
    }

    // 2. 设置服务器地址
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = inet_addr(ip_str);
    ser_addr.sin_port = htons(port);

    // 3. connect连接服务器
    if (connect(comm_fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) < 0)
    {
        printf("Connect to server fail! errno=%d\n", errno);
        close(comm_fd);
        comm_fd = -1;
        return -1;
    }

    printf("TCP Client Connect to %s:%d Success!\n", ip_str, port);
    return 0;
}

int CLinuxTCPCom::TCPSendData(const void *buf, size_t size)
{
    std::lock_guard<std::mutex> lock(send_mutex);

    if (comm_fd < 0)
    {
        printf("No valid communication fd to send data.\n");
        return -1;
    }
    if (NULL == buf || size == 0)
    {
        printf("Invalid buffer or size to send.\n");
        return -1;
    }

    ssize_t sent = send(comm_fd, buf, size, MSG_NOSIGNAL); // MSG_NOSIGNAL: 防止SIGPIPE信号导致进程退出
    if (sent < 0)
    {
        printf("Send data fail! errno=%d\n", errno);
        return -1;
    }

    return (int)sent;
}

int CLinuxTCPCom::TCPRecvData(void *buf, size_t size)
{
    if (comm_fd < 0)
    {
        printf("No valid communication fd to receive data.\n");
        return -1;
    }

    if (NULL == buf || size == 0)
    {
        printf("Invalid buffer or size to receive.\n");
        return -1;
    }

    ssize_t received = recv(comm_fd, buf, size, 0);
    if (received < 0)
    {
        printf("Receive data fail! errno=%d\n", errno);
        return -1;
    }

    // Note: If received = 0, it usually means the peer closed the connection.
    return (int)received;
}

void CLinuxTCPCom::SetCommFd(int fd)
{
    comm_fd = fd;
}

int CLinuxTCPCom::GetCommFd() const
{
    return comm_fd;
}

void CLinuxTCPCom::CloseFd()
{
    if (comm_fd >= 0)
    {
        close(comm_fd);
        comm_fd = -1;
    }

    if (listen_fd >= 0)
    {
        close(listen_fd);
        listen_fd = -1;
    }
}

// 为了唤醒阻塞在recv的线程，必须调用shutdown
// shutdown(tcpCom.GetCommFd(), SHUT_RDWR);