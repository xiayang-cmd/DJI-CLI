#include "common_types.h"

std::queue<DataFrame> g_dataFrameQueue;
std::mutex g_queueMutex;
std::condition_variable g_queueCond;

std::queue<DataFrame> g_recvRawDataFrameQueue;
std::mutex g_recvRawQueueMutex;
std::condition_variable g_recvRawQueueCond;

std::queue<DataFrame> g_completeDataFrameQueue;
std::mutex g_completeQueueMutex;
std::condition_variable g_completeQueueCond;

ServerConfig g_serverConfig;