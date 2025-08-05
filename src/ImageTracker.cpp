#include "ImageTracker.h"
#include "ImageProcessor.h"
#include <thread>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <map>

namespace fs = std::filesystem;

bool natural_sort_compare(const fs::path& a, const fs::path& b) {
    auto extract_number = [](const std::string& s) {
        size_t start = s.find("frame_");
        if (start == std::string::npos) return -1;
        start += 6;
        size_t end = s.find('_', start);
        if (end == std::string::npos) return -1;
        try {
            return std::stoi(s.substr(start, end - start));
        } catch (...) {
            return -1;
        }
    };
    return extract_number(a.filename().string()) < extract_number(b.filename().string());
}

ImageTracker::ImageTracker(const Config& config)
    : m_config(config) {

    std::string folder_name = fs::path(m_config.input_path).filename().string();
    m_output_dir = "output/" + folder_name;
    fs::create_directories(m_output_dir);

    std::cout << "[Info] Mode: Image Sequence from path: " << m_config.input_path << std::endl;
    for (const auto& entry : fs::directory_iterator(m_config.input_path)) {
        if (entry.path().extension() == ".png" || entry.path().extension() == ".jpg") {
            m_image_files.push_back(entry.path().string());
        }
    }
    std::sort(m_image_files.begin(), m_image_files.end(),
        [](const std::string& a, const std::string& b){
            return natural_sort_compare(fs::path(a), fs::path(b));
        });

    m_total_frames = m_image_files.size();
    if (m_total_frames == 0) {
        throw std::runtime_error("No images found in the specified directory!");
    }
    std::cout << "[Info] Found " << m_total_frames << " images." << std::endl;

    m_roi = cv::Rect(820, 100, 1360 - 820, 1030 - 100);
    m_display_size = {1440, 810};
}

ImageTracker::~ImageTracker() {
    m_is_running = false;
    for (auto& t : m_threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    if (m_video_writer.isOpened()) {
        m_video_writer.release();
    }
}

void ImageTracker::run() {
    m_is_running = true;

    unsigned int num_consumers = std::max(1u, std::thread::hardware_concurrency() - 2);
    if (num_consumers == 0) num_consumers = 1;
    m_threads.emplace_back(&ImageTracker::producer_thread_main, this, num_consumers);
    for (unsigned int i = 0; i < num_consumers; ++i) {
        m_threads.emplace_back(&ImageTracker::consumer_thread_main, this);
    }
    std::cout << "[Info] Started 1 producer and " << num_consumers << " consumer threads." << std::endl;

    main_processing_loop();

    for (auto& t : m_threads) {
        if (t.joinable()) t.join();
    }

    save_video();
    process_and_output_statistics();

    std::cout << "[Info] Processing finished for folder: " << m_config.input_path << std::endl;
}

void ImageTracker::producer_thread_main(unsigned int num_consumers) {
    for (int i = 0; i < m_total_frames && m_is_running; ++i) {
        cv::Mat img = cv::imread(m_image_files[i]);
        if (img.empty()) continue;
        m_input_queue.push({i, img});
    }
    for (unsigned int i = 0; i < num_consumers; ++i) {
        m_input_queue.push({-1, cv::Mat()});
    }
}

void ImageTracker::consumer_thread_main() {
    while (true) {
        ProducerTask task;
        m_input_queue.wait_and_pop(task);
        if (task.frame_idx == -1 || !m_is_running) {
            break;
        }
        ConsumerResult result = ImageProcessor::process_frame(task, m_roi);
        m_output_queue.push(result);
    }
}

void ImageTracker::visualize(cv::Mat& display_frame, int frame_idx,
                           const std::unordered_map<int, TrackedObject>& objects,
                           const cv::Mat& labels_in_roi) {
    cv::Mat display_roi = display_frame(m_roi);
    for (const auto& pair : objects) {
        const auto& obj = pair.second;
        if (obj.missed_frames == 0 && obj.current_label_id != -1) {
            display_roi.setTo(obj.color, labels_in_roi == obj.current_label_id);
            cv::Point text_pos(
                obj.current_bbox.x + m_roi.x + 5,
                obj.current_bbox.y + m_roi.y + 20
            );
            cv::putText(display_frame, std::to_string(obj.assigned_number), text_pos,
                        cv::FONT_HERSHEY_SIMPLEX, 0.7, {255, 255, 255}, 2);
        }
    }
    cv::rectangle(display_frame, m_roi, {0, 255, 255}, 2);
    cv::putText(display_frame, "Frame: " + std::to_string(frame_idx), {10, 30}, cv::FONT_HERSHEY_SIMPLEX, 1, {0, 255, 0}, 2);
}

void ImageTracker::save_video() {
    if (m_config.save_video && !m_frame_buffer_for_video.empty()) {
        std::string output_video_path = m_output_dir + "/tracked_video.mp4";
        m_video_writer.open(output_video_path, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), m_source_fps, m_frame_buffer_for_video[0].size());
        if (!m_video_writer.isOpened()) {
            std::cerr << "[Warning] Could not open VideoWriter. Video will not be saved." << std::endl;
        } else {
             for(const auto& frame : m_frame_buffer_for_video) {
                m_video_writer.write(frame);
             }
             std::cout << "[Info] Video saved to " << output_video_path << std::endl;
        }
    }
}

