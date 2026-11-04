#pragma once

#include <cstddef>
#include <cstdint>

namespace Core::Checksum
{
    uint32_t Update(uint32_t checksum, const void* data, size_t size);
}
