#include "TrackManager.h"
#include "config/Configuration.h"
#include "hungarian/Hungarian.h"
#include <set>

TrackManager::TrackManager() {
    m_next_left_column_number = Config::START_NUMBER_LEFT;
    m_next_right_column_number = Config::START_NUMBER_RIGHT;
    m_colors = {{255,0,0},{0,255,0},{0,0,255},{255,255,0},{0,255,255},{255,0,255},{128,0,0},{0,128,0},{0,0,128},{128,128,0},{0,128,128},{128,0,128}};
}

void TrackManager::update(const ConsumerResult& result, std::unordered_map<int, TrackedObject>& tracked_objects) {
    // 1. 获取当前帧的所有有效检测结果
    std::vector<Detection> current_detections;
    for (int i = 1; i < result.stats.rows; ++i) {
        if (result.stats.at<int>(i, cv::CC_STAT_AREA) >= Config::MIN_AREA_THRESHOLD) {
            Detection det;
            det.label_id = i;
            det.centroid = cv::Point2f(result.centroids.at<double>(i, 0), result.centroids.at<double>(i, 1));
            det.bbox = cv::Rect(result.stats.at<int>(i, cv::CC_STAT_LEFT), result.stats.at<int>(i, cv::CC_STAT_TOP), result.stats.at<int>(i, cv::CC_STAT_WIDTH), result.stats.at<int>(i, cv::CC_STAT_HEIGHT));
            current_detections.push_back(det);
        }
    }

    // 2. 准备数据进行匹配
    std::vector<int> tracked_ids_vec; // 使用不同的名字避免混淆
    for (const auto& pair : tracked_objects) {
        tracked_ids_vec.push_back(pair.first);
    }

    std::set<int> matched_detection_indices; // 记录被匹配上的新检测
    std::set<int> matched_track_unique_ids; // [关键修正] 记录被匹配上的旧目标的 unique_id

    // 3. 执行匹配 (如果存在旧目标和新检测)
    if (!tracked_ids_vec.empty() && !current_detections.empty()) {
        std::vector<std::vector<double>> cost_matrix(tracked_ids_vec.size(), std::vector<double>(current_detections.size(), 1e6));
        for (size_t i = 0; i < tracked_ids_vec.size(); ++i) {
            const auto& obj = tracked_objects.at(tracked_ids_vec[i]);
            cv::Point2f predicted_pos = obj.centroid + obj.velocity;
            for (size_t j = 0; j < current_detections.size(); ++j) {
                double dist = cv::norm(predicted_pos - current_detections[j].centroid);
                if (dist < Config::MAX_DISTANCE_FOR_TRACKING) { cost_matrix[i][j] = dist; }
            }
        }

        HungarianAlgorithm hungarian_solver;
        std::vector<int> assignment;
        hungarian_solver.Solve(cost_matrix, assignment);

        // 4. 更新匹配上的目标
        for (size_t i = 0; i < assignment.size(); ++i) {
            if (assignment[i] != -1 && cost_matrix[i][assignment[i]] < Config::MAX_DISTANCE_FOR_TRACKING) {
                int det_idx = assignment[i];
                int tid = tracked_ids_vec[i];
                TrackedObject& obj = tracked_objects.at(tid);
                const Detection& det = current_detections[det_idx];

                cv::Point2f new_velocity = det.centroid - obj.centroid;
                obj.velocity = obj.velocity * 0.5 + new_velocity * 0.5;
                obj.centroid = det.centroid;
                obj.missed_frames = 0; // 重置 missed_frames
                obj.current_label_id = det.label_id;
                obj.current_bbox = det.bbox;

                matched_detection_indices.insert(det_idx);
                matched_track_unique_ids.insert(tid); // [关键修正] 记录这个旧目标的ID是匹配上的
            }
        }
    }

    // 5. [修正后逻辑] 处理未匹配上的旧目标
    for (auto& pair : tracked_objects) {
        // 如果当前目标的ID不在“已匹配”的集合中，说明它没有匹配上
        if (matched_track_unique_ids.find(pair.first) == matched_track_unique_ids.end()) {
            pair.second.missed_frames++;
            // 预测一小步位置，使其看起来更平滑
            if (pair.second.missed_frames <= Config::MAX_MISSED_FRAMES) {
                pair.second.centroid += pair.second.velocity;
            }
            pair.second.current_label_id = -1; // 标记它在当前帧没有对应的label
        }
    }

    // 6. 处理新的目标 (未匹配上的新检测)
    for (size_t i = 0; i < current_detections.size(); ++i) {
        if (matched_detection_indices.find(i) == matched_detection_indices.end()) {
            const auto& det = current_detections[i];
            TrackedObject new_obj;
            new_obj.unique_id = m_next_unique_id++;
            new_obj.centroid = det.centroid;
            new_obj.velocity = {0, Config::INITIAL_VERTICAL_MOVEMENT};
            new_obj.color = m_colors[m_color_index++ % m_colors.size()];
            new_obj.current_label_id = det.label_id;
            new_obj.current_bbox = det.bbox;
            bool is_right = det.centroid.x > (Config::ROI.width / 2.0);
            if (is_right) { new_obj.assigned_number = m_next_right_column_number++; }
            else { new_obj.assigned_number = m_next_left_column_number++; }
            tracked_objects[new_obj.unique_id] = new_obj;
        }
    }

    // 7. 清理“死亡”的目标 (那些丢失了太久的目标)
    for (auto it = tracked_objects.begin(); it != tracked_objects.end(); ) {
        if (it->second.missed_frames > Config::MAX_MISSED_FRAMES) {
            it = tracked_objects.erase(it); // 正确的删除方式
        } else {
            ++it;
        }
    }
}