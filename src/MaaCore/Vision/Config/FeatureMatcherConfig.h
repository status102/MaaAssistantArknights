#pragma once
#include "Common/AsstTypes.h"
#include "Utils/NoWarningCVMat.h"
#include <variant>

namespace asst
{
    class FeatureMatcherConfig
    {
    public:
        enum class Detector
        {
            SIFT, // 计算复杂度高，具有尺度不变性、旋转不变性。效果最好。
            SURF,
            ORB,   // 计算速度非常快，具有旋转不变性。但不具有尺度不变性。
            BRISK, // 计算速度非常快，具有尺度不变性、旋转不变性。
            KAZE,  // 适用于2D和3D图像，具有尺度不变性、旋转不变性。
            AKAZE, // 计算速度较快，具有尺度不变性、旋转不变性。
        };
        struct Params
        {

            // enum class Matcher
            //{
            //     FLANN,
            //     BRUTEFORCE,
            // };

            inline static constexpr Detector kDefaultDetector = Detector::ORB;
            // inline static constexpr Matcher kDefaultMatcher = Matcher::FLANN;
            inline static constexpr double kDefaultDistanceRatio = 0.6;
            inline static constexpr int kDefaultCount = 4;

            std::vector<cv::Rect> roi;
            std::vector<std::variant<std::string, cv::Mat>> templs;
            bool green_mask = false;

            Detector detector = kDefaultDetector;
            // Matcher matcher = kDefaultMatcher;

            double distance_ratio = kDefaultDistanceRatio;
            int count = kDefaultCount; // 匹配的特征点的数量要求（阈值），默认 4.
        };

    public:
        FeatureMatcherConfig() = default;
        virtual ~FeatureMatcherConfig() = default;

        void set_params(Params params) { m_params = std::move(params); }

        // void set_task_info(const std::shared_ptr<TaskInfo>& task_ptr);
        // void set_task_info(const std::string& task_name);

        void set_templ(std::variant<std::string, cv::Mat> templ) { m_params.templs = { std::move(templ) }; }
        // void set_templ(std::vector<std::variant<std::string, cv::Mat>> templs);
        // void set_threshold(double templ_thres) noexcept;
        // void set_threshold(std::vector<double> templ_thres) noexcept;

    protected:
        virtual void _set_roi(const Rect& roi) = 0;

        // void _set_task_info(MatchTaskInfo task_info);

    protected:
        Params m_params;
    };
}