void ImageTracker::process_and_output_statistics() {
    if (m_all_stats_data.empty()) {
        std::cout << "没有收集到追踪数据。" << std::endl;
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
        if (group.size() <= 20) continue;
        std::vector<TrackingStats> trimmed_group(group.begin() + 10, group.end() - 10);
        if (trimmed_group.size() < 2) continue;
        const auto& first_row = trimmed_group.front();
        const auto& last_row = trimmed_group.back();
        cv::Point2f start_point(first_row.centroid_x, first_row.centroid_y);
        cv::Point2f end_point(last_row.centroid_x, last_row.centroid_y);
        double pixel_distance = cv::norm(end_point - start_point);
        double time_duration = (last_row.frame - first_row.frame) / m_source_fps;
        double speed_pps = (time_duration > 0) ? (pixel_distance / time_duration) : 0.0;
        summaries.push_back({assigned_num, (int)trimmed_group.size(), speed_pps});
    }
    if (summaries.empty()) {
        std::cout << "筛选后没有符合条件的追踪数据。" << std::endl;
        return;
    }
    std::cout << "\n--- 实例统计摘要 ---" << std::endl;
    std::cout << std::left << std::setw(20) << "assigned_number"
              << std::setw(15) << "frame_count"
              << "speed_pixels_per_sec" << std::endl;
    for (const auto& summary : summaries) {
        std::cout << std::left << std::setw(20) << summary.assigned_number
                  << std::setw(15) << summary.frame_count
                  << std::fixed << std::setprecision(2) << summary.speed_pixels_per_sec << std::endl;
    }
    if (m_config.save_csv) {
        std::string csv_path = m_output_dir + "/instance_summary.csv";
        std::ofstream csv_file(csv_path);
        if (csv_file.is_open()) {
            csv_file << "assigned_number,frame_count,speed_pixels_per_sec\n";
            for (const auto& summary : summaries) {
                csv_file << summary.assigned_number << ","
                         << summary.frame_count << ","
                         << std::fixed << std::setprecision(2) << summary.speed_pixels_per_sec << "\n";
            }
            std::cout << "\n统计摘要已保存到: " << csv_path << std::endl;
        } else {
            std::cerr << "[Error] 无法打开文件用于写入: " << csv_path << std::endl;
        }
    }
}


void ImageTracker::main_processing_loop() {
    auto compare_results = [](const ConsumerResult& a, const ConsumerResult& b) { return a.frame_idx > b.frame_idx; };
    std::priority_queue<ConsumerResult, std::vector<ConsumerResult>, decltype(compare_results)> results_buffer(compare_results);

    int processed_frame_count = 0;
    int next_expected_frame = 0;

    while (m_is_running) {
        ConsumerResult result;
        if (!m_output_queue.try_pop(result)) {
             if (processed_frame_count >= m_total_frames) {
                m_is_running = false;
             }
             std::this_thread::sleep_for(std::chrono::milliseconds(5));
             continue;
        }
        results_buffer.push(result);

        while (!results_buffer.empty() && results_buffer.top().frame_idx == next_expected_frame) {
            ConsumerResult current_result = results_buffer.top();
            results_buffer.pop();

            m_track_manager.update(current_result, m_tracked_objects);

            for (const auto& pair : m_tracked_objects) {
                const auto& obj = pair.second;
                if (obj.missed_frames == 0) {
                    m_all_stats_data.push_back({
                        current_result.frame_idx,
                        obj.assigned_number,
                        obj.unique_id,
                        obj.centroid.x,
                        obj.centroid.y
                    });
                }
            }

            cv::Mat display_frame = current_result.original_image.clone();
            visualize(display_frame, current_result.frame_idx, m_tracked_objects, current_result.labels);

            cv::Mat resized_display;
            cv::resize(display_frame, resized_display, m_display_size);
            cv::imshow("Apple Tracker", resized_display);

            if (m_config.save_video) {
                m_frame_buffer_for_video.push_back(resized_display);
            }

            if (cv::waitKey(1) == 27) { // ESC
                m_is_running = false;
                break;
            }

            processed_frame_count++;
            next_expected_frame++;
        }
    }
    cv::destroyAllWindows();
}