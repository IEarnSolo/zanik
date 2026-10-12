#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace zanik::qol {

enum class CompostType : std::uint8_t { none, regular, super };

struct Bucket {
    std::uint64_t instance_id = 0;
    CompostType type = CompostType::none;
    std::uint32_t charges = 0;
};

// Inventory is supplied in game slot order. Returns inventory.size() if there
// is no eligible replacement. The emptied item is identified by instance ID,
// not by its (possibly changed) item data after consumption.
std::size_t next_compost_bucket(const Bucket& emptied,
                                const std::vector<Bucket>& inventory);

} // namespace zanik::qol
