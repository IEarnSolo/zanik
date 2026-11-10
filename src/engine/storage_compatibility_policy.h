#pragma once

#include <string_view>

namespace DomEngine::StorageCompatibilityPolicy {

inline constexpr std::string_view LegacyCandidateSha =
    "b5017cf67bc6127dbec443e00fed1f818e82afd5d6d17f912d959a1dd87d5687";
inline constexpr std::string_view PreviousReviewedSha =
    "45cd13cf24658153f4c53777816ffa38ac893a471ebdf5d9c9768f7e4e74c966";
inline constexpr std::string_view CurrentReviewedSha =
    "a350b998ea38955345619b7adde31aa9e29db629e9e9aa32287013957eb41901";

inline bool IsProductionReviewed(std::string_view hash) {
    return hash == "445f8b65047d208adb04a3cb03bc7706f069306d814172ae29caec16c15bec7e" ||
           hash == "ef39023c04e1bde785907e13b9b64d72600455f1633bbfe231f639d31d98cfac" ||
           hash == PreviousReviewedSha || hash == CurrentReviewedSha;
}

inline bool IsCandidateOnly(std::string_view hash) {
    return hash == LegacyCandidateSha;
}

} // namespace DomEngine::StorageCompatibilityPolicy
