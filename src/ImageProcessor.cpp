#include "ImageProcessor.h"

namespace ImageProcessor {

    ConsumerResult process_frame(const ProducerTask& task, const cv::Rect& roi) {
        cv::Mat roi_bgr_img = task.image(roi);
        cv::Mat roi_hsv_img;
        cv::cvtColor(roi_bgr_img, roi_hsv_img, cv::COLOR_BGR2HSV);

        // 与您的 Python 脚本完全一致的 HSV 阈值
        cv::Scalar lower_hsv_thresh = {10, 40, 40};
        cv::Scalar upper_hsv_thresh = {40, 255, 255};

        cv::Mat hsv_mask;
        // (关键修正) 确保在 roi_hsv_img 上执行 inRange
        cv::inRange(roi_hsv_img, lower_hsv_thresh, upper_hsv_thresh, hsv_mask);

        cv::Mat morph_kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
        cv::Mat denoised_mask;
        // 迭代次数为 1，与 Python 一致
        cv::morphologyEx(hsv_mask, denoised_mask, cv::MORPH_OPEN, morph_kernel, cv::Point(-1,-1), 1);

        cv::Mat labels, stats, centroids;
        cv::connectedComponentsWithStats(denoised_mask, labels, stats, centroids, 8, CV_32S);

        // 返回结果，用于后续处理（我们暂时去掉调试图像的返回，以保持与Python逻辑一致）
        return {task.frame_idx, task.image, labels, stats, centroids};
    }

} // namespace ImageProcessor