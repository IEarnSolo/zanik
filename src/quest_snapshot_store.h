#pragma once
#include "engine/dom_engine.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>
struct StoredQuestProgress { std::string id,status,objective; std::int64_t completedObservedAt=0; };
struct StoredQuestSnapshot { std::string characterGuid,catalogFingerprint;std::vector<StoredQuestProgress> quests;std::int64_t capturedAt=0;bool complete=false; };
class QuestSnapshotStore { public: explicit QuestSnapshotStore(std::string path);bool Load();bool Save()const;bool Sync(const std::vector<DomEngine::PlayerQuestSnapshot>&);bool Find(const std::string&,StoredQuestSnapshot&)const;std::size_t Count()const{return snapshots_.size();}private:std::string path_;std::map<std::string,StoredQuestSnapshot> snapshots_;};
