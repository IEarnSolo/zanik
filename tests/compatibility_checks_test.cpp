#include "../tools/compatibility_checks.h"

#include <cassert>
#include <vector>

int main() {
    const unsigned char signature[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57};
    std::vector<unsigned char> binary(32, 0x90);
    std::copy(std::begin(signature), std::end(signature), binary.begin() + 8);
    assert(CompatibilityChecks::ExactBytesAt(binary, 8, signature, sizeof(signature)));
    binary[10] ^= 0x01;
    assert(!CompatibilityChecks::ExactBytesAt(binary, 8, signature, sizeof(signature)));
    assert(!CompatibilityChecks::ExactBytesAt(binary, 30, signature, sizeof(signature)));

    std::vector<unsigned char> narrow{'L','o','g','M','a','p','R','e','g','i','o','n','s'};
    assert(CompatibilityChecks::ContainsContractText(narrow, "LogMapRegions"));
    narrow[3] = 'X';
    assert(!CompatibilityChecks::ContainsContractText(narrow, "LogMapRegions"));
    std::vector<unsigned char> wide{'P',0,'i',0,'n',0,'g',0};
    assert(CompatibilityChecks::ContainsContractText(wide, "Ping"));

    const unsigned char delayedTeleport[] = {
        0x55,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,
        0x48,0x81,0xec,0x18,0x01,0x00,0x00
    };
    std::vector<unsigned char> delayed(delayedTeleport, delayedTeleport + sizeof(delayedTeleport));
    assert(CompatibilityChecks::ExactBytesAt(delayed, 0, delayedTeleport, sizeof(delayedTeleport)));
    delayed[13] ^= 0x08;
    assert(!CompatibilityChecks::ExactBytesAt(delayed, 0, delayedTeleport, sizeof(delayedTeleport)));
}
