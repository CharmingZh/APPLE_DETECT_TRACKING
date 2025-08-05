#include "ImageTracker.h"
#include "ImageProcessor.h"
#include "config/Configuration.h" // [关键] 包含新的全局配置文件
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <map>

namespace fs = std::filesystem;

// 自然排序函数保持不变
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

// [关键修改] 构造函数定义要与头文件中的声明匹配
ImageTracker::ImageTracker(const Settings& config) : m_config(config) {
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
        throw std::runtime_error("No images found!");
    }
}

ImageTracker::~ImageTracker() {
    m_is_running = false;
    for (auto& t : m_threads) {
        if (t.joinable()) t.join();
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
    main_processing_loop();
    for (auto& t : m_threads) {
        if (t.joinable()) t.join();
    }
    save_video();
    process_and_output_statistics();
    std::cout << "[Info] Processing finished for folder: " << m_config.input_path << std::endl;
}

// producer 和 consumer 线程函数保持不变
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
    // 从全局 Config 命名空间读取 ROI
    cv::Mat display_roi = display_frame(Config::ROI);

    for (const auto& pair : objects) {
        const auto& obj = pair.second;
        if (obj.missed_frames == 0 && obj.current_label_id != -1) {
            display_roi.setTo(obj.color, labels_in_roi == obj.current_label_id);
            // 使用 Config 中的偏移量、字体、颜色等参数
            cv::Point text_pos(
                obj.current_bbox.x + Config::ROI.x + Config::OBJECT_ID_TEXT_OFFSET.x,
                obj.current_bbox.y + Config::ROI.y + Config::OBJECT_ID_TEXT_OFFSET.y
            );
            cv::putText(display_frame, std::to_string(obj.assigned_number), text_pos,
                        Config::FONT_FACE, Config::FONT_SCALE_OBJECT_ID, Config::TEXT_COLOR_OBJECT_ID, Config::LINE_THICKNESS);
        }
    }
    // 使用 Config 中的颜色和粗细
    cv::rectangle(display_frame, Config::ROI, Config::ROI_RECT_COLOR, Config::LINE_THICKNESS);
    // 使用 Config 中的位置、字体、颜色等参数
    cv::putText(display_frame, "Frame: " + std::to_string(frame_idx), Config::FRAME_COUNTER_POS,
                Config::FONT_FACE, Config::FONT_SCALE_FRAME_COUNTER, Config::FRAME_COUNTER_COLOR, Config::LINE_THICKNESS);
}

void ImageTracker::save_video() {
    std::string folder_name = fs::path(m_config.input_path).filename().string();
    // 使用 Config 中的输出路径和文件名
    std::string output_video_path = "output/" + folder_name + "/" + Config::OUTPUT_VIDEO_FILENAME;

    if (m_config.save_video && !m_frame_buffer_for_video.empty()) {
        // 确保输出目录存在
        fs::create_directories(fs::path(output_video_path).parent_path());

        // 使用 Config 中的编码器和帧率
        m_video_writer.open(output_video_path, Config::VIDEO_CODEC,
                            Config::VIDEO_FPS, m_frame_buffer_for_video[0].size());
        if (!m_video_writer.isOpened()) {
            std::cerr << "[Warning] Could not open VideoWriter." << std::endl;
        } else {
             for(const auto& frame : m_frame_buffer_for_video) {
                m_video_writer.write(frame);
             }
             std::cout << "[Info] Video saved to " << output_video_path << std::endl;
        }
    }
}

void ImageTracker::process_and_output_statistics() {
    std::string folder_name = fs::path(m_config.input_path).filename().string();
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
        // 使用 Config 中的最小轨迹长度
        if (group.size() <= Config::MIN_TRACK_LENGTH_FOR_STATS) continue;

        // 使用 Config 中的轨迹修剪长度
        if (group.size() < 2 * Config::TRIM_FRAMES_FROM_ENDS + 2) continue; // 确保修剪后至少剩2帧
        std::vector<TrackingStats> trimmed_group(group.begin() + Config::TRIM_FRAMES_FROM_ENDS, group.end() - Config::TRIM_FRAMES_FROM_ENDS);

        const auto& first_row = trimmed_group.front();
        const auto& last_row = trimmed_group.back();
        cv::Point2f start_point(first_row.centroid_x, first_row.centroid_y);
        cv::Point2f end_point(last_row.centroid_x, last_row.centroid_y);
        double pixel_distance = cv::norm(end_point - start_point);

        // 使用 Config 中的视频帧率
        double time_duration = (last_row.frame - first_row.frame) / Config::VIDEO_FPS;
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
        // 使用 Config 中的CSV文件名
        std::string csv_path = "output/" + folder_name + "/" + Config::OUTPUT_CSV_FILENAME;
        // 确保输出目录存在
        fs::create_directories(fs::path(csv_path).parent_path());
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

// main_processing_loop 保持不变，除了 resize() 函数
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
            // 使用 Config 中的显示尺寸
            cv::resize(display_frame, resized_display, Config::DISPLAY_SIZE);
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