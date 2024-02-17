#include "MiniController.h"

#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include <numeric>

bool asst::MiniController::connect(const std::string& adb_path, const std::string& address, const std::string& config)
{
    bool res = MinitouchController::connect(adb_path, address, config);
    if (!res) return false;

    auto get_info_json = [&]() -> json::value {
        return json::object {
            { "uuid", m_uuid },
            { "details",
              json::object {
                  { "adb", adb_path },
                  { "address", address },
                  { "config", config },
              } },
        };
    };

    std::string display_id;
    std::string nc_address = "10.0.2.2";
    uint16_t nc_port = 0;

    auto cmd_replace = [&](const std::string& cfg_cmd) -> std::string {
        return utils::string_replace_all(cfg_cmd, {
                                                      { "[Adb]", adb_path },
                                                      { "[AdbSerial]", address },
                                                      { "[DisplayId]", display_id },
                                                      { "[NcPort]", std::to_string(nc_port) },
                                                      { "[NcAddress]", nc_address },
                                                  });
    };

    auto adb_ret = Config.get_adb_cfg(config);

    if (!adb_ret) {
        return false;
    }

    const auto& adb_cfg = adb_ret.value();

    bool m_minicap_available = probe_minicap(adb_cfg, cmd_replace);

    if (!m_minicap_available) {
        json::value info = get_info_json() | json::object {
            { "what", "MinicapNotAvailable" },
            { "why", "" },
        };
        callback(AsstMsg::ConnectionInfo, info);
        return false;
    }

    return true;
}

bool asst::MiniController::screencap(cv::Mat& image_payload, bool allow_reconnect)
{
    using namespace std::chrono;
    auto start_time = steady_clock::now();
    const auto once_command = m_adb.call_minicap + "-P " + std::to_string(m_width) + "x" + std::to_string(m_height) +
                              "@" + std::to_string(m_width) + "x" + std::to_string(m_height) + "/0 -s";

    bool is_success = false;
    do {
        auto res = call_command(once_command, 2000, allow_reconnect);
        if (!res || res->empty()) break;

        auto decode_res = process_data(res.value(), trunc_decode_jpg);
        if (!decode_res) return false;

        image_payload = decode_res.value();
        is_success = true;
    } while (false);
    m_last_command_duration = duration_cast<milliseconds>(steady_clock::now() - start_time).count();

    // 记录截图耗时，每10次截图回传一次最值+平均值
    m_screencap_duration.emplace_back(is_success ? m_last_command_duration : LLONG_MAX); // 记录截图耗时
    ++m_screencap_time;

    if (m_screencap_duration.size() > 30) {
        m_screencap_duration.pop_front();
    }
    if (m_screencap_time > 9) { // 每 10 次截图计算一次平均耗时
        m_screencap_time = 0;
        auto filtered_duration = m_screencap_duration | views::filter([](long long num) { return num < LLONG_MAX; });
        // 过滤后的有效截图用时次数
        auto filtered_count = m_screencap_duration.size() - ranges::count(m_screencap_duration, LLONG_MAX);
        auto [screencap_cost_min, screencap_cost_max] = ranges::minmax(filtered_duration);
        json::value info = json::object {
            { "uuid", m_uuid },
            { "what", "ScreencapCost" },
            { "details",
              json::object {
                  { "min", screencap_cost_min },
                  { "max", screencap_cost_max },
                  { "avg",
                    filtered_count > 0
                        ? std::accumulate(filtered_duration.begin(), filtered_duration.end(), 0ll) / filtered_count
                        : -1 },
              } },
        };
        if (m_screencap_duration.size() - filtered_count > 0) {
            info["details"]["fault_times"] = m_screencap_duration.size() - filtered_count;
        }
        callback(AsstMsg::ConnectionInfo, info);
    }
    return is_success;
}

bool asst::MiniController::probe_minicap(const AdbCfg& adb_cfg,
                                         std::function<std::string(const std::string&)> cmd_replace)
{
    using namespace asst::utils::path_literals;
    LogTraceFunction;

    static const std::vector<std::string> kDefaultArch = {
        "x86",
        "armeabi-v7a",
        "armeabi",
    };
    static const std::vector<int> kDefaultSdk = {
        31, 29, 28, 27, 26, 25, 24, 23, 22, 21, 19, 18, 17, 16, 15, 14,
    };

    std::string_view arch;
    int sdk = 0;
    std::string abilist = call_command(cmd_replace(adb_cfg.abilist)).value_or(std::string());
    for (const auto& abi : kDefaultArch) {
        if (abilist.find(abi) != std::string::npos) {
            arch = abi;
            break;
        }
    }
    std::string sdk_str = call_command(cmd_replace(adb_cfg.sdk_version)).value_or("0");
    if (!utils::chars_to_number(sdk_str, sdk)) {
        LogError << "Failed to convert sdk version: " << sdk_str;
        return false;
    }
    if (ranges::count(kDefaultSdk, sdk) == 0) {
        sdk = 31;
        //return false;
    }
    Log.info("arch", arch, "sdk", sdk);

    if (arch.empty()) return false;

    auto minicap_cmd_rep = [&](const std::string& cfg_cmd, std::filesystem::path arg = {},
                               std::string file_name = "") -> std::string {
        return utils::string_replace_all(
            cmd_replace(cfg_cmd),
            {
                { "[minicapLocalPath]", utils::path_to_utf8_string(ResDir.get() / "minicap"_p / arch / arg) },
                { "[minicapFileName]", file_name },
            });
    };
    const auto bin_path = "bin"_p / "minicap"_p;
    const auto lib_path = "lib"_p / ("android-" + std::to_string(sdk)) / "minicap.so"_p;
    if (!call_command(minicap_cmd_rep(adb_cfg.push_minicap, bin_path, "minicap"))) return false;
    if (!call_command(minicap_cmd_rep(adb_cfg.push_minicap, lib_path, "minicap.so"))) return false;
    if (!call_command(minicap_cmd_rep(adb_cfg.chmod_minicap))) return false;

    // if (!call_and_hup_minitouch()) return false;

    return true;
}

