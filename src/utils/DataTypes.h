#ifndef DATATYPES_H
#define DATATYPES_H

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

struct ProducerTask {
    int frame_idx;
    cv::Mat image;
};

struct ConsumerResult {
    int frame_idx;
    cv::Mat original_image;
    cv::Mat labels;
    cv::Mat stats;
    cv::Mat centroids;
};

struct Detection {
    int label_id;
    cv::Point2f centroid;
    cv::Rect bbox;
};

struct TrackedObject {
    int unique_id;
    int assigned_number;
    int missed_frames = 0;
    cv::Point2f centroid;
    cv::Point2f velocity;
    cv::Scalar color;
    int current_label_id = -1;
    cv::Rect current_bbox;
};

struct TrackingStats {
    int frame;
    int assigned_number;
    int unique_id;
    float centroid_x;
    float centroid_y;
};

#endif //DATATYPES_H