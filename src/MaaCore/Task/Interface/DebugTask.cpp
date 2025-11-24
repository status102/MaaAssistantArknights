#include "DebugTask.h"

#include <filesystem>

#include "Common/AsstTypes.h"
#include "Config/TaskData.h"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Battle/BattlefieldClassifier.h"
#include "Vision/Battle/BattlefieldMatcher.h"
#include "Vision/Matcher.h"
#include "Vision/Miscellaneous/DepotImageAnalyzer.h"
#include "Vision/Miscellaneous/StageDropsImageAnalyzer.h"
#include "Vision/MultiMatcher.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

asst::DebugTask::DebugTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType)
{
}

bool asst::DebugTask::run()
{
    auto image_base = MaaNS::imread(
        utils::path("D:\\My_Program\\Arknights\\MaaAssistantArknights\\tools\\ImageRegistration") / "a.png");
    auto image_tilted = MaaNS::imread(
        utils::path("D:\\My_Program\\Arknights\\MaaAssistantArknights\\tools\\ImageRegistration") / "b.png");

    // 定义 ROI 区域
    cv::Rect roi(140, 80, 910, 500);
    // 裁剪并转换为灰度图
    cv::Mat image_base_gray, image_tilted_gray;
    cv::cvtColor(image_base(roi), image_base_gray, cv::COLOR_BGR2GRAY);
    cv::cvtColor(image_tilted(roi), image_tilted_gray, cv::COLOR_BGR2GRAY);

    std::vector<cv::KeyPoint> kp_base, kp_tilted;

    auto start_time = std::chrono::steady_clock::now();

    for (int i = 0; i < 50; i++) {
        // 创建 SIFT 特征检测器
        // auto detector = cv::SIFT::create();
        auto detector = cv::SIFT::create();

        // 检测特征点和描述符
        // std::vector<cv::KeyPoint> kp_base, kp_tilted;
        cv::Mat desc_base, desc_tilted;
        detector->detectAndCompute(image_base_gray, cv::noArray(), kp_base, desc_base);
        detector->detectAndCompute(image_tilted_gray, cv::noArray(), kp_tilted, desc_tilted);

        if (desc_base.empty() || desc_tilted.empty()) {
            Log.error(__FUNCTION__, "未检测到足够的特征点");
            return false;
        }

        // 特征匹配 (使用 BFMatcher 和 KNN)
        cv::BFMatcher matcher(cv::NORM_L2, false);
        std::vector<std::vector<cv::DMatch>> knn_matches;
        matcher.knnMatch(desc_base, desc_tilted, knn_matches, 2);

        // 应用比率测试筛选好的匹配
        std::vector<cv::DMatch> good_matches;
        for (const auto& match_pair : knn_matches) {
            if (match_pair.size() == 2) {
                if (match_pair[0].distance < 0.75 * match_pair[1].distance) {
                    good_matches.push_back(match_pair[0]);
                }
            }
        }

        if (good_matches.size() < 4) {
            Log.error(__FUNCTION__, "匹配点数量不足:", good_matches.size());
            return false;
        }

        // 提取匹配点的坐标并调整到原图坐标系
        std::vector<cv::Point2f> pts_base, pts_tilted;
        for (const auto& match : good_matches) {
            cv::Point2f pt_base = kp_base[match.queryIdx].pt;
            cv::Point2f pt_tilted = kp_tilted[match.trainIdx].pt;

            // 调整到原图坐标系
            pts_base.push_back(cv::Point2f(pt_base.x + roi.x, pt_base.y + roi.y));
            pts_tilted.push_back(cv::Point2f(pt_tilted.x + roi.x, pt_tilted.y + roi.y));
        }

        // 计算单应性矩阵 (从 base 到 tilted 的变换)
        cv::Mat mask;
        cv::Mat homography = cv::findHomography(pts_base, pts_tilted, cv::RANSAC, 5.0, mask);

        if (homography.empty()) {
            Log.error(__FUNCTION__, "无法计算变换矩阵");
            return false;
        }
        // 统计内点数量
        int inliers = cv::countNonZero(mask);
        Log.info(__FUNCTION__, "匹配成功:", good_matches.size(), "个匹配点,", inliers, "个内点");
        Log.info(__FUNCTION__, "变换矩阵:\n", homography);
    }

    auto costs =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
    LogInfo << __FUNCTION__ << costs << "平均每次配准耗时:" << costs / 50.0 << "ms";
    /*
    // 可视化匹配结果
    std::vector<cv::DMatch> inlier_matches;
    for (size_t i = 0; i < good_matches.size(); ++i) {
        if (mask.at<uchar>(static_cast<int>(i))) {
            inlier_matches.push_back(good_matches[i]);
        }
    }

    cv::Mat match_img;
    cv::drawMatches(
        image_base_gray,
        kp_base,
        image_tilted_gray,
        kp_tilted,
        inlier_matches,
        match_img,
        cv::Scalar(0, 255, 0),
        cv::Scalar(255, 0, 0),
        std::vector<char>(),
        cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

    // 添加文字信息
    cv::putText(
        match_img,
        "Total Matches: " + std::to_string(good_matches.size()),
        cv::Point(10, 30),
        cv::FONT_HERSHEY_SIMPLEX,
        0.8,
        cv::Scalar(255, 255, 255),
        2);
    cv::putText(
        match_img,
        "Inliers: " + std::to_string(inliers),
        cv::Point(10, 60),
        cv::FONT_HERSHEY_SIMPLEX,
        0.8,
        cv::Scalar(0, 255, 0),
        2);

    // 保存匹配可视化结果
    MaaNS::imwrite(
        utils::path("D:\\My_Program\\Arknights\\MaaAssistantArknights\\tools\\ImageRegistration") / "matches.png",
        match_img);

    // 应用变换矩阵进行配准
    cv::Mat warped_base, warped_tilted;
    cv::warpPerspective(image_base, warped_base, homography, image_tilted.size());
    cv::warpPerspective(image_tilted, warped_tilted, homography.inv(), image_base.size());

    // 保存配准结果
    MaaNS::imwrite(
        utils::path("D:\\My_Program\\Arknights\\MaaAssistantArknights\\tools\\ImageRegistration") /
            "warped_result_a.png",
        warped_base);
    MaaNS::imwrite(
        utils::path("D:\\My_Program\\Arknights\\MaaAssistantArknights\\tools\\ImageRegistration") /
            "warped_result_b.png",
        warped_tilted);
        */
    Log.info(__FUNCTION__, "配准完成，结果已保存");

    return true;
}

