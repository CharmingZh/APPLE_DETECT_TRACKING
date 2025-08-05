#include "ImageProcessor.h"
#include "config/Configuration.h"

namespace ImageProcessor {
    ConsumerResult process_frame(const ProducerTask& task) {
        cv::Mat roi_bgr_img = task.image(Config::ROI);
        cv::Mat roi_hsv_img;
        cv::cvtColor(roi_bgr_img, roi_hsv_img, cv::COLOR_BGR2HSV);
        cv::Mat hsv_mask;
        cv::inRange(roi_hsv_img, Config::LOWER_HSV, Config::UPPER_HSV, hsv_mask);
        cv::Mat morph_kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(Config::MORPH_KERNEL_SIZE, Config::MORPH_KERNEL_SIZE));
        cv::Mat denoised_mask;
        cv::morphologyEx(hsv_mask, denoised_mask, cv::MORPH_OPEN, morph_kernel, cv::Point(-1,-1), Config::MORPH_ITERATIONS);
        cv::Mat labels, stats, centroids;
        cv::connectedComponentsWithStats(denoised_mask, labels, stats, centroids, 8, CV_32S);
        return {task.frame_idx, task.image, labels, stats, centroids};
    }
}