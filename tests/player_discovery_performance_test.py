from pathlib import Path

source = (Path(__file__).resolve().parents[1] / "src/engine/players.cpp").read_text()

# Steady-state player synchronization must use PlayerState::PawnPrivate and
# must not walk the global object array merely to prove the same relationship.
direct = 'uintptr_t pawn = Mem::ReadPtr(state + Offsets::PlayerState_PawnPrivate);'
fallback = 'if (!listedStateIsLive) {'
scan = 'for (int32_t i = 0; i < objectCount; ++i) {'
assert direct in source
assert fallback in source
assert source.index(direct) < source.index(fallback) < source.index(scan, source.index(fallback))

# Pawn survival-stat components are stable enough to cache by UE object
# identity, with a bounded refresh for respawns and pointer reuse.
assert "CachedPawnComponents" in source
assert "pawnComponentCache" in source
assert "cached->second.pawnIdentity == identity" in source
assert "componentNow - cached->second.refreshedAt < std::chrono::seconds(30)" in source
assert "if (!componentMisses.empty())" in source

print("steady-state player discovery avoids repeated whole-registry scans: ok")
