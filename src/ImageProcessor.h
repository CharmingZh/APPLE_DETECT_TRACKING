#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include "utils/DataTypes.h"

namespace ImageProcessor {
    ConsumerResult process_frame(const ProducerTask& task, const cv::Rect& roi);
}

#endif //IMAGEPROCESSOR_H