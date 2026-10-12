#include "compost.h"

namespace zanik::qol {

std::size_t next_compost_bucket(const Bucket& emptied,
                                const std::vector<Bucket>& inventory) {
    if (emptied.type == CompostType::none || emptied.charges != 0) {
        return inventory.size();
    }
    for (std::size_t slot = 0; slot < inventory.size(); ++slot) {
        const Bucket& candidate = inventory[slot];
        if (candidate.instance_id != 0 &&
            candidate.instance_id != emptied.instance_id &&
            candidate.type == emptied.type && candidate.charges > 0) {
            return slot;
        }
    }
    return inventory.size();
}

} // namespace zanik::qol
