#include "FightTimesTaskPlugin.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

bool asst::FightTimesTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const auto& task_name = details.at("details").at("task").as_string();
    return task_name.ends_with("StartButton1");
}

void asst::FightTimesTaskPlugin::set_fight_times_temp(int times)
{
    m_fight_times_temp = times;
}

void asst::FightTimesTaskPlugin::finish_fight(int times)
{
    if (times - m_fight_times_temp > 0) {
        Log.warn(__FUNCTION__, "战斗中次数录入失败");
    }

    Log.trace(__FUNCTION__, "战斗结束，成功:", times, "次");
    m_fight_times_finished += times;
    m_fight_times_temp = 0;
}

bool asst::FightTimesTaskPlugin::_run()
{
    if (m_fight_times_temp != 0) {
        Log.warn(__FUNCTION__, "上次战斗未能正常结束, 正在打第", m_fight_times_temp, "次");
        m_fight_times_finished += m_fight_times_temp - 1;
        m_fight_times_temp = 0;
    }

    // 执行次数达到上限，退出
    if (m_fight_times_max != -1 && m_fight_times_finished >= m_fight_times_max) {
        Log.info(__FUNCTION__, "fight times is enough. Return true.");
        m_task_ptr->set_enable(false);
        return true;
    }

    auto img = ctrler()->get_image();
    if (!m_is_valid) {
        m_is_valid = init(img);
    }

    if (!*m_is_valid) {
        Log.info(__FUNCTION__, "FightSeries is not supported.");
        return true;
    }

    if (!change_times(img)) {
        Log.error(__FUNCTION__, "unable to set fight times.");
        return false;
    }

    return true;
}

bool asst::FightTimesTaskPlugin::init(const cv::Mat& img)
{
    auto task = ProcessTask(*this, { "FightSeries-Indicator", "FightSeries-Icon" });
    task.set_reusable_image(img).set_retry_times(0);
    return task.run();
}

bool asst::FightTimesTaskPlugin::change_times(cv::Mat img)
{
    // 打开次数选择列表
    auto task = ProcessTask(*this, { "FightSeries-Indicator", "FightSeries-Open" });
    if (!task.set_reusable_image(img).run()) {
        Log.error(__FUNCTION__, "failed to open fight times list. Return false.");
        return false;
    }
    img = ctrler()->get_image();

    // 有药，且识别到吃药标志；返回值为[0, 5]
    if (auto result = match_medicine_icon(img); result) {
        auto times = std::min(*result, m_fight_times_max - m_fight_times_finished);
        Log.trace(__FUNCTION__, "there is enough sanity for", times, "times.");
        ProcessTask(*this, { Fight_Times_Tasks[std::max(1, times) - 1] }).run();
        // TODO: 添加callback
        return true;
    }

    // 判断是否有高亮的次数（理智足够）
    for (const auto& task_name : Fight_Times_Tasks | views::reverse) {
        RegionOCRer ocr(img);
        ocr.set_roi(Task.get(task_name)->specific_rect);
        ocr.set_bin_threshold(180);
        if (ocr.analyze()) {
            int times;
            if (!utils::chars_to_number(ocr.get_result().text, times)) {
                Log.error(__FUNCTION__, "failed to convert", ocr.get_result().text, "to number. Return false.");
                return false;
            }
            times = std::min(times, m_fight_times_max - m_fight_times_finished);
            Log.trace(__FUNCTION__, "there is enough sanity for", times, "times.");
            ProcessTask(*this, { Fight_Times_Tasks[times - 1] }).run();
            // TODO: 添加callback
            return true;
        }
    }

    // 选1去吃药
    Log.trace(__FUNCTION__, "there is not enough sanity for any fight and no medicine.");
    ProcessTask(*this, { "FightSeries-List-1" }).run();

    return true;
}

std::optional<int> asst::FightTimesTaskPlugin::match_medicine_icon(const cv::Mat& img) const
{
    MultiMatcher matcher(img);
    matcher.set_task_info("FightSeries-MedicineIcon");
    if (matcher.analyze()) {
        auto result = matcher.get_result();
        sort_by_vertical_(result);

        // 求最下方的加号对应的次数
        const auto& rect = result.rbegin()->rect;
        
        for (int index = 0; index < 6; index++) {
            if (Task.get(Fight_Times_Tasks[index])->specific_rect.include(rect)) {
                // 第几个rect里面有加号，对应次数-1就是可以打的次数
                return index;
            }
        }
    }

    return std::nullopt;
}
