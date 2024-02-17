#pragma once

#include <cstdint>

#pragma pack(push, 1)
namespace asst
{
    struct MinicapHeader
    {
        enum
        {
            DUMB = 1 << 0,
            ALWAYS_UPRIGHT = 1 << 1,
            TEAR = 1 << 2
        };

        uint8_t version = 0;
        uint8_t size = 0;
        uint32_t pid = 0;
        uint32_t real_width = 0;
        uint32_t real_height = 0;
        uint32_t virt_width = 0;
        uint32_t virt_height = 0;
        uint8_t orientation = 0;
        uint8_t flags = 0;
    };
}; // namespace asst
#pragma pack(pop)
