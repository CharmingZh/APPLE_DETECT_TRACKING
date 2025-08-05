#ifndef TRACKMANAGER_H
#define TRACKMANAGER_H

#include "utils/DataTypes.h"
#include <unordered_map>
#include <vector>
#include <chrono>

// PendingAction 结构体保持不变
struct PendingAction {
    char action_type; // 'A' 或 'B'
    std::chrono::steady_clock::time_point trigger_time;
};

class TrackManager {
public:
    TrackManager();
    void update(const ConsumerResult& result, std::unordered_map<int, TrackedObject>& tracked_objects);
    std::vector<PendingAction> getAndClearFiredActions();

private:
    // [核心修改] 更新成员变量以反映双通道逻辑
    int m_next_unique_id = 0;
    int m_next_number_A; // 通道A的下一个分配编号
    int m_next_number_B; // 通道B的下一个分配编号
    int m_color_index = 0;
    std::vector<cv::Scalar> m_colors;

    int m_exit_counter_A = 0;
    int m_exit_counter_B = 0;

    std::vector<PendingAction> m_pending_actions;
};

#endif //TRACKMANAGER_H
