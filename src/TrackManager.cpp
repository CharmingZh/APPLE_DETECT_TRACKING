#include "TrackManager.h"
#include "config/Configuration.h"
#include "hungarian/Hungarian.h"
#include <set>
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <functional> // 为了使用 std::function

// --- 颜色定义和序数词函数保持不变 ---
const std::string ANSI_COLOR_CYAN   = "\033[36m";
const std::string ANSI_COLOR_GREEN  = "\033[32m";
const std::string ANSI_COLOR_YELLOW = "\033[33m";
const std::string ANSI_COLOR_RESET  = "\033[0m";
std::string getOrdinal(int n) {
    if (n <= 0) return std::to_string(n);
    if (n % 100 >= 11 && n % 100 <= 13) return std::to_string(n) + "th";
    switch (n % 10) {
        case 1: return std::to_string(n) + "st";
        case 2: return std::to_string(n) + "nd";
        case 3: return std::to_string(n) + "rd";
        default: return std::to_string(n) + "th";
    }
}

TrackManager::TrackManager() {
    // [核心修改] 初始化新的编号计数器
    m_next_number_A = Config::START_NUMBER_A;
    m_next_number_B = Config::START_NUMBER_B;
    m_colors = {{255,0,0},{0,255,0},{0,0,255},{255,255,0},{0,255,255},{255,0,255},{128,0,0},{0,128,0},{0,0,128},{128,128,0},{0,128,128},{128,0,128}};
}

