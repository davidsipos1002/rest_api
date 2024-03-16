#pragma once

#include <chrono>
#include <iostream>
#include <string>

template <class T>
struct ClassName
{
};

template <class T>
void log(const std::string &message)
{
    auto now = std::chrono::system_clock::now();
    time_t time = std::chrono::system_clock::to_time_t(now);
    char *timeString = std::ctime(&time);
    timeString[strlen(timeString) - 1] = 0;
    std::cout << "[" << ClassName<T>::name << " : " << timeString << "] " << message;
    std::cout << std::endl;
}
