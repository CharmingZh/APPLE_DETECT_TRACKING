#ifndef KINECT_MANAGER_H
#define KINECT_MANAGER_H

#include <opencv2/opencv.hpp>
#include <string>
#include <k4a/k4a.h>

class KinectManager {
public:
    KinectManager();
    ~KinectManager();
    bool isOpened() const;
    bool getNextFrame(cv::Mat& colorFrame);
private:
    k4a_device_t m_device = NULL;
    bool m_is_opened = false;
};
#endif