#include "ImageTracker.h"
#include "ImageProcessor.h"
#include "config/Configuration.h"
#include "SimpleSerial.h" // For serial communication
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <map>
#include <vector>
#include <string>

namespace fs = std::filesystem;

// Helper function for natural sorting of filenames
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

// Constructor: Initializes the tracker with settings and finds image files
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
        throw std::runtime_error("No images found in the specified directory!");
    }
}

// Destructor: Cleans up threads and video writer
ImageTracker::~ImageTracker() {
    m_is_running = false;
    for (auto& t : m_threads) {
        if (t.joinable()) t.join();
    }
    if (m_video_writer.isOpened()) {
        m_video_writer.release();
    }
}

// Main entry point to start the tracking process
void ImageTracker::run(SimpleSerial* serial) {
    m_serial = serial; // Store the serial port object
    m_is_running = true;

    // Determine the number of consumer threads based on hardware concurrency
    // [关键修正] 使用括号来避免Windows平台上的宏冲突
    unsigned int num_consumers = (std::max)(1u, std::thread::hardware_concurrency() - 2);
    if (num_consumers == 0) num_consumers = 1;

    // Start producer and consumer threads
    m_threads.emplace_back(&ImageTracker::producer_thread_main, this, num_consumers);
    for (unsigned int i = 0; i < num_consumers; ++i) {
        m_threads.emplace_back(&ImageTracker::consumer_thread_main, this);
    }

    // Start the main processing loop in the current thread
    main_processing_loop();

    // Wait for all threads to finish
    for (auto& t : m_threads) {
        if (t.joinable()) t.join();
    }

    // Finalize by saving video and statistics
    save_video();
    process_and_output_statistics();
    std::cout << "[Info] Processing finished for folder: " << m_config.input_path << std::endl;
}

// Producer thread: Reads images from disk and pushes them to a queue
void ImageTracker::producer_thread_main(unsigned int num_consumers) {
    for (int i = 0; i < m_total_frames && m_is_running; ++i) {
        cv::Mat img = cv::imread(m_image_files[i]);
        if (img.empty()) continue;
        m_input_queue.push({i, img});
    }
    // Send sentinel values to signal consumers to stop
    for (unsigned int i = 0; i < num_consumers; ++i) {
        m_input_queue.push({-1, cv::Mat()});
    }
}

// Consumer thread: Takes images from the queue and processes them
void ImageTracker::consumer_thread_main() {
    while (m_is_running) {
        ProducerTask task;
        m_input_queue.wait_and_pop(task);
        if (task.frame_idx == -1) break; // Sentinel value received
        ConsumerResult result = ImageProcessor::process_frame(task);
        m_output_queue.push(result);
    }
}

// Main loop: Processes results, visualizes, and handles actions
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

            // Update tracking logic
            m_track_manager.update(current_result, m_tracked_objects);

            // Collect statistics
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

            // Create display frame and visualize
            cv::Mat display_frame = current_result.original_image.clone();
            visualize(display_frame, current_result.frame_idx, m_tracked_objects, current_result.labels);

            // Resize and show the frame
            cv::Mat resized_display;
            cv::resize(display_frame, resized_display, Config::DISPLAY_SIZE);
            cv::imshow("Apple Tracker", resized_display);

            if (m_config.save_video) {
                m_frame_buffer_for_video.push_back(resized_display);
            }

            // --- Asynchronous Action Handling ---
            // Check for any actions that are due to be fired
            auto fired_actions = m_track_manager.getAndClearFiredActions();
            for (const auto& action : fired_actions) {
                std::cout << "[ACTION TRIGGERED] Firing action: " << action.action_type << std::endl;
                if (m_serial && m_serial->isConnected()) {
                    m_serial->write(std::string(1, action.action_type));
                }
            }

            // Handle user input (ESC to quit)
            if (cv::waitKey(1) == 27) {
                m_is_running = false;
                break;
            }

            processed_frame_count++;
            next_expected_frame++;
        }
    }
    cv::destroyAllWindows();
}

// Visualization function for drawing objects and ROIs
void ImageTracker::visualize(cv::Mat& display_frame, int frame_idx,
                           const std::unordered_map<int, TrackedObject>& objects,
                           const cv::Mat& labels_in_roi) {

    // Draw all currently tracked objects
    for (const auto& pair : objects) {
        const auto& obj = pair.second;
        if (obj.missed_frames == 0) {
            // Draw a bounding box around the object
            cv::rectangle(display_frame, obj.current_bbox, obj.color, 2);

            // Draw the object's assigned number
            cv::Point text_pos(
                obj.current_bbox.x + Config::OBJECT_ID_TEXT_OFFSET.x,
                obj.current_bbox.y + Config::OBJECT_ID_TEXT_OFFSET.y
            );
            cv::putText(display_frame, std::to_string(obj.assigned_number), text_pos,
                        Config::FONT_FACE, Config::FONT_SCALE_OBJECT_ID, Config::TEXT_COLOR_OBJECT_ID, Config::LINE_THICKNESS);
        }
    }

    // Draw the two independent ROI boxes
    cv::rectangle(display_frame, Config::ROI_A, Config::ROI_RECT_COLOR, Config::LINE_THICKNESS);
    cv::rectangle(display_frame, Config::ROI_B, Config::ROI_RECT_COLOR, Config::LINE_THICKNESS);

    // Add labels to the ROI boxes
    cv::putText(display_frame, "Line A", {Config::ROI_A.x, Config::ROI_A.y - 10}, Config::FONT_FACE, 0.8, Config::ROI_RECT_COLOR, 2);
    cv::putText(display_frame, "Line B", {Config::ROI_B.x, Config::ROI_B.y - 10}, Config::FONT_FACE, 0.8, Config::ROI_RECT_COLOR, 2);

    // Draw the current frame number
    cv::putText(display_frame, "Frame: " + std::to_string(frame_idx), Config::FRAME_COUNTER_POS,
                Config::FONT_FACE, Config::FONT_SCALE_FRAME_COUNTER, Config::FRAME_COUNTER_COLOR, Config::LINE_THICKNESS);
}

// Saves the processed frames to a video file
void ImageTracker::save_video() {
    std::string folder_name = fs::path(m_config.input_path).filename().string();
    std::string output_video_path = "output/" + folder_name + "/tracked_video.mp4";

    if (m_config.save_video && !m_frame_buffer_for_video.empty()) {
        fs::create_directories(fs::path(output_video_path).parent_path());

        m_video_writer.open(output_video_path, cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
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

// Processes and outputs tracking statistics
void ImageTracker::process_and_output_statistics() {
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
        std::string csv_path = "output/" + folder_name + "/instance_summary.csv";
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