#pragma once
/*
#include "MinicapBase.h"

#include "Utils/NoWarningCV.h"
#include <condition_variable>
#include <meojson/json.hpp>
#include <thread>

namespace asst
{
    class MinicapStream : public MinicapBase
    {
    public:
        using MinicapBase::MinicapBase;

        virtual ~MinicapStream() override;

    public: // from UnitBase
        virtual bool parse(const json::value& config) override;

    public: // from ScreencapAPI
        virtual bool init(int swidth, int sheight) override;

        virtual std::optional<cv::Mat> screencap() override;

    private:
        bool read_until(std::string& buffer, size_t size);
        bool take_out(void* out, size_t size);
        void working_thread();

        std::optional<cv::Mat> decode_jpg(const std::string& buffer);
        bool check_head_tail(std::string_view input, std::string_view head, std::string_view tail);
        std::optional<cv::Mat> decode(const std::string& buffer);

        Argv forward_argv_;

        bool quit_ = true;
        std::mutex mutex_;
        cv::Mat image_;
        std::condition_variable cond_;
        std::thread pull_thread_;

        std::shared_ptr<IOHandler> process_handle_;
        std::shared_ptr<IOHandler> stream_handle_;
    };

}; // namespace asst
*/
