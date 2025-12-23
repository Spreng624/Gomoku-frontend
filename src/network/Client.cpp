#include "Client.h"
#include "Logger.h"
#include <thread>
#include <chrono>
#include <algorithm>

#define BUFFER_SIZE 4096

// --- 跨平台宏定义 (与 Server 保持一致) ---
#ifdef _WIN32
#define GET_LAST_ERROR() WSAGetLastError()
#define WOULD_BLOCK_ERROR WSAEWOULDBLOCK
#define CLOSE_SOCKET(s) closesocket(s)
#else
#define GET_LAST_ERROR() errno
#define WOULD_BLOCK_ERROR EWOULDBLOCK
#define CLOSE_SOCKET(s) close(s)
#endif

#include "TimeWheel.hpp"
#include "Crypto.h"

// --- 网络初始化实现 ---
#ifdef _WIN32
bool Client::InitializeNetworking()
{
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
}
void Client::CleanupNetworking() { WSACleanup(); }
bool Client::SetNonBlocking(SOCKET_TYPE s)
{
    u_long mode = 1;
    return ioctlsocket(s, FIONBIO, &mode) == 0;
}
#else
bool Client::InitializeNetworking() { return true; }
void Client::CleanupNetworking() {}
bool Client::SetNonBlocking(SOCKET_TYPE s)
{
    int flags = fcntl(s, F_GETFL, 0);
    return (flags != -1) && (fcntl(s, F_SETFL, flags | O_NONBLOCK) != -1);
}
#endif

int printHex(const std::vector<uint8_t> &data)
{
    for (auto byte : data)
    {
        printf("%02X ", byte);
    }
    printf("\n");
    return 0;
}

// --- Client 实现 ---

Client::Client(const std::string &ip, int port)
    : server_ip(ip), server_port(port), is_running(false)
{
    // 初始 Context 为空，连接建立后再初始化
    context = nullptr;
    packetCallback = nullptr;

    LOG_DEBUG("Client constructor called for " + ip + ":" + std::to_string(port));

    // 网络初始化
    if (!InitializeNetworking())
    {
        LOG_ERROR("Net init failed.");
        return;
    }
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == (SOCKET_TYPE)INVALID_SOCKET)
    {
        LOG_ERROR("Socket create failed: " + std::to_string(GET_LAST_ERROR()));
        return;
    }

    LOG_DEBUG("Socket created successfully");
}

Client::~Client()
{
    LOG_DEBUG("Client destructor called");
    Disconnect();

    if (sock != (SOCKET_TYPE)INVALID_SOCKET)
    {
        CLOSE_SOCKET(sock);
        sock = (SOCKET_TYPE)INVALID_SOCKET;
        LOG_DEBUG("Socket closed.");
    }
    CleanupNetworking();
    LOG_DEBUG("Client cleanup completed");
}

bool Client::Connect()
{
    LOG_INFO("Attempting to connect to server " + server_ip + ":" + std::to_string(server_port));

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

    LOG_DEBUG("Connecting to " + server_ip + ":" + std::to_string(server_port) + "...");
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        LOG_ERROR("Connect failed: " + std::to_string(GET_LAST_ERROR()));
        return false;
    }

    LOG_INFO("Connected to server successfully!");

    if (!SetNonBlocking(sock))
    {
        LOG_WARN("Failed to set socket to non-blocking mode");
    }
    else
    {
        LOG_DEBUG("Socket set to non-blocking mode");
    }

    is_running = true;
    worker_thread = std::thread(&Client::MainLoop, this);
    LOG_DEBUG("Worker thread started");

    LOG_DEBUG("Sending Hello frame to initiate handshake");
    SendFrame(Frame(Frame::Status::Hello, 0, {}, {}));

    LOG_INFO("Connection established and handshake initiated");
    return true;
}

int Client::Disconnect()
{
    LOG_DEBUG("Disconnect called, setting is_running to false");
    is_running = false;

    if (worker_thread.joinable())
    {
        LOG_DEBUG("Waiting for worker thread to finish...");
        worker_thread.join();
        LOG_DEBUG("Worker thread joined successfully.");
    }
    else
    {
        LOG_DEBUG("Worker thread is not joinable or already finished");
    }

    if (sock != (SOCKET_TYPE)INVALID_SOCKET)
    {
        LOG_DEBUG("Closing socket");
        CLOSE_SOCKET(sock);
    }

    if (context)
    {
        LOG_DEBUG("Resetting session context");
        context.reset();
    }

    LOG_INFO("Client disconnected successfully.");
    return 0;
}

void Client::MainLoop()
{
    if (!is_running)
        return;

    LOG_INFO("Client loop started.");
    char temp_buf[BUFFER_SIZE];
    timeval timeout = {0, 100 * 1000}; // 100ms select timeout
    uint64_t lastHeartbeatTime = GetTimeMS();
    const uint64_t HEARTBEAT_INTERVAL = 10000; // 10秒发送一次心跳

    while (is_running)
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);

        // 修复select调用：在Windows上使用0，在Linux上使用sock+1
#ifdef _WIN32
        int activity = select(0, &read_fds, NULL, NULL, &timeout);
#else
        int activity = select(sock + 1, &read_fds, NULL, NULL, &timeout);
