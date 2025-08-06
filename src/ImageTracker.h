#ifndef IMAGETRACKER_H
#define IMAGETRACKER_H

#include "utils/DataTypes.h"
#include "utils/ThreadSafeQueue.h"
#include "TrackManager.h"
#include "SimpleSerial.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <thread>

class ImageTracker {
public:
    struct Settings {
        std::string input_path; // 仅用于数据集模式
        bool save_video = true;
        bool save_csv = true;
    };

    ImageTracker(const Settings& config);
    ~ImageTracker();

    // 为两种模式提供不同的入口函数
    void runFromDataset(SimpleSerial* serial); // 用于本地数据集
    void runFromCamera(SimpleSerial* serial);  // 用于实时相机

private:
    void producer_thread_from_files(unsigned int num_consumers);
    void consumer_thread();

    void visualize(cv::Mat& frame, int frame_idx, const std::unordered_map<int, TrackedObject>& objects, const cv::Mat& labels_in_roi);
    void save_video();
    void process_and_output_statistics();

    Settings m_config;
    std::atomic<bool> m_is_running = {true};
    SimpleSerial* m_serial = nullptr;

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