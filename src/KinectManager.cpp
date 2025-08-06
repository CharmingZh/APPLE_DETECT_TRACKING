#include "KinectManager.h"
#include <iostream>

KinectManager::KinectManager() {
    if (K4A_RESULT_SUCCEEDED != k4a_device_open(K4A_DEVICE_DEFAULT, &m_device)) {
        std::cerr << "[ERROR] Kinect: Failed to open device!" << std::endl;
        return;
    }
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
    config.color_resolution = K4A_COLOR_RESOLUTION_1080P;
    config.depth_mode = K4A_DEPTH_MODE_OFF;
    config.camera_fps = K4A_FRAMES_PER_SECOND_30;
    if (K4A_RESULT_SUCCEEDED != k4a_device_start_cameras(m_device, &config)) {
        std::cerr << "[ERROR] Kinect: Failed to start cameras!" << std::endl;
        k4a_device_close(m_device);
        m_device = NULL;
        return;
    }
    std::cout << "[INFO] Azure Kinect sensor initialized successfully." << std::endl;
    m_is_opened = true;
}

KinectManager::~KinectManager() {
    if (m_device != NULL) {
        k4a_device_stop_cameras(m_device);
        k4a_device_close(m_device);
    }
}

bool KinectManager::isOpened() const {
    return m_is_opened;
}

bool KinectManager::getNextFrame(cv::Mat& colorFrame) {
    if (!m_is_opened) return false;
    k4a_capture_t capture = NULL;
    k4a_wait_result_t get_capture_result = k4a_device_get_capture(m_device, &capture, 100);
    if (get_capture_result == K4A_WAIT_RESULT_SUCCEEDED) {
        k4a_image_t color_image = k4a_capture_get_color_image(capture);
        if (color_image != NULL) {
            uint8_t* buffer = k4a_image_get_buffer(color_image);
            int width = k4a_image_get_width_pixels(color_image);
            int height = k4a_image_get_height_pixels(color_image);
            cv::Mat bgraImage(height, width, CV_8UC4, buffer);
            cv::cvtColor(bgraImage, colorFrame, cv::COLOR_BGRA2BGR);
            k4a_image_release(color_image);
            k4a_capture_release(capture);
            return true;
        }
        k4a_capture_release(capture);
    } else if (get_capture_result != K4A_WAIT_RESULT_TIMEOUT) {
        std::cerr << "[ERROR] Kinect: Failed to get capture from device!" << std::endl;
        m_is_opened = false;
    }
    return false;
}