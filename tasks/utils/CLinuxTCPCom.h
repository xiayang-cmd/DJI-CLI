#ifndef C_LINUX_TCP_COM_H
#define C_LINUX_TCP_COM_H

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#define TCP_BUFF_LEN 1024

class CLinuxTCPCom
{
public:
    CLinuxTCPCom();
    ~CLinuxTCPCom();

    /**
     * @brief 初始化为TCP服务器模式
     * @param ip_str 绑定的本机IP地址(如"127.0.0.1")
     * @param port 监听端口号
     * @return 成功返回0，失败返回-1
     * @note 该函数会创建监听套接字并使用bind()和listen()。
     */
    int TCPInitServer(const char *ip_str, uint16_t port);

    /**
     * @brief 等待客户端连接（仅在服务器模式下使用）
     * @return 成功返回新生成的通信socket的文件描述符，失败返回-1
     * @note 此函数调用accept()阻塞等待客户端连接，一旦有连接请求会产生新的套接字用于通信。
     */
    int TCPAccept();

    /**
     * @brief 初始化为TCP客户端模式
     * @param ip_str 服务器IP地址
     * @param port 服务器端口号
     * @return 成功返回0，失败返回-1
     * @note 该函数会创建一个套接字，然后使用connect()与服务器建立连接。
     */
    int TCPInitClient(const char *ip_str, uint16_t port);

    /**
     * @brief 发送数据
     * @param buf 待发送数据的缓冲区指针
     * @param size 待发送数据的长度
     * @return 成功发送的字节数，出错返回-1
     */
    int TCPSendData(const void *buf, size_t size);

    /**
     * @brief 接收数据
     * @param buf 接收数据的缓冲区指针
     * @param size 缓冲区可接收数据的最大长度
     * @return 实际接收到的字节数，出错返回-1（对端关闭连接可能返回0）
     */
    int TCPRecvData(void *buf, size_t size);

    /**
     * @brief 设置通信使用的文件描述符（如服务器accept后的套接字）
     */
    void SetCommFd(int fd);

    /**
     * @brief 获取当前的通信套接字文件描述符
     */
    int GetCommFd() const;

    /**
     * @brief 关闭套接字
     */
    void CloseFd();

private:
    int listen_fd;   // 服务器监听套接字（服务器模式下使用）
    int comm_fd;     // 通信套接字（客户端模式或服务器accept后使用）
    struct sockaddr_in ser_addr;
    struct sockaddr_in cli_addr;
    char recv_buf[TCP_BUFF_LEN];
};

#endif // C_LINUX_TCP_COM_H
