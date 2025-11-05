#pragma once

#include <string>
#include <cstdint>
#include <optional>

enum SecurityMode: int;
enum AccessPointType: int;

struct WifiInfo {
    std::string ssid;
    std::string key;
    std::string bssid;
    SecurityMode securityMode;
    AccessPointType accessPointType;
    std::string ipAddress;
    int32_t port;
};

enum class ConnectionStrategy {
    DONGLE_MODE = 0,
    PHONE_FIRST = 1,
    USB_FIRST = 2
};

struct RetryConfig {
    int32_t maxRetries;           // Maximum number of retry attempts (0 = infinite)
    int32_t initialDelayMs;       // Initial delay in milliseconds
    int32_t maxDelayMs;           // Maximum delay in milliseconds
    double backoffMultiplier;     // Multiplier for exponential backoff
};

class Config {
public:
    static Config* instance();

    WifiInfo getWifiInfo();
    ConnectionStrategy getConnectionStrategy();
    RetryConfig getRetryConfig();

    std::string getUniqueSuffix();

    // Validation
    bool validate();

private:
    Config() = default;

    int32_t getenv(std::string name, int32_t defaultValue);
    std::string getenv(std::string name, std::string defaultValue);
    double getenv(std::string name, double defaultValue);

    std::string getMacAddress(std::string interface);

    bool validateWifiInfo(const WifiInfo& wifi);
    bool validateRetryConfig(const RetryConfig& retry);

    std::optional<ConnectionStrategy> connectionStrategy;
};

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

class Logger {
public:
    static Logger* instance();

    void debug(const char *format, ...);
    void info(const char *format, ...);
    void warn(const char *format, ...);
    void error(const char *format, ...);

    void setLogLevel(LogLevel level);
    LogLevel getLogLevel() const;

private:
    Logger();
    ~Logger();

    void log(LogLevel level, const char *format, va_list args);

    LogLevel m_logLevel = LogLevel::INFO;
};