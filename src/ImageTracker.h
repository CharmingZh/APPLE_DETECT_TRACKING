#ifndef IMAGETRACKER_H
#define IMAGETRACKER_H

#include "utils/DataTypes.h"
#include "utils/ThreadSafeQueue.h"
#include "TrackManager.h"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>

class ImageTracker {
public:
    struct Config {
        std::string input_path;
        bool save_video = true;
        bool save_csv = true;
    };

    ImageTracker(const Config& config);
    ~ImageTracker();
    void run();

private:
    void producer_thread_main(unsigned int num_consumers);
    void consumer_thread_main();
    void main_processing_loop();

    void visualize(cv::Mat& frame, int frame_idx,
                   const std::unordered_map<int, TrackedObject>& objects,
                   const cv::Mat& labels_in_roi);
    void save_video();
    void process_and_output_statistics();

    Config m_config;
    std::atomic<bool> m_is_running = {true};

    std::string m_output_dir;
    cv::Rect m_roi;
    cv::Size m_display_size;
    const float m_source_fps = 30.0f;

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