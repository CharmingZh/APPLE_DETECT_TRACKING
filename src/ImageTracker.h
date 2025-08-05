#ifndef IMAGETRACKER_H
#define IMAGETRACKER_H

#include "utils/DataTypes.h"
#include "utils/ThreadSafeQueue.h"
#include "TrackManager.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <thread> // 包含了thread头文件

class ImageTracker {
public:
    // [关键修改] 将内部结构体从 Config 重命名为 Settings
    // 它只负责携带那些每次运行都可能不同的参数
    struct Settings {
        std::string input_path;
        bool save_video = true;
        bool save_csv = true;
    };

    // [关键修改] 构造函数使用新的 Settings 类型
    ImageTracker(const Settings& config);
    ~ImageTracker();
    void run();

private:
    void producer_thread_main(unsigned int num_consumers);
    void consumer_thread_main();
    void main_processing_loop();

    void visualize(cv::Mat& frame, int frame_idx, const std::unordered_map<int, TrackedObject>& objects, const cv::Mat& labels_in_roi);
    void save_video();
    void process_and_output_statistics();

    // [关键修改] 成员变量的类型也同步更新
    Settings m_config;
    std::atomic<bool> m_is_running = {true};

    std::vector<std::string> m_image_files;
    int m_total_frames = 0;

    ThreadSafeQueue<ProducerTask> m_input_queue;
    ThreadSafeQueue<ConsumerResult> m_output_queue;
    std::vector<std::thread> m_threads;

    TrackManager m_track_manager;
    std::unordered_map<int, TrackedObject> m_tracked_objects;

    std::vector<TrackingStats> m_all_stats_data;
    cv::VideoWriter m_video_writer;
    std::vector<cv::Mat> m_frame_buffer_for_video;
};

#endif //IMAGETRACKER_H