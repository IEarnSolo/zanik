#include "../compost.h"

#include <cassert>
#include <vector>

using zanik::qol::Bucket;
using zanik::qol::CompostType;
using zanik::qol::next_compost_bucket;

int main() {
    const Bucket regular{1, CompostType::regular, 0};
    const Bucket super{2, CompostType::super, 0};
    const std::vector<Bucket> slots{
        regular, {3, CompostType::super, 4},
        {4, CompostType::regular, 0}, {5, CompostType::regular, 2},
        {6, CompostType::regular, 7}, {7, CompostType::super, 1}};

    assert(next_compost_bucket(regular, slots) == 3);
    assert(next_compost_bucket(super, slots) == 1);
    assert(next_compost_bucket(regular, {slots[0], slots[1], slots[2]}) == 3);
    assert(next_compost_bucket({1, CompostType::regular, 1}, slots) == slots.size());
    assert(next_compost_bucket({1, CompostType::none, 0}, slots) == slots.size());
    assert(next_compost_bucket(regular, {{0, CompostType::regular, 5}}) == 1);
}