void TrackManager::update(const ConsumerResult& result, std::unordered_map<int, TrackedObject>& tracked_objects) {
    // 1. 获取当前帧的所有有效检测结果
    std::vector<Detection> all_detections;
    for (int i = 1; i < result.stats.rows; ++i) {
        if (result.stats.at<int>(i, cv::CC_STAT_AREA) >= Config::MIN_AREA_THRESHOLD) {
            Detection det;
            det.label_id = i;
            det.centroid = cv::Point2f(result.centroids.at<double>(i, 0), result.centroids.at<double>(i, 1));
            det.bbox = cv::Rect(result.stats.at<int>(i, cv::CC_STAT_LEFT), result.stats.at<int>(i, cv::CC_STAT_TOP), result.stats.at<int>(i, cv::CC_STAT_WIDTH), result.stats.at<int>(i, cv::CC_STAT_HEIGHT));
            all_detections.push_back(det);
        }
    }

    // --- 核心重构：定义一个通用的ROI处理函数 ---
    auto process_roi = [&](const cv::Rect& roi,
                           int& next_assigned_number,
                           int& exit_counter,
                           const std::set<int>& sorting_sequence,
                           char action_char) {

        // a. 筛选出属于当前ROI的检测和已追踪对象
        std::vector<Detection> roi_detections;
        for (const auto& det : all_detections) {
            if (roi.contains(det.centroid)) {
                roi_detections.push_back(det);
            }
        }

        std::vector<int> roi_track_ids;
        for (const auto& pair : tracked_objects) {
            if (roi.contains(pair.second.centroid)) {
                roi_track_ids.push_back(pair.first);
            }
        }

        // b. 执行匹配 (仅在当前ROI内)
        std::set<int> matched_track_ids;
        std::set<int> matched_detection_labels;

        if (!roi_track_ids.empty() && !roi_detections.empty()) {
            std::vector<std::vector<double>> cost_matrix(roi_track_ids.size(), std::vector<double>(roi_detections.size(), 1e6));
            for (size_t i = 0; i < roi_track_ids.size(); ++i) {
                const auto& obj = tracked_objects.at(roi_track_ids[i]);
                cv::Point2f predicted_pos = obj.centroid + obj.velocity;
                for (size_t j = 0; j < roi_detections.size(); ++j) {
                    double dist = cv::norm(predicted_pos - roi_detections[j].centroid);
                    if (dist < Config::MAX_DISTANCE_FOR_TRACKING) cost_matrix[i][j] = dist;
                }
            }
            HungarianAlgorithm solver;
            std::vector<int> assignment;
            solver.Solve(cost_matrix, assignment);

            for (size_t i = 0; i < assignment.size(); ++i) {
                if (assignment[i] != -1 && cost_matrix[i][assignment[i]] < Config::MAX_DISTANCE_FOR_TRACKING) {
                    int tid = roi_track_ids[i];
                    const auto& det = roi_detections[assignment[i]];
                    TrackedObject& obj = tracked_objects.at(tid);
                    obj.velocity = (obj.velocity * 0.5) + ((det.centroid - obj.centroid) * 0.5);
                    obj.centroid = det.centroid;
                    obj.missed_frames = 0;
                    obj.current_label_id = det.label_id;
                    obj.current_bbox = det.bbox;
                    matched_track_ids.insert(tid);
                    matched_detection_labels.insert(det.label_id);
                }
            }
        }

        // c. 更新ROI内未匹配的旧目标
        for (int tid : roi_track_ids) {
            if (matched_track_ids.find(tid) == matched_track_ids.end()) {
                tracked_objects.at(tid).missed_frames++;
                tracked_objects.at(tid).current_label_id = -1;
            }
        }

        // d. 创建ROI内的新目标
        for (const auto& det : roi_detections) {
            if (matched_detection_labels.find(det.label_id) == matched_detection_labels.end()) {
                TrackedObject new_obj;
                new_obj.unique_id = m_next_unique_id++;
                new_obj.assigned_number = next_assigned_number++;
                new_obj.centroid = det.centroid;
                new_obj.velocity = {0, Config::INITIAL_VERTICAL_MOVEMENT};
                new_obj.color = m_colors[m_color_index++ % m_colors.size()];
                new_obj.current_label_id = det.label_id;
                new_obj.current_bbox = det.bbox;
                tracked_objects[new_obj.unique_id] = new_obj;
            }
        }

        // e. 清理ROI内“死亡”的目标并触发动作
        for (auto it = roi_track_ids.begin(); it != roi_track_ids.end(); ++it) {
            if (tracked_objects.at(*it).missed_frames > Config::MAX_MISSED_FRAMES) {
                const TrackedObject& dead_object = tracked_objects.at(*it);
                exit_counter++;
                std::cout << ANSI_COLOR_CYAN << "[INFO] Target #" << dead_object.assigned_number << " exited from Line " << action_char
                          << ". It was the " << getOrdinal(exit_counter) << " object on this line." << ANSI_COLOR_RESET << std::endl;

                if (sorting_sequence.count(dead_object.assigned_number)) {
                    auto trigger_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(Config::ACTION_DELAY_MS);
                    m_pending_actions.push_back({action_char, trigger_time});
                    std::cout << ANSI_COLOR_GREEN << "[ACTION BY ID] Queued action '" << action_char << "' for target #" << dead_object.assigned_number << "." << ANSI_COLOR_RESET << std::endl;
                } else {
                    std::cout << ANSI_COLOR_YELLOW << "[SKIP BY ID] Target #" << dead_object.assigned_number << " not in sorting sequence for Line " << action_char << "." << ANSI_COLOR_RESET << std::endl;
                }
                tracked_objects.erase(*it);
            }
        }
    };

    // 2. 独立处理每个ROI
    process_roi(Config::ROI_A, m_next_number_A, m_exit_counter_A, Config::SortingLogic::SEQUENCE_A, 'A');
    process_roi(Config::ROI_B, m_next_number_B, m_exit_counter_B, Config::SortingLogic::SEQUENCE_B, 'B');
}

std::vector<PendingAction> TrackManager::getAndClearFiredActions() {
    std::vector<PendingAction> fired_actions;
    auto now = std::chrono::steady_clock::now();
    auto it = std::remove_if(m_pending_actions.begin(), m_pending_actions.end(),
        [&](const PendingAction& action) {
            if (now >= action.trigger_time) {
                fired_actions.push_back(action);
                return true;
            }
            return false;
        });
    m_pending_actions.erase(it, m_pending_actions.end());
    return fired_actions;
}