void asst::DebugTask::test_drops()
{
    size_t total = 0;
    size_t success = 0;
    for (const auto& entry : std::filesystem::directory_iterator("../../test/drops/screenshots/zh_cn")) {
        cv::Mat image = MAA_NS::imread(entry.path());
        if (image.empty()) {
            continue;
        }
        total += 1;
        cv::Mat resized;
        cv::resize(image, resized, cv::Size(1280, 720), 0, 0, cv::INTER_AREA);
        StageDropsImageAnalyzer analyzer(resized);
        success += analyzer.analyze();
    }
    Log.info(__FUNCTION__, success, "/", total);
}

void asst::DebugTask::test_skill_ready()
{
    int total = 0;
    int correct = 0;

    // 测试 y 类别（预期为 ready，即 true）
    for (const auto& entry : std::filesystem::directory_iterator(R"(../../test/skill_ready/y)")) {
        cv::Mat image = MAA_NS::imread(entry.path());
        BattlefieldClassifier analyzer(image);
        analyzer.set_object_of_interest({ .skill_ready = true });
        total++;
        auto result = analyzer.analyze()->skill_ready;
        // 记录日志：文件、预期结果、实际预测、得分、概率信息
        Log.info(
            __FUNCTION__,
            "File: ",
            entry.path().string(),
            " | Expected: Y (ready: true)",
            " | Predicted: ",
            result.ready,
            " | Score: ",
            result.score,
            " | Prob: ",
            result.prob);
        if (result.ready) {
            correct++;
        }
    }

    // 测试 n 类别（预期为 not ready，即 false）
    for (const auto& entry : std::filesystem::directory_iterator(R"(../../test/skill_ready/n)")) {
        cv::Mat image = MAA_NS::imread(entry.path());
        BattlefieldClassifier analyzer(image);
        analyzer.set_object_of_interest({ .skill_ready = true });
        total++;
        auto result = analyzer.analyze()->skill_ready;
        Log.info(
            __FUNCTION__,
            "File: ",
            entry.path().string(),
            " | Expected: N (ready: false)",
            " | Predicted: ",
            result.ready,
            " | Score: ",
            result.score,
            " | Prob: ",
            result.prob);
        if (!result.ready) {
            correct++;
        }
    }

    // 测试 c 类别（同样预期为 not ready）
    for (const auto& entry : std::filesystem::directory_iterator(R"(../../test/skill_ready/c)")) {
        cv::Mat image = MAA_NS::imread(entry.path());
        BattlefieldClassifier analyzer(image);
        analyzer.set_object_of_interest({ .skill_ready = true });
        total++;
        auto result = analyzer.analyze()->skill_ready;
        Log.info(
            __FUNCTION__,
            "File: ",
            entry.path().string(),
            " | Expected: C (ready: false)",
            " | Predicted: ",
            result.ready,
            " | Score: ",
            result.score,
            " | Prob: ",
            result.prob);
        if (!result.ready) {
            correct++;
        }
    }

    Log.info(__FUNCTION__, "Final Accuracy: ", correct, "/", total, " (", double(correct) / total, ")");
}

void asst::DebugTask::test_battle_image()
{
    cv::Mat image = MAA_NS::imread(utils::path("1.png"));
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(1280, 720), 0, 0, cv::INTER_AREA);
    BattlefieldMatcher analyzer(resized);
    analyzer.set_object_of_interest({ .deployment = true });
    analyzer.analyze();
}

void asst::DebugTask::test_match_template()
{
    auto test_task = [](const std::string& path, const std::string& task_name) -> double {
        cv::Mat image = MAA_NS::imread(utils::path(path));
        cv::Mat resized;
        cv::resize(image, resized, cv::Size(1280, 720), 0, 0, cv::INTER_AREA);
        Matcher match_analyzer(resized, Rect(0, 0, 1280, 720));
        const auto& task_ptr = Task.get(task_name);
        const auto match_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(task_ptr);
        match_analyzer.set_task_info(match_task_ptr);
        const auto& result_opt = match_analyzer.analyze();
        if (result_opt) {
            const auto& result = result_opt.value().to_string();
            Log.info("active", path, task_name, result);
            return result_opt.value().score;
        }
        else {
            Log.info("inactive", path, task_name);
            return 0.;
        }
    };

    // test_task(
    //     "../../x64/Release/debug/roguelike/2024-07-27_16-32-25-198_raw.png",
    //     "Sarkaz@Roguelike@StageWindAndRain");

    // for (int i = 1; i <= 15; ++i) {
    //     test_task("../../test/dist/" + std::to_string(i) + ".png", "Sarkaz@Roguelike@StageCombatDps");
    //     test_task("../../test/dist/" + std::to_string(i) + ".png", "Sarkaz@Roguelike@StageBoskyPassage");
    //     test_task("../../test/dist/" + std::to_string(i) + ".png", "Sarkaz@Roguelike@StageEmergencyTransportation");
    //     test_task("../../test/dist/" + std::to_string(i) + ".png", "Sarkaz@Roguelike@StageWindAndRain");
    // }

#define TEST(expr)                                       \
    if (!(expr)) {                                       \
        throw std::runtime_error("Test failed: " #expr); \
    }

#define ASSERT_ACTIVE(path, task_name) TEST(test_task(path, task_name) > DoubleDiff)
#define ASSERT_INACTIVE(path, task_name) TEST(test_task(path, task_name) < DoubleDiff)

    ASSERT_INACTIVE("../../test/dist/12.png", "Sarkaz@Roguelike@StageBoskyPassage");
    ASSERT_ACTIVE("../../test/dist/13.png", "Sarkaz@Roguelike@StageEmergencyTransportation");
    ASSERT_ACTIVE("../../test/dist/14.png", "Sarkaz@Roguelike@StageWindAndRain");
    ASSERT_ACTIVE("../../test/dist/15.png", "Sarkaz@Roguelike@StageEmergencyTransportation");
    ASSERT_ACTIVE("../../test/dist/#10160.png", "Sarkaz@Roguelike@StageTraderEnter");
    ASSERT_INACTIVE("../../test/dist/#10235.png", "Sarkaz@Roguelike@StageRefresh");

#undef TEST
#undef ASSERT_ACTIVE
#undef ASSERT_INACTIVE
}
