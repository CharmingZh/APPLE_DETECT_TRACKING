#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <set>

namespace Config {
    // =================================================================
    // 1. 全局模式开关
    // =================================================================
    // 设置为 true 则使用实时相机模式，设置为 false 则使用本地数据集模式
    // 注意：如果使用本地数据集模式，则需要确保 DATASETS_PATH 中的路径正确
    //       并且 DATASET_INDICES_TO_RUN 中的索引在 DATASETS_PATH 范围内
    constexpr bool USE_LIVE_CAMERA = false;

    // =================================================================
    // 2. 通用配置
    // =================================================================
    constexpr int MIN_AREA_THRESHOLD = 2750;
    constexpr float MAX_DISTANCE_FOR_TRACKING = 150.0f;
    constexpr int MAX_MISSED_FRAMES = 5;
    constexpr float VIDEO_FPS = 30.0f;
    const cv::Size DISPLAY_SIZE = {1280, 720};
    constexpr int ACTION_DELAY_MS = 500;

#if USE_LIVE_CAMERA
    // =================================================================
    // 3.A 实时相机模式配置
    // =================================================================
    const cv::Rect ROI_A = {100, 50, 900, 250};
    const cv::Rect ROI_B = {100, 400, 900, 250};
    const cv::Scalar LOWER_HSV = {15, 100, 100};
    const cv::Scalar UPPER_HSV = {35, 255, 255};
    const cv::Point2f INITIAL_MOVEMENT = {20.0f, 0.0f};
    constexpr int START_NUMBER_A = 1;
    constexpr int START_NUMBER_B = 1001;
    namespace SortingLogic {
        const std::set<int> SEQUENCE_A = {1, 3, 5};
        const std::set<int> SEQUENCE_B = {1002, 1004};
    }
#else
    // =================================================================
    // 3.B 本地数据集模式配置
    // =================================================================
    const std::vector<std::string> DATASETS_PATH = {
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
    const std::vector<int> DATASET_INDICES_TO_RUN = {0, 5, 7, 11, 14, 15, 17, 19, 20, 23, 25};
    const cv::Rect ROI_A = {820, 100, 200, 900};
    const cv::Rect ROI_B = {1130, 100, 200, 900};
    const cv::Scalar LOWER_HSV = {10, 40, 40};
    const cv::Scalar UPPER_HSV = {40, 255, 255};
    const cv::Point2f INITIAL_MOVEMENT = {0.0f, -20.0f};
    constexpr int START_NUMBER_A = 1;
    constexpr int START_NUMBER_B = 1001;
    namespace SortingLogic {
        const std::set<int> SEQUENCE_A = {1, 8};
        const std::set<int> SEQUENCE_B = {1001, 1002};
    }
#endif

    // =================================================================
    // 4. 其他通用参数
    // =================================================================
    constexpr int MORPH_KERNEL_SIZE = 5;
    constexpr int MORPH_ITERATIONS = 1;
    const int FONT_FACE = cv::FONT_HERSHEY_SIMPLEX;
    const double FONT_SCALE_OBJECT_ID = 0.7;
    const double FONT_SCALE_FRAME_COUNTER = 1.0;
    const int LINE_THICKNESS = 2;
    const cv::Scalar TEXT_COLOR_OBJECT_ID = cv::Scalar(255, 255, 255);
    const cv::Scalar ROI_RECT_COLOR = cv::Scalar(0, 255, 255);
    const cv::Scalar FRAME_COUNTER_COLOR = cv::Scalar(0, 255, 0);
    const cv::Point OBJECT_ID_TEXT_OFFSET = cv::Point(5, 20);
    const cv::Point FRAME_COUNTER_POS = cv::Point(10, 30);
    const int MIN_TRACK_LENGTH_FOR_STATS = 20;
    const int TRIM_FRAMES_FROM_ENDS = 10;
    const std::string OUTPUT_VIDEO_FILENAME = "tracked_video.mp4";
    const int VIDEO_CODEC = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
    const std::string OUTPUT_CSV_FILENAME = "instance_summary.csv";
}
#endif