#endif

        if (activity < 0)
        {
            LOG_ERROR("Select error: " + std::to_string(GET_LAST_ERROR()));
            break;
        }

        if (activity > 0 && FD_ISSET(sock, &read_fds))
        {
            int n = recv(sock, temp_buf, BUFFER_SIZE, 0);
            if (n > 0)
            {
                buffer.insert(buffer.end(), temp_buf, temp_buf + n);

                // 解析 Frame
                Frame frame;
                while (frame.ReadStream(buffer))
                {
                    OnFrame(frame);
                }
            }
            else if (n == 0 || (n < 0 && GET_LAST_ERROR() != WOULD_BLOCK_ERROR))
            {
                LOG_WARN("Server disconnected.");
                is_running = false;
            }
        }

        // 心跳发送逻辑
        uint64_t currentTime = GetTimeMS();
        if (context && context->isActive && (currentTime - lastHeartbeatTime) > HEARTBEAT_INTERVAL)
        {
            SendPacket(Packet());
            lastHeartbeatTime = currentTime;
            LOG_DEBUG("Heartbeat sent.");
        }
    }
}

int Client::SendFrame(Frame frame)
{
    if (is_running == false)
    {
        LOG_ERROR("Socket not connected.");
        return -1;
    }
    std::vector<uint8_t> data = frame.ToBytes();
    printHex(data);
    LOG_DEBUG("Sending frame: " + std::to_string((int)frame.head.status));
    int sent = send(sock, reinterpret_cast<const char *>(data.data()), (int)data.size(), 0);
    return sent;
}

int Client::OnFrame(Frame frame)
{
    // 状态机逻辑
    switch (frame.head.status)
    {
    case Frame::Status::NewSession:
        // Step 1: 服务器分配了 SessionID
        LOG_INFO("[Handshake] Received NewSession. ID: " + std::to_string(frame.head.sessionId));
        printHex(frame.ToBytes());
        context = std::make_unique<SessionContext>((int)sock, (uint64_t)frame.head.sessionId);

        // 保存服务器公钥 (假设 frame.data 包含 pk + sig)
        // 需要解析服务器公钥
        if (!frame.data.empty() && frame.data.size() >= 32)
        {
            // 假设前32字节是公钥，后面是签名
            context->pk2.assign(frame.data.begin(), frame.data.begin() + 32);
            LOG_DEBUG("Saved server public key");
        }

        context->KeyGen(); // 生成客户端密钥对

        // 回复 Pending 帧，带上客户端公钥
        {
            std::vector<uint8_t> my_pk_sig = context->Get_Pk_Sig();
            SendFrame(Frame(Frame::Status::Pending, context->sessionId, {}, my_pk_sig));
        }
        break;

    case Frame::Status::NoSession:
        LOG_ERROR("Server reported NoSession. Session may have expired.");
        // 可以尝试重新发送Hello帧
        SendFrame(Frame(Frame::Status::Hello, 0, {}, {}));
        break;

    case Frame::Status::Activated:
        // Step 2: 服务器确认激活
        if (context && context->sessionId == frame.head.sessionId)
        {
            LOG_INFO("[Handshake] Session Activated!");
            context->CalculateSharedKey();
            context->isActive = true;
        }
        break;

    case Frame::Status::Inactive:
        LOG_WARN("Session is inactive. Need to re-authenticate.");
        context->isActive = false;
        break;

    case Frame::Status::Active:
        LOG_DEBUG("Received Active frame.");
        // 收到业务数据
        if (context && context->isActive)
        {
            if (context->Decrypt(frame.data))
            {
                context->lastHeartbeat = GetTimeMS(); // 更新心跳

                Packet packet;
                if (packet.FromData(context->sessionId, frame.data))
                {
                    LOG_DEBUG("[Packet] Received Packet Type: " + std::to_string((int)packet.msgType));
                    // 调用回调函数将Packet传递给上层
                    if (packetCallback)
                    {
                        packetCallback(packet);
                    }
                }
            }
            else
            {
                LOG_ERROR("Decrypt failed.");
            }
        }
        else
        {
            LOG_WARN("Received Active frame but session is not active.");
        }
        break;

    case Frame::Status::Error:
        LOG_ERROR("Server reported Error.");
        break;

    case Frame::Status::InvalidRequest:
        LOG_ERROR("Server reported InvalidRequest. Check your request format.");
        break;

    case Frame::Status::Success:
        LOG_INFO("Operation succeeded.");
        break;

    default:
        LOG_WARN("Unhandled status: " + std::to_string((int)frame.head.status));
        break;
    }
    return 0;
}

int Client::SendPacket(const Packet &packet)
{
    // TODO: 完成握手过程功能再启用校验
    // if (!context || !context->isActive)
    // {
    //     LOG_ERROR("Session not active, cannot send.");
    //     return -1;
    // }

    // 构造 IV 和加密
    context->nextIV();
    std::vector<uint8_t> iv = context->iv;
    std::array<uint8_t, 16> iv_arr;
    std::copy(iv.begin(), iv.end(), iv_arr.begin());

    std::vector<uint8_t> data = packet.ToBytes();
    context->Encrypt(data);

    Frame frame(Frame::Status::Active, context->sessionId, iv_arr, data);
    return SendFrame(frame);
}

bool Client::IsConnected() const
{
    return is_running && context && context->isActive;
}

void Client::SetPacketCallback(std::function<void(const Packet &)> callback)
{
    packetCallback = callback;
}

uint64_t Client::GetSessionId() const
{
    return context->sessionId;
}