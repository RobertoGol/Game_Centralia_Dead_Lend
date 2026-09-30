#pragma once
#include <string>
#include <cstdint>
#include "platform/Platform.hpp"

namespace Centralia {

enum class AuthStatus : uint8_t {
    Disconnected,
    Authenticating,
    Authorized,
    Banned,
    Error
};

class LoginSystem {
private:
    AuthStatus m_status;
    std::string m_username;
    std::string m_sessionToken;
    uint64_t m_accountUid;

    LoginSystem() noexcept;

public:
    ~LoginSystem() = default;

    LoginSystem(const LoginSystem&) = delete;
    LoginSystem& operator=(const LoginSystem&) = delete;

    static LoginSystem& GetInstance() noexcept {
        static LoginSystem instance;
        return instance;
    }

    bool Login(const std::string& username, const std::string& passwordHash);
    void Logout() noexcept;
    bool ValidateSessionToken(const std::string& token) const noexcept;

    [[nodiscard]] AuthStatus GetAuthStatus() const noexcept { return m_status; }
    [[nodiscard]] const std::string& GetUsername() const noexcept { return m_username; }
    [[nodiscard]] uint64_t GetAccountUID() const noexcept { return m_accountUid; }
};

} // namespace Centralia