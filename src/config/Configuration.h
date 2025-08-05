#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

namespace Config {
    // 1. 数据集路径
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
    // 选择要运行的数据集索引
    const std::vector<int> DATASET_INDICES_TO_RUN = {0, 5, 7, 11, 14, 15, 17, 19, 20, 23, 25};


    // =================================================================
    // 2. 核心图像处理参数 (来自你的原始文件)
    // 这些参数可能在 ImageProcessor.cpp 中使用
    // =================================================================
    // 感兴趣区域 (Region of Interest, ROI) - 你的原始值 {820, 100, 540, 930}
    const cv::Rect ROI = {820, 100, 1360 - 820, 1030 - 100};

    // =================================================================
    // [核心修改] 为两条线定义独立的ROI区域
    // =================================================================
    // !!! 请根据你的视频画面精确调整这两个矩形框的位置和大小 !!!
    // 格式：{x坐标, y坐标, 宽度, 高度}
    const cv::Rect ROI_A = {820, 100, 200, 900}; // 左侧通道 A 的ROI
    const cv::Rect ROI_B = {1130, 100, 250, 900}; // 右侧通道 B 的ROI

    // HSV 颜色阈值，用于分割目标
    const cv::Scalar LOWER_HSV = {10, 40, 40};
    const cv::Scalar UPPER_HSV = {40, 255, 255};
    // 形态学操作的内核大小和迭代次数
    constexpr int MORPH_KERNEL_SIZE = 5;
    constexpr int MORPH_ITERATIONS = 1;


    // =================================================================
    // 3. 追踪算法参数 (来自你的原始文件)
    // 这些参数可能在 TrackManager.cpp 中使用
    // =================================================================
    // 被视为有效目标的最小轮廓面积
    constexpr int MIN_AREA_THRESHOLD = 2750;
    // 帧间关联时，新旧目标质心的最大允许距离
    constexpr float MAX_DISTANCE_FOR_TRACKING = 100.0f;
    // 目标被判定为消失前，允许的最大连续未匹配帧数
    constexpr int MAX_MISSED_FRAMES = 5;
    // (用途待定) 初始垂直移动量
    constexpr float INITIAL_VERTICAL_MOVEMENT = -20.0f;
    // // 左右两侧目标的起始编号
    // constexpr int START_NUMBER_LEFT = 1;
    // constexpr int START_NUMBER_RIGHT = 10;

    // 为通道A和B分配独立的起始编号，确保编号组之间不重复
    constexpr int START_NUMBER_A = 1;    // 通道A的目标将从1开始编号 (1, 2, 3...)
    constexpr int START_NUMBER_B = 1001; // 通道B的目标将从1001开始编号 (1001, 1002...)


    // =================================================================
    // 4. 视频、显示与输出参数 (合并了你的和新增的参数)
    // =================================================================
    // 视频帧率 (FPS) - 使用你文件中的 'f' 后缀，表示float类型
    constexpr float VIDEO_FPS = 30.0f;
    // 显示窗口的尺寸 - 使用你文件中的值
    const cv::Size DISPLAY_SIZE = {1440, 810};
    // [新增] 输出视频的文件名
    const std::string OUTPUT_VIDEO_FILENAME = "tracked_video.mp4";
    // [新增] 输出视频的编码器
    const int VIDEO_CODEC = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
    // [新增] 输出统计摘要的CSV文件名
    const std::string OUTPUT_CSV_FILENAME = "instance_summary.csv";


    // =================================================================
    // 5. 可视化参数 (新增，用于美化和统一显示效果)
    // =================================================================
    // 字体
    const int FONT_FACE = cv::FONT_HERSHEY_SIMPLEX;
    // 目标ID数字的字体大小
    const double FONT_SCALE_OBJECT_ID = 0.7;
    // 左上角帧计数器的字体大小
    const double FONT_SCALE_FRAME_COUNTER = 1.0;
    // 所有绘制线条的粗细 (文本、框)
    const int LINE_THICKNESS = 2;
    // 目标ID数字的颜色 (白色)
    const cv::Scalar TEXT_COLOR_OBJECT_ID = cv::Scalar(255, 255, 255);
    // ROI矩形框的颜色 (青色)
    const cv::Scalar ROI_RECT_COLOR = cv::Scalar(0, 255, 255);
    // 帧计数器的颜色 (绿色)
    const cv::Scalar FRAME_COUNTER_COLOR = cv::Scalar(0, 255, 0);
    // 目标ID文本相对于其包围盒左上角的偏移量
    const cv::Point OBJECT_ID_TEXT_OFFSET = cv::Point(5, 20);
    // 帧计数器文本在窗口中的位置
    const cv::Point FRAME_COUNTER_POS = cv::Point(10, 30);


    // =================================================================
    // 6. 统计分析参数 (新增，用于控制统计逻辑)
    // =================================================================
    // 一个目标至少要被追踪多少帧，才会被纳入最终的统计摘要
    const int MIN_TRACK_LENGTH_FOR_STATS = 20;
    // 在计算速度前，从每个追踪轨迹的开始和结尾去掉多少帧，以消除噪声
    const int TRIM_FRAMES_FROM_ENDS = 10;

    // =================================================================
    // [新增] 分拣与延时逻辑配置
    // =================================================================

    // 目标退出后，等待多少毫秒再执行分拣动作
    constexpr int ACTION_DELAY_MS = 500; // 延时500毫秒

    namespace SortingLogic {
        // 定义一个序列，只对这个序列中的第n个目标进行分拣
        // 例如：{1, 3, 4} 表示只分拣第1、第3、第4个目标

        // // 左侧通道 A 的分拣序列
        // const std::set<int> SEQUENCE_A = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        //
        // // 右侧通道 B 的分拣序列
        // const std::set<int> SEQUENCE_B = {10, 11, 12, 13, 14, 15, 16, 17, 18};

        // 左侧通道 A 的分拣序列
        const std::set<int> SEQUENCE_A = {1, 8};

        // 右侧通道 B 的分拣序列
        const std::set<int> SEQUENCE_B = {1001, 1002};
    }

} // namespace Config

#endif //APPLE_RGBD_CONFIGURATION_H