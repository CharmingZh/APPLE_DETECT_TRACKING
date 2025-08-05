#include "ImageTracker.h"
#include "config/Configuration.h" // 包含配置文件
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
    // 循环处理在 Configuration.h 中定义的每一个数据集
    for (int index : Config::DATASET_INDICES_TO_RUN) {
        // 安全检查，防止索引越界
        if (index >= Config::DATASETS_PATH.size()) {
            std::cerr << "[Warning] Index " << index << " is out of bounds for DATASETS_PATH. Skipping." << std::endl;
            continue;
        }

        std::string current_folder = Config::DATASETS_PATH[index];
        std::cout << "\n\n--- Processing Folder: " << current_folder << " ---" << std::endl;

        // [关键修正]
        // 使用我们在 ImageTracker.h 中定义的新名称 Settings
        ImageTracker::Settings settings;
        settings.input_path = current_folder;
        // settings.save_video 和 .save_csv 会使用在 Settings 结构体中定义的默认值 (true)

        try {
            // 将新的 settings 对象传递给 ImageTracker 的构造函数
            ImageTracker tracker(settings);
            tracker.run();
        } catch (const std::exception& e) {
            std::cerr << "[FATAL ERROR] in folder " << current_folder << ": " << e.what() << std::endl;
        }
    }

    std::cout << "\n\n--- All selected datasets have been processed. ---" << std::endl;
    cv::waitKey(0); // 等待按键后退出，方便查看控制台输出
    return 0;
}