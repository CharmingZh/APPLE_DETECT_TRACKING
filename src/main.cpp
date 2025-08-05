#include "ImageTracker.h"
#include "config/Configuration.h"
#include "SimpleSerial.h" // [新增]
#include <iostream>
#include <vector>
#include <string>

// --- (新增) Windows平台相关的代码 ---
#ifdef _WIN32
#include <windows.h> // 引入 Windows 头文件

// 一个辅助函数，用于开启控制台的ANSI颜色支持
void enable_virtual_terminal_processing() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        return;
    }
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        return;
    }
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        return;
    }
}
#endif
// --- 新增代码结束 ---

// 在ImageTracker的循环外部定义，以便在所有数据集处理中共享
// [修改] 将tracker的生命周期移到循环外，以便在数据集之间保留其状态
// 这可能需要对ImageTracker的构造和运行方式做更大调整，为简单起见，我们暂时仍在循环内创建
// 但串口对象应该在最外层
int main(int argc, char* argv[]) {

    // --- (新增) 在程序开始时调用激活函数 ---
    #ifdef _WIN32
        enable_virtual_terminal_processing();
    #endif

    // 初始化串口通信
    // !!! 重要: 请根据你的设备管理器中的Arduino端口号修改 "COM3" !!!
    SimpleSerial serial("COM3", 9600);
    if (!serial.isConnected()) {
        std::cerr << "Serial connection failed. Continuing without hardware control." << std::endl;
    }

    // 循环处理在 Configuration.h 中定义的每一个数据集
    for (int index : Config::DATASET_INDICES_TO_RUN) {
        if (index >= Config::DATASETS_PATH.size()) {
            std::cerr << "[Warning] Index " << index << " is out of bounds. Skipping." << std::endl;
            continue;
        }

        std::string current_folder = Config::DATASETS_PATH[index];
        std::cout << "\n\n--- Processing Folder: " << current_folder << " ---" << std::endl;

        try {
            ImageTracker::Settings settings;
            settings.input_path = current_folder;

            ImageTracker tracker(settings);

            // [关键修正] 将串口对象的地址传递给 run 方法
            tracker.run(&serial);

        } catch (const std::exception& e) {
            std::cerr << "[FATAL ERROR] in folder " << current_folder << ": " << e.what() << std::endl;
        }
    }

    std::cout << "\n\n--- All selected datasets have been processed. ---" << std::endl;
    cv::waitKey(0);
    return 0;
}