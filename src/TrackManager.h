#ifndef TRACKMANAGER_H
#define TRACKMANAGER_H

#include "utils/DataTypes.h"
#include <unordered_map>

class TrackManager {
public:
    TrackManager();
    void update(const ConsumerResult& result, std::unordered_map<int, TrackedObject>& tracked_objects);

private:
    int m_next_unique_id = 0;
    int m_next_left_column_number = 1;
    int m_next_right_column_number = 10;

    int m_color_index = 0;
    std::vector<cv::Scalar> m_colors;

    const float m_max_dist_for_tracking = 100.0f;
    const int m_max_missed_frames = 5;
    const int m_min_area_threshold = 2750;
    const float m_initial_vertical_movement = -20.0f;
};

#endif //TRACKMANAGER_H