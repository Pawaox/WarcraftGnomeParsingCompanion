#include "Core/Checksum.h"

#include <array>

namespace Core::Checksum
{
    namespace
    {
        constexpr uint32_t POLYNOMIAL = 0xEDB88320;

        constexpr std::array<uint32_t, 256> BuildTable()
        {
            std::array<uint32_t, 256> table{};
            for (uint32_t index = 0; index < table.size(); ++index)
            {
                uint32_t value = index;
                for (int bit = 0; bit < 8; ++bit)
                    value = (value & 1) ? (value >> 1) ^ POLYNOMIAL : value >> 1;
                table[index] = value;
            }
            return table;
        }

        constexpr std::array<uint32_t, 256> TABLE = BuildTable();
    }

    uint32_t Update(uint32_t checksum, const void* data, size_t size)
    {
        uint32_t value = ~checksum;
        const unsigned char* bytes = static_cast<const unsigned char*>(data);
        for (size_t index = 0; index < size; ++index)
            value = TABLE[(value ^ bytes[index]) & 0xFF] ^ (value >> 8);
        return ~value;
    }
}
