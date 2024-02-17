#include "MinicapDirect.h"
/*
#include "Utils/Logger.hpp"

std::optional<cv::Mat> asst::MinicapDirect::screencap()
{
    int width = screencap_helper_.get_w();
    int height = screencap_helper_.get_h();

    auto res = binary_->invoke_bin_stdout("-P " + std::to_string(width) + "x" + std::to_string(height) + "@" +
                                          std::to_string(width) + "x" + std::to_string(height) + "/0 -s");

    if (!res) {
        return std::nullopt;
    }

    return screencap_helper_.process_data(
        res.value(), std::bind(&ScreencapHelper::trunc_decode_jpg, &screencap_helper_, std::placeholders::_1));
}
*/
