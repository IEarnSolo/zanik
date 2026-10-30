#pragma once

#include <cstdint>
#include <pthread.h>
#include <string>
#include <unordered_map>
#include <vector>

struct AdminSession {
    std::string publicId;
    std::string characterGuid;
    std::string characterName;
    std::string csrf;
    std::string sessionHashPrefix;
};

struct AdminStoreDiagnostics {
    bool allowlistReadable = false;
    bool sessionStorageReadable = false;
    bool sessionStorageWritable = false;
    bool sessionStorageRestricted = false;
    bool lastSaveSucceeded = true;
    std::size_t allowedCharacters = 0;
    std::size_t activeSessions = 0;
    std::size_t malformedSessionRecords = 0;
};

struct ActiveAdminSession {
    std::string hashPrefix;
    std::string publicId;
    std::string characterName;
    std::int64_t created = 0;
};

class AdminStore {
public:
    AdminStore(std::string allowlistPath, std::string sessionPath);
    bool Load();
    std::string CreateClaim(const std::string& publicId, const std::string& characterGuid,
                            const std::string& characterName);
    bool RedeemClaim(const std::string& token, std::string& sessionToken, AdminSession& session);
    bool Authenticate(const std::string& sessionToken, AdminSession& session, bool* removed=nullptr);
    bool Logout(const std::string& sessionToken);
    std::size_t RevokeCharacter(const std::string& publicId);
    std::vector<ActiveAdminSession> ActiveSessions();
    bool RevokeSession(const std::string& hashPrefix);
    bool IsAllowed(const std::string& publicId);
    bool Authorize(const std::string& publicId, const std::string& characterName);
    std::vector<std::string> AllowedIds();
    void Audit(const AdminSession& session, const std::string& destination, const std::string& result);
    std::string SessionHashPrefix(const std::string& sessionToken) const;
    AdminStoreDiagnostics Diagnostics();
private:
    struct Claim { AdminSession session; std::int64_t expires=0; };
    struct Stored { AdminSession session; std::string hash; std::int64_t created=0; };
    void ReloadAllowlistLocked();
    bool SaveLocked() const;
    std::string RandomToken(std::size_t bytes) const;
    std::string Hash(const std::string& value) const;
    std::string allowlistPath_, sessionPath_, auditPath_;
    std::int64_t allowlistMtime_=-1;
    std::unordered_map<std::string,bool> allowed_;
    std::unordered_map<std::string,Claim> claims_;
    std::unordered_map<std::string,Stored> sessions_;
    std::unordered_map<std::string,bool> removedSessions_;
    std::size_t malformedSessionRecords_ = 0;
    mutable bool lastSaveSucceeded_ = true;
    mutable pthread_mutex_t mutex_ = PTHREAD_MUTEX_INITIALIZER;
};
