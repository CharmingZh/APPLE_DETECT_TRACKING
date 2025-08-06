#include "ImageTracker.h"
#include "ImageProcessor.h"
#include "config/Configuration.h"
#include "SimpleSerial.h"
#include "KinectManager.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <map>
#include <vector>
#include <string>

namespace fs = std::filesystem;

// 构造函数
ImageTracker::ImageTracker(const Settings& config) : m_config(config) {}

// 析构函数
ImageTracker::~ImageTracker() {
    m_is_running = false;
    for (auto& t : m_threads) {
        if (t.joinable()) t.join();
    }
    if (m_video_writer.isOpened()) {
        m_video_writer.release();
    }
}

// --- 数据集模式入口 ---
void ImageTracker::runFromDataset(SimpleSerial* serial) {
    m_serial = serial;
    m_is_running = true;

    for (const auto& entry : fs::directory_iterator(m_config.input_path)) {
        if (entry.path().extension() == ".png" || entry.path().extension() == ".jpg") {
            m_image_files.push_back(entry.path().string());
        }
    }
    std::sort(m_image_files.begin(), m_image_files.end());
    m_total_frames = m_image_files.size();
    if (m_total_frames == 0) {
        throw std::runtime_error("No images found in the specified directory!");
    }

    unsigned int num_consumers = (std::max)(1u, std::thread::hardware_concurrency() - 2);
    if (num_consumers == 0) num_consumers = 1;

    m_threads.emplace_back(&ImageTracker::producer_thread_from_files, this, num_consumers);
    for (unsigned int i = 0; i < num_consumers; ++i) {
        m_threads.emplace_back(&ImageTracker::consumer_thread, this);
    }

    int processed_frame_count = 0;
    while(processed_frame_count < m_total_frames && m_is_running) {
        ConsumerResult result;
        if(m_output_queue.try_pop(result)) {
            m_track_manager.update(result, m_tracked_objects);
            cv::Mat display_frame = result.original_image.clone();
            visualize(display_frame, result.frame_idx, m_tracked_objects, result.labels);
            cv::resize(display_frame, display_frame, Config::DISPLAY_SIZE);
            cv::imshow("Apple Tracker", display_frame);
            if (m_config.save_video) m_frame_buffer_for_video.push_back(display_frame);

            auto fired_actions = m_track_manager.getAndClearFiredActions();
            for (const auto& action : fired_actions) {
                std::cout << "[ACTION TRIGGERED] Firing action: " << action.action_type << std::endl;
                if (m_serial && m_serial->isConnected()) m_serial->write(std::string(1, action.action_type));
            }

            if (cv::waitKey(1) == 27) m_is_running = false;
            processed_frame_count++;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    m_is_running = false;

    for (auto& t : m_threads) {
        if (t.joinable()) t.join();
    }

    cv::destroyAllWindows();
    save_video();
    process_and_output_statistics();
    std::cout << "[Info] Processing finished for folder: " << m_config.input_path << std::endl;
}

// --- 实时相机模式入口 ---
void ImageTracker::runFromCamera(SimpleSerial* serial) {
    m_serial = serial;
    m_is_running = true;

    KinectManager kinect;
    if (!kinect.isOpened()) {
        throw std::runtime_error("Failed to initialize Kinect camera.");
    }

    unsigned int num_consumers = (std::max)(1u, std::thread::hardware_concurrency() - 2);
    if (num_consumers == 0) num_consumers = 1;
    for (unsigned int i = 0; i < num_consumers; ++i) {
        m_threads.emplace_back(&ImageTracker::consumer_thread, this);
    }

    int frame_idx = 0;
    while (m_is_running) {
        cv::Mat color_frame;
        if (!kinect.getNextFrame(color_frame)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        m_input_queue.push({frame_idx, color_frame});

        ConsumerResult result;
        if (m_output_queue.try_pop(result)) {
            m_track_manager.update(result, m_tracked_objects);

            cv::Mat display_frame = result.original_image.clone();
            visualize(display_frame, result.frame_idx, m_tracked_objects, result.labels);

            cv::resize(display_frame, display_frame, Config::DISPLAY_SIZE);
            cv::imshow("Apple Tracker", display_frame);

            if (m_config.save_video) {
                if(!m_video_writer.isOpened()){
                    std::string output_video_path = "output/live_session.mp4";
                    fs::create_directories(fs::path(output_video_path).parent_path());
                    m_video_writer.open(output_video_path, Config::VIDEO_CODEC, Config::VIDEO_FPS, display_frame.size());
                }
                if(m_video_writer.isOpened()) m_video_writer.write(display_frame);
            }
        }

        auto fired_actions = m_track_manager.getAndClearFiredActions();
        for (const auto& action : fired_actions) {
            std::cout << "[ACTION TRIGGERED] Firing action: " << action.action_type << std::endl;
            if (m_serial && m_serial->isConnected()) {
                m_serial->write(std::string(1, action.action_type));
            }
        }

        if (cv::waitKey(1) == 27) {
            m_is_running = false;
        }
        frame_idx++;
    }

    for (size_t i = 0; i < m_threads.size(); ++i) {
        m_input_queue.push({-1, cv::Mat()});
    }
    for (auto& t : m_threads) {
        if (t.joinable()) t.join();
    }
    cv::destroyAllWindows();
    if(m_video_writer.isOpened()) m_video_writer.release();
}

void ImageTracker::producer_thread_from_files(unsigned int num_consumers) {
    for (int i = 0; i < m_total_frames && m_is_running; ++i) {
        cv::Mat img = cv::imread(m_image_files[i]);
        if (img.empty()) continue;
        m_input_queue.push({i, img});
    }
    for (unsigned int i = 0; i < num_consumers; ++i) {
        m_input_queue.push({-1, cv::Mat()});
    }
}

void ImageTracker::consumer_thread() {
    while (m_is_running) {
        ProducerTask task;
        m_input_queue.wait_and_pop(task);
        if (task.frame_idx == -1) break;
        ConsumerResult result = ImageProcessor::process_frame(task);
        m_output_queue.push(result);
    }
}

void ImageTracker::visualize(cv::Mat& display_frame, int frame_idx,
                           const std::unordered_map<int, TrackedObject>& objects,
                           const cv::Mat& labels_in_roi) {
    for (const auto& pair : objects) {
        const auto& obj = pair.second;
        if (obj.missed_frames == 0) {
            cv::rectangle(display_frame, obj.current_bbox, obj.color, 2);
            cv::Point text_pos(obj.current_bbox.x + Config::OBJECT_ID_TEXT_OFFSET.x, obj.current_bbox.y + Config::OBJECT_ID_TEXT_OFFSET.y);
            cv::putText(display_frame, std::to_string(obj.assigned_number), text_pos, Config::FONT_FACE, Config::FONT_SCALE_OBJECT_ID, Config::TEXT_COLOR_OBJECT_ID, Config::LINE_THICKNESS);
        }
    }
    cv::rectangle(display_frame, Config::ROI_A, Config::ROI_RECT_COLOR, Config::LINE_THICKNESS);
    cv::rectangle(display_frame, Config::ROI_B, Config::ROI_RECT_COLOR, Config::LINE_THICKNESS);
    cv::putText(display_frame, "Line A", {Config::ROI_A.x, Config::ROI_A.y - 10}, Config::FONT_FACE, 0.8, Config::ROI_RECT_COLOR, 2);
    cv::putText(display_frame, "Line B", {Config::ROI_B.x, Config::ROI_B.y - 10}, Config::FONT_FACE, 0.8, Config::ROI_RECT_COLOR, 2);
    cv::putText(display_frame, "Frame: " + std::to_string(frame_idx), Config::FRAME_COUNTER_POS, Config::FONT_FACE, Config::FONT_SCALE_FRAME_COUNTER, Config::FRAME_COUNTER_COLOR, Config::LINE_THICKNESS);
}

void ImageTracker::save_video() {
    if (!Config::USE_LIVE_CAMERA && m_config.save_video && !m_frame_buffer_for_video.empty()) {
        std::string folder_name = fs::path(m_config.input_path).filename().string();
        std::string output_video_path = "output/" + folder_name + "/" + Config::OUTPUT_VIDEO_FILENAME;
        fs::create_directories(fs::path(output_video_path).parent_path());
        cv::VideoWriter writer;
        writer.open(output_video_path, Config::VIDEO_CODEC, Config::VIDEO_FPS, m_frame_buffer_for_video[0].size());
        if(writer.isOpened()){
            for(const auto& frame : m_frame_buffer_for_video){
                writer.write(frame);
            }
        }
    }
}

void ImageTracker::process_and_output_statistics() {
    if(Config::USE_LIVE_CAMERA) return;

    std::string folder_name = fs::path(m_config.input_path).filename().string();
    if (m_all_stats_data.empty()) {
        std::cout << "No tracking data was collected." << std::endl;
        return;
    }
    std::map<int, std::vector<TrackingStats>> grouped_stats;
    for (const auto& stat : m_all_stats_data) {
        grouped_stats[stat.assigned_number].push_back(stat);
    }
    struct Summary {
        int assigned_number;
        int frame_count;
        double speed_pixels_per_sec;
    };
    std::vector<Summary> summaries;
    for (auto const& [assigned_num, group] : grouped_stats) {
        if (group.size() <= Config::MIN_TRACK_LENGTH_FOR_STATS) continue;
        if (group.size() < 2 * Config::TRIM_FRAMES_FROM_ENDS + 2) continue;
        std::vector<TrackingStats> trimmed_group(group.begin() + Config::TRIM_FRAMES_FROM_ENDS, group.end() - Config::TRIM_FRAMES_FROM_ENDS);

        const auto& first_row = trimmed_group.front();
        const auto& last_row = trimmed_group.back();
        cv::Point2f start_point(first_row.centroid_x, first_row.centroid_y);
        cv::Point2f end_point(last_row.centroid_x, last_row.centroid_y);
        double pixel_distance = cv::norm(end_point - start_point);

        double time_duration = (last_row.frame - first_row.frame) / Config::VIDEO_FPS;
        double speed_pps = (time_duration > 0) ? (pixel_distance / time_duration) : 0.0;
        summaries.push_back({assigned_num, (int)trimmed_group.size(), speed_pps});
    }
    if (summaries.empty()) {
        std::cout << "No tracks met the criteria for statistical summary." << std::endl;
        return;
    }
    std::cout << "\n--- Instance Statistics Summary ---" << std::endl;
    std::cout << std::left << std::setw(20) << "assigned_number"
              << std::setw(15) << "frame_count"
              << "speed_pixels_per_sec" << std::endl;
    for (const auto& summary : summaries) {
        std::cout << std::left << std::setw(20) << summary.assigned_number
                  << std::setw(15) << summary.frame_count
                  << std::fixed << std::setprecision(2) << summary.speed_pixels_per_sec << std::endl;
    }
    if (m_config.save_csv) {
        std::string csv_path = "output/" + folder_name + "/" + Config::OUTPUT_CSV_FILENAME;
        fs::create_directories(fs::path(csv_path).parent_path());
        std::ofstream csv_file(csv_path);
        if (csv_file.is_open()) {
            csv_file << "assigned_number,frame_count,speed_pixels_per_sec\n";
            for (const auto& summary : summaries) {
                csv_file << summary.assigned_number << ","
                         << summary.frame_count << ","
                         << std::fixed << std::setprecision(2) << summary.speed_pixels_per_sec << "\n";
            }
            std::cout << "\nStatistics summary saved to: " << csv_path << std::endl;
        } else {
            std::cerr << "[Error] Could not open file for writing: " << csv_path << std::endl;
        }
    }
}