#include "PRTSTaskPlugin.h"

#include "Controller/Controller.h"
#include "Task/Fight/FightTimesTaskPlugin.h"
#include "Vision/RegionOCRer.h"

bool asst::PRTSTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const auto& task_name = details.at("details").at("task").as_string();
    return task_name.ends_with("PRTS1") || task_name.ends_with("PRTS2") || task_name.ends_with("PRTS3");
}

bool asst::PRTSTaskPlugin::_run()
{
    RegionOCRer ocr(ctrler()->get_image());
    ocr.set_task_info("FightSeries-Times");
    ocr.set_bin_threshold(200);
    if (!ocr.analyze()) {
        Log.warn(__FUNCTION__, "fight times OCR failed.");
        return true;
    }

    int times;

    if (!utils::chars_to_number(ocr.get_result().text, times)) {
        Log.warn(__FUNCTION__, "couldn't convert string to number. string:", ocr.get_result().text);
        return true;
    }

    for (const auto& plugin : m_task_ptr->get_plugins()) {
        if (auto ptr = std::dynamic_pointer_cast<FightTimesTaskPlugin>(plugin)) {
            ptr->finish_fight(times);
        }
    }

    return true;
}
