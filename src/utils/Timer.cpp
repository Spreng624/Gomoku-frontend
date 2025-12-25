#include "Timer.hpp"

// 静态成员定义
TimerManager *TimerManager::instance_ = nullptr;
std::mutex TimerManager::instance_mutex_;