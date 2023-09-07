#include "InfrastControlTask.h"

#include "Utils/Logger.hpp"

bool asst::InfrastControlTask::_run()
{
    m_all_available_opers.clear();

    // 控制中枢只能造这一个
    set_product("MoodAddition");
    if (m_is_custom && current_room_config().skip) {
        Log.info("skip this room");
        return true;
    }
    if (!enter_facility()) {
        return false;
    }
    if (!enter_oper_list_page()) {
        return false;
    }

    // 如果是使用了编队组来排班
    if (current_room_config().use_operator_groups) {
        current_room_config().names.clear();
        int swipe_times = 0;

        std::set<std::string> oper_list;
        std::vector<std::string> temp, pre_temp;
        while (true) {
            if (need_exit()) {
                return false;
            }
            temp.clear();
            if (!get_opers(temp, m_mood_threshold)) {
                return false;
            }
            if (pre_temp == temp) {
                break;
            }
            oper_list.insert(temp.begin(), temp.end());
            pre_temp = temp;
            swipe_of_operlist();
            swipe_times++;
        }
        swipe_to_the_left_of_operlist(swipe_times + 1);
        swipe_times = 0;
        // 筛选第一个满足要求的干员组
        for (auto it = current_room_config().operator_groups.begin(); it != current_room_config().operator_groups.end();
             it++) {
            if (ranges::all_of(it->second, [oper_list](std::string& oper) { return oper_list.contains(oper); })) {
                current_room_config().names.insert(current_room_config().names.end(), it->second.begin(),
                                                   it->second.end());

                json::value sanity_info = basic_info_with_what("CustomInfrastRoomGroupsMatch");
                sanity_info["details"]["group"] = it->first;
                callback(AsstMsg::SubTaskExtraInfo, sanity_info);
                break;
            }
        }
    }

    for (int i = 0; i <= OperSelectRetryTimes; ++i) {
        if (need_exit()) {
            return false;
        }
        if (is_use_custom_opers()) {
            bool name_select_ret = swipe_and_select_custom_opers();
            if (name_select_ret) {
                break;
            }
            else {
                swipe_to_the_left_of_operlist();
                continue;
            }
        }

        click_clear_button();

        if (!opers_detect_with_swipe()) {
            return false;
        }
        swipe_to_the_left_of_operlist();

        optimal_calc();
        if (!opers_choose()) {
            m_all_available_opers.clear();
            swipe_to_the_left_of_operlist();
            continue;
        }
        break;
    }
    click_confirm_button();
    click_return_button();

    return true;
}