std::optional<cv::Mat> asst::MiniController::process_data(
    std::string& buffer, std::function<std::optional<cv::Mat>(const std::string& buffer)> decoder)
{
    bool tried_clean = false;

#ifdef _WIN32
    if (m_adb.screencap_end_of_line == AdbProperty::ScreencapEndOfLine::UnknownYet) {
        auto saved = buffer;
        if (clean_cr(buffer)) {
            auto res = decoder(buffer);
            if (res) {
                LogInfo << "end_of_line is CRLF";
                m_adb.screencap_end_of_line = AdbProperty::ScreencapEndOfLine::CRLF;
                return res;
            }
            else {
                saved.swap(buffer);
            }
        }
        tried_clean = true;
    }
#endif

    if (m_adb.screencap_end_of_line == AdbProperty::ScreencapEndOfLine::CRLF) {
        tried_clean = true;
        if (!clean_cr(buffer)) {
            LogInfo << "end_of_line is set to CRLF but no `\\r\\n` found, set it to LF";
            m_adb.screencap_end_of_line = AdbProperty::ScreencapEndOfLine::LF;
        }
    }

    auto res = decoder(buffer);

    if (res) {
        if (m_adb.screencap_end_of_line == AdbProperty::ScreencapEndOfLine::UnknownYet) {
            LogInfo << "end_of_line is LF";
            m_adb.screencap_end_of_line = AdbProperty::ScreencapEndOfLine::LF;
        }
        return res;
    }

    LogInfo << "data is not empty, but image is empty";
    if (tried_clean) {
        LogError << "skip retry decoding and decode failed!";
        return std::nullopt;
    }

    LogInfo << "try to cvt lf";
    if (!clean_cr(buffer)) {
        LogError << "no `\\r\\n` found, skip retry decode";
        return std::nullopt;
    }

    res = decoder(buffer);

    if (!res) {
        LogError << "convert lf and retry decode failed!";
        return std::nullopt;
    }

    if (m_adb.screencap_end_of_line == AdbProperty::ScreencapEndOfLine::UnknownYet) {
        LogInfo << "end_of_line is CRLF";
    }
    else {
        LogInfo << "end_of_line is changed to CRLF";
    }
    m_adb.screencap_end_of_line = AdbProperty::ScreencapEndOfLine::CRLF;

    return res;
}

bool asst::MiniController::clean_cr(std::string& buffer)
{
    if (buffer.size() < 2) {
        return false;
    }

    auto check = [](std::string::iterator it) { return *it == '\r' && *(it + 1) == '\n'; };

    auto scan = buffer.end();
    for (auto it = buffer.begin(); it != buffer.end() - 1; ++it) {
        if (check(it)) {
            scan = it;
            break;
        }
    }
    if (scan == buffer.end()) {
        return false;
    }

    auto last = buffer.end() - 1;
    auto ptr = scan;
    while (++scan != last) {
        if (!check(scan)) {
            *ptr = *scan;
            ++ptr;
        }
    }
    *ptr = *last;
    ++ptr;
    buffer.erase(ptr, buffer.end());
    return true;
}

std::optional<cv::Mat> asst::MiniController::trunc_decode_jpg(const std::string& buffer)
{
    auto pos = buffer.find("\xFF\xD8\xFF");
    auto truncbuf = buffer.substr(pos);
    if (!check_head_tail(truncbuf, "\xFF\xD8\xFF", "\xFF\xD9")) {
        return std::nullopt;
    }
    return decode(truncbuf);
}

std::optional<cv::Mat> asst::MiniController::decode_jpg(const std::string& buffer)
{
    if (!check_head_tail(buffer, "\xFF\xD8\xFF", "\xFF\xD9")) {
        return std::nullopt;
    }
    return decode(buffer);
}

bool asst::MiniController::check_head_tail(std::string_view input, std::string_view head, std::string_view tail)
{
    if (input.size() < head.size() || input.size() < tail.size()) {
        Log.error("input too short", (input), (head), (tail));
        return false;
    }

    if (input.substr(0, head.size()) != head || input.substr(input.size() - tail.size(), tail.size()) != tail) {
        LogError << "head or tail mismatch" << (input) << (head) << (tail);
        return false;
    }

    return true;
}

std::optional<cv::Mat> asst::MiniController::decode(const std::string& buffer)
{
    cv::Mat img = cv::imdecode({ buffer.data(), int(buffer.size()) }, cv::IMREAD_COLOR);
    return img.empty() ? std::nullopt : std::make_optional(img);
}
