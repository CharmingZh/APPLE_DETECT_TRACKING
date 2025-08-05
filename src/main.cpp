#include "ImageTracker.h"
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
    std::vector<std::string> datasets_path = {
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-160338", // 0
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-160531", // 1
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-160719", // 2
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-161139", // 3
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-161332", // 4
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-161505", // 5
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-161852", // 6
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-162139", // 7
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-162525", // 8
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-175040", // 9
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-175246", // 10
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-175537", // 11
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-175758", // 12
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-175936", // 13
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-180049", // 14
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-180705", // 15
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-180852", // 16
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-181203", // 17
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-181933", // 18
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-182312", // 19
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-182845", // 20
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-183526", // 21
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-183759", // 22
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-183958", // 23
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-184356", // 24
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-184500", // 25
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-184648", // 26
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-184859", // 27
        "C:/Users/JmZha/VSCode_Project/Datasets/Apple/recording_20250107-185322"  // 28
    };
    std::vector<int> speed_1_lst = {0, 5, 7, 11, 14, 15, 17, 19, 20, 23, 25};

    for (int index : speed_1_lst) {
        if (index >= datasets_path.size()) {
            std::cerr << "[Warning] Index " << index << " is out of bounds. Skipping." << std::endl;
            continue;
        }
        std::string current_folder = datasets_path[index];
        std::cout << "\n\n--- Processing Folder: " << current_folder << " ---" << std::endl;
        ImageTracker::Config config;
        config.input_path = current_folder;
        try {
            ImageTracker tracker(config);
            tracker.run();
        } catch (const std::exception& e) {
            std::cerr << "[FATAL ERROR] in folder " << current_folder << ": " << e.what() << std::endl;
        }
    }
    std::cout << "\n\n--- All selected datasets have been processed. ---" << std::endl;
    cv::waitKey(0);
    return 0;
}