#pragma once
#include "Task/AbstractTaskPlugin.h"
#include <array>

namespace asst
{
    class FightTimesTaskPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~FightTimesTaskPlugin() override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;
        void set_fight_times_temp(int times);
        // 战斗结束，设置成功战斗的次数
        void finish_fight(int times);

    protected:
        virtual bool _run() override;
        // 初始化，检测是否支持连续战斗
        bool init(const cv::Mat& img);
        // 设置战斗次数
        bool change_times(cv::Mat img);
        // 匹配蓝色加号，返回可战斗的次数
        std::optional<int> match_medicine_icon(const cv::Mat& img) const;

    private:
        std::optional<bool> m_is_valid = std::make_optional<bool>(); // 检测连续战斗是否可用
        int m_fight_times_temp = 0;                                  // 正在战斗中的次数
        int m_fight_times_finished = 0;                              // 当前已完成的次数
        int m_fight_times_max = -1;                                  // 最大可战斗的次数，默认为-1
        const std::array<std::string, 6> Fight_Times_Tasks = { "FightSeries-List-1", "FightSeries-List-2",
                                                               "FightSeries-List-3", "FightSeries-List-4",
                                                               "FightSeries-List-5", "FightSeries-List-6" };
    };
}
