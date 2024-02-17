#include "MinicapBase.h"
/*
#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include <array>
#include <ranges>

bool asst::MinicapBase::parse(const json::value& config)
{
    static const json::array kDefaultArch = {
        "x86",
        "armeabi-v7a",
        "armeabi",
    };
    json::array jarch = config.get("prebuilt", "minicap", "arch", kDefaultArch);

    if (ranges::any_of(jarch, [](const json::value& val) { return !val.is_string(); })) {
        return false;
    }
    arch_list_ = jarch.to_vector<std::string>();

    static const json::array kDefaultSdk = {
        31, 29, 28, 27, 26, 25, 24, 23, 22, 21, 19, 18, 17, 16, 15, 14,
    };
    json::array jsdk = config.get("prebuilt", "minicap", "sdk", kDefaultSdk);
    sdk_list_ = jsdk.to_vector<int>();

    return binary_->parse(config) && library_->parse(config);
}

// x86_64的prebuilt里面的library是32位的, 用不了
// arm64-v8会卡住, 不知道原因
bool asst::MinicapBase::init(int swidth, int sheight)
{
    LogTraceFunction;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return false;
    }

    if (!binary_->init() || !library_->init("minicap.so")) {
        return false;
    }

    auto archs = binary_->abilist();
    auto sdk = binary_->sdk();

    if (!archs || !sdk) {
        return false;
    }

    auto arch_iter = ranges::find_first_of(*archs, arch_list_);
    if (arch_iter == archs->end()) {
        return false;
    }
    const std::string& target_arch = *arch_iter;

    auto sdk_iter = ranges::find_if(sdk_list_, [sdk](int s) { return s <= sdk.value(); });
    if (sdk_iter == sdk_list_.end()) {
        return false;
    }
    int fit_sdk = *sdk_iter;

    // TODO: 确认低版本是否使用minicap-nopie
    const auto bin_path = agent_path_ / utils::path(target_arch) / utils::path("bin") / utils::path("minicap");
    const auto lib_path = agent_path_ / utils::path(target_arch) / utils::path("lib") /
                          utils::path("android-" + std::to_string(fit_sdk)) / utils::path("minicap.so");
    if (!binary_->push(bin_path) || !library_->push(lib_path)) {
        return false;
    }

    if (!binary_->chmod() || !library_->chmod()) {
        return false;
    }

    return set_wh(swidth, sheight);
}
*/
