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
    int m_next_left_column_number;
    int m_next_right_column_number;
    int m_color_index = 0;
    std::vector<cv::Scalar> m_colors;
};
#endif //TRACKMANAGER_H