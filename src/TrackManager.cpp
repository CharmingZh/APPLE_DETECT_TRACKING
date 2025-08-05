#include "TrackManager.h"
#include "hungarian/Hungarian.h"
#include <set>

TrackManager::TrackManager() {
    m_colors = {
        {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0}, {0, 255, 255},
        {255, 0, 255}, {128, 0, 0}, {0, 128, 0}, {0, 0, 128}, {128, 128, 0},
        {0, 128, 128}, {128, 0, 128}
    };
}

void TrackManager::update(const ConsumerResult& result, std::unordered_map<int, TrackedObject>& tracked_objects) {
    std::vector<Detection> current_detections;
    for (int i = 1; i < result.stats.rows; ++i) {
        if (result.stats.at<int>(i, cv::CC_STAT_AREA) >= m_min_area_threshold) {
            Detection det;
            det.label_id = i;
            det.centroid = cv::Point2f(result.centroids.at<double>(i, 0), result.centroids.at<double>(i, 1));
            det.bbox = cv::Rect(
                result.stats.at<int>(i, cv::CC_STAT_LEFT),
                result.stats.at<int>(i, cv::CC_STAT_TOP),
                result.stats.at<int>(i, cv::CC_STAT_WIDTH),
                result.stats.at<int>(i, cv::CC_STAT_HEIGHT)
            );
            current_detections.push_back(det);
        }
    }

    std::vector<int> tracked_ids;
    for (const auto& pair : tracked_objects) tracked_ids.push_back(pair.first);

    std::set<int> matched_track_ids;
    std::set<int> matched_detection_indices;

    if (!tracked_ids.empty() && !current_detections.empty()) {
        std::vector<std::vector<double>> cost_matrix(tracked_ids.size(), std::vector<double>(current_detections.size(), 1e6));
        for (size_t i = 0; i < tracked_ids.size(); ++i) {
            cv::Point2f predicted_pos = tracked_objects[tracked_ids[i]].centroid + tracked_objects[tracked_ids[i]].velocity;
            for (size_t j = 0; j < current_detections.size(); ++j) {
                double dist = cv::norm(predicted_pos - current_detections[j].centroid);
                if (dist < m_max_dist_for_tracking) {
                    cost_matrix[i][j] = dist;
                }
            }
        }

        HungarianAlgorithm hungarian_solver;
        std::vector<int> assignment;
        hungarian_solver.Solve(cost_matrix, assignment);

        for (size_t i = 0; i < assignment.size(); ++i) {
            if (assignment[i] != -1 && cost_matrix[i][assignment[i]] < m_max_dist_for_tracking) {
                int det_idx = assignment[i];
                int tid = tracked_ids[i];

                TrackedObject& obj = tracked_objects[tid];
                const Detection& det = current_detections[det_idx];

                cv::Point2f new_velocity = det.centroid - obj.centroid;
                obj.velocity = obj.velocity * 0.5 + new_velocity * 0.5;

                obj.centroid = det.centroid;
                obj.missed_frames = 0;
                obj.current_label_id = det.label_id;
                obj.current_bbox = det.bbox;

                matched_track_ids.insert(tid);
                matched_detection_indices.insert(det_idx);
            }
        }
    }

    for (auto& pair : tracked_objects) {
        if (matched_track_ids.find(pair.first) == matched_track_ids.end()) {
            pair.second.missed_frames++;
            if (pair.second.missed_frames <= m_max_missed_frames) {
                pair.second.centroid += pair.second.velocity;
            }
            pair.second.current_label_id = -1;
        }
    }

    cv::Rect roi(820, 100, 1360 - 820, 1030 - 100);
    for (size_t i = 0; i < current_detections.size(); ++i) {
        if (matched_detection_indices.find(i) == matched_detection_indices.end()) {
            const auto& det = current_detections[i];
            TrackedObject new_obj;
            new_obj.unique_id = m_next_unique_id++;
            new_obj.centroid = det.centroid;
            new_obj.velocity = {0, m_initial_vertical_movement};
            new_obj.color = m_colors[m_color_index++ % m_colors.size()];
            new_obj.current_label_id = det.label_id;
            new_obj.current_bbox = det.bbox;

            bool is_right = det.centroid.x > (roi.width / 2.0);
            if (is_right) {
                new_obj.assigned_number = m_next_right_column_number++;
            } else {
                new_obj.assigned_number = m_next_left_column_number++;
            }

            tracked_objects[new_obj.unique_id] = new_obj;
        }
    }

    for (auto it = tracked_objects.begin(); it != tracked_objects.end(); ) {
        if (it->second.missed_frames > m_max_missed_frames) {
            it = tracked_objects.erase(it);
        } else {
            ++it;
        }
    }
}