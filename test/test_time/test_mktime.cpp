#include <iostream>
#include <chrono>
#include <ctime>
 
int main() {
    std::tm timeinfo = {};                 // 创建一个空的std::tm结构体
    std::istringstream ss("2022-01-01");   // 设置日期字符串
    ss >> std::get_time(&timeinfo, "%Y-%m-%d");   // 解析日期字符串
 
    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&timeinfo));   // 转换为时间点类型
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();   // 获取时间戳
 
    std::cout << "时间戳：" << timestamp << std::endl;
 
    return 0;
}