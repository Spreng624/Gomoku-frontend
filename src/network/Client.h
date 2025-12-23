#ifndef CLIENT_H
#define CLIENT_H

#include "Frame.h"
#include "Packet.h"
#include "Crypto.h"
#include "Logger.h"
#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <thread>

#ifdef _WIN32
// Windows 平台
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#undef byte
using SOCKET_TYPE = SOCKET;
#else
// Linux 平台
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
using SOCKET_TYPE = int;
#endif

class Client
{
private:
    std::string server_ip;
    int server_port;
    SOCKET_TYPE sock;
    bool is_running;
    std::thread worker_thread; // 工作线程

    // 只有一个 SessionContext，直接集成
    std::unique_ptr<SessionContext> context;

    // 接收缓冲区
    std::vector<uint8_t> buffer;

    // Packet回调函数
    std::function<void(const Packet &)> packetCallback;

    // Session成功回调函数
    std::function<void(uint64_t sessionId)> sessionActivatedCallback;

    // 断开连接回调函数
    std::function<void()> disconnectedCallback;

    // 网络基础函数 (复用 Server 风格)
    bool InitializeNetworking();
    void CleanupNetworking();
    bool SetNonBlocking(SOCKET_TYPE sock);

    // 核心处理逻辑
    int SendFrame(Frame frame);
    int OnFrame(Frame frame);

    // 内部主循环（线程执行）
    void MainLoop();

public:
    Client(const std::string &ip, int port);
    ~Client();

    bool Connect();
    int Disconnect();

    // 上层业务接口
    uint64_t GetSessionId() const;
    int SendPacket(const Packet &packet);
    bool IsConnected() const;

    // 设置Packet回调
    void SetPacketCallback(std::function<void(const Packet &)> callback);

    // 设置Session激活回调
    void SetSessionActivatedCallback(std::function<void(uint64_t sessionId)> callback);

    // 设置断开连接回调
    void SetDisconnectedCallback(std::function<void()> callback);
};

#endif