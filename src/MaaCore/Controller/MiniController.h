#pragma once
#include "MinitouchController.h"

namespace asst
{
    class MiniController : public MinitouchController
    {
    public:
        using MinitouchController::MinitouchController;

        virtual bool connect(const std::string& adb_path, const std::string& address,
                             const std::string& config) override;
        virtual bool screencap(cv::Mat& image_payload, bool allow_reconnect = false) override;
        bool probe_minicap(const AdbCfg& adb_cfg, std::function<std::string(const std::string&)> cmd_replace);

        std::optional<cv::Mat> process_data(std::string& buffer,
                                            std::function<std::optional<cv::Mat>(const std::string& buffer)> decoder);

        bool clean_cr(std::string& buffer);

        static std::optional<cv::Mat> trunc_decode_jpg(const std::string& buffer);
        static std::optional<cv::Mat> decode_jpg(const std::string& buffer);
        static bool check_head_tail(std::string_view input, std::string_view head, std::string_view tail);
        static std::optional<cv::Mat> decode(const std::string& buffer);

        
    protected:
        virtual std::optional<std::string> reconnect(const std::string& cmd, int64_t timeout,
                                                     bool recv_by_socket) override;

        private:
        bool minicap_test();
    };
}; // namespace asst
