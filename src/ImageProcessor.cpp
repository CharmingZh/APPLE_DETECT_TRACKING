#include "ImageProcessor.h"
#include "config/Configuration.h"

// namespace ImageProcessor {
//      // CPU 版本
//     // 辅助函数，用于处理单个ROI区域
//     void process_single_roi(const cv::Mat& full_image, const cv::Rect& roi, cv::Mat& output_mask) {
//         // 1. 从完整图像中提取ROI
//         cv::Mat roi_bgr_img = full_image(roi);
//
//         // 2. 转换为HSV色彩空间
//         cv::Mat roi_hsv_img;
//         cv::cvtColor(roi_bgr_img, roi_hsv_img, cv::COLOR_BGR2HSV);
//
//         // 3. 根据HSV阈值创建掩码
//         cv::Mat hsv_mask;
//         cv::inRange(roi_hsv_img, Config::LOWER_HSV, Config::UPPER_HSV, hsv_mask);
//
//         // 4. 使用形态学操作去噪
//         cv::Mat morph_kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(Config::MORPH_KERNEL_SIZE, Config::MORPH_KERNEL_SIZE));
//         cv::morphologyEx(hsv_mask, output_mask, cv::MORPH_OPEN, morph_kernel, cv::Point(-1,-1), Config::MORPH_ITERATIONS);
//     }
//
//     ConsumerResult process_frame(const ProducerTask& task) {
//         // [核心修改] 分别处理两个ROI
//         cv::Mat mask_A, mask_B;
//         process_single_roi(task.image, Config::ROI_A, mask_A);
//         process_single_roi(task.image, Config::ROI_B, mask_B);
//
//         // 创建一个与完整图像同样大小的空白掩码，用于合并结果
//         cv::Mat combined_mask = cv::Mat::zeros(task.image.size(), CV_8UC1);
//
//         // 将两个ROI的处理结果复制到合并掩码的正确位置
//         mask_A.copyTo(combined_mask(Config::ROI_A));
//         mask_B.copyTo(combined_mask(Config::ROI_B));
//
//         // 在合并后的完整掩码上执行连通组件分析
//         // 这样做的好处是，得到的坐标(stats, centroids)都是相对于完整大图的，方便后续处理
//         cv::Mat labels, stats, centroids;
//         cv::connectedComponentsWithStats(combined_mask, labels, stats, centroids, 8, CV_32S);
//
//         // 返回包含完整坐标信息的结果
//         return {task.frame_idx, task.image, labels, stats, centroids};
//     }
// }


namespace ImageProcessor {

    // 辅助函数，用于处理单个ROI区域
    // 只需要将 cv::Mat 替换为 cv::UMat
    // OpenCV 会自动处理后台的 GPU 计算
    void process_single_roi(const cv::UMat& full_image, const cv::Rect& roi, cv::UMat& output_mask) {
        // 1. 从完整图像中提取ROI
        cv::UMat roi_bgr_img = full_image(roi);

        // 2. 转换为HSV色彩空间
        cv::UMat roi_hsv_img;
        cv::cvtColor(roi_bgr_img, roi_hsv_img, cv::COLOR_BGR2HSV);

        // 3. 根据HSV阈值创建掩码
        cv::UMat hsv_mask;
        cv::inRange(roi_hsv_img, Config::LOWER_HSV, Config::UPPER_HSV, hsv_mask);

        // 4. 使用形态学操作去噪
        cv::Mat morph_kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(Config::MORPH_KERNEL_SIZE, Config::MORPH_KERNEL_SIZE));
        cv::morphologyEx(hsv_mask, output_mask, cv::MORPH_OPEN, morph_kernel, cv::Point(-1,-1), Config::MORPH_ITERATIONS);
    }

    // 在 process_frame 中也同样使用 UMat
    ConsumerResult process_frame(const ProducerTask& task) {
        // 将 task.image (cv::Mat) 转换为 cv::UMat
        cv::UMat u_image = task.image.getUMat(cv::ACCESS_READ);

        cv::UMat mask_A, mask_B;
        process_single_roi(u_image, Config::ROI_A, mask_A);
        process_single_roi(u_image, Config::ROI_B, mask_B);

        cv::UMat combined_mask = cv::UMat::zeros(task.image.size(), CV_8UC1);

        mask_A.copyTo(combined_mask(Config::ROI_A));
        mask_B.copyTo(combined_mask(Config::ROI_B));

        // 注意：connectedComponentsWithStats 目前在很多后端上不支持 UMat
        // 所以在这一步需要从 GPU 下载回 CPU
        cv::Mat final_mask = combined_mask.getMat(cv::ACCESS_READ);
        cv::Mat labels, stats, centroids;
        cv::connectedComponentsWithStats(final_mask, labels, stats, centroids, 8, CV_32S);

        return {task.frame_idx, task.image, labels, stats, centroids};
    }
}
