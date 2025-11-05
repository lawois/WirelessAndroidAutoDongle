#include <cstdlib>
#include <cstdarg>
#include <sstream>
#include <fstream>
#include <syslog.h>

#include "common.h"
#include "proto/WifiInfoResponse.pb.h"

#pragma region Config
/*static*/ Config* Config::instance() {
    static Config s_instance;
    return &s_instance;
}

int32_t Config::getenv(std::string name, int32_t defaultValue) {
    char* envValue = std::getenv(name.c_str());
    try {
        return envValue != nullptr ? std::stoi(envValue) : defaultValue;
    }
    catch(...) {
        return defaultValue;
    }
}

std::string Config::getenv(std::string name, std::string defaultValue) {
    char* envValue = std::getenv(name.c_str());
    return envValue != nullptr ? envValue : defaultValue;
}

std::string Config::getMacAddress(std::string interface) {
    std::ifstream addressFile("/sys/class/net/" + interface + "/address");

    std::string macAddress;
    getline(addressFile, macAddress);

    return macAddress;
}

std::string Config::getUniqueSuffix() {
    std::string uniqueSuffix = getenv("AAWG_UNIQUE_NAME_SUFFIX", "");
    if (!uniqueSuffix.empty()) {
        return uniqueSuffix;
    }

    std::ifstream serialNumberFile("/sys/firmware/devicetree/base/serial-number");

    std::string serialNumber;
    getline(serialNumberFile, serialNumber);

    // Removing trailing null from serialNumber, pad at the beginning
    serialNumber = std::string("00000000") + serialNumber.c_str();

    return serialNumber.substr(serialNumber.size() - 6);
}

WifiInfo Config::getWifiInfo() {
    return {
        getenv("AAWG_WIFI_SSID", "AAWirelessDongle"),
        getenv("AAWG_WIFI_PASSWORD", "ConnectAAWirelessDongle"),
        getenv("AAWG_WIFI_BSSID", getMacAddress("wlan0")),
        SecurityMode::WPA2_PERSONAL,
        AccessPointType::DYNAMIC,
        getenv("AAWG_PROXY_IP_ADDRESS", "10.0.0.1"),
        getenv("AAWG_PROXY_PORT", 5288),
    };
}

ConnectionStrategy Config::getConnectionStrategy() {
    if (!connectionStrategy.has_value()) {
        const int32_t connectionStrategyEnv = getenv("AAWG_CONNECTION_STRATEGY", 1);

        switch (connectionStrategyEnv) {
            case 0:
                connectionStrategy = ConnectionStrategy::DONGLE_MODE;
                break;
            case 1:
                connectionStrategy = ConnectionStrategy::PHONE_FIRST;
                break;
            case 2:
                connectionStrategy = ConnectionStrategy::USB_FIRST;
                break;
            default:
                connectionStrategy = ConnectionStrategy::PHONE_FIRST;
                break;
        }
    }

    return connectionStrategy.value();
}
#pragma endregion Config

#pragma region Logger
/*static*/ Logger* Logger::instance() {
    static Logger s_instance;
    return &s_instance;
}

Logger::Logger() {
    openlog(nullptr, LOG_PERROR | LOG_PID, LOG_USER);

    // Read log level from environment
    char* envLevel = std::getenv("AAWG_LOG_LEVEL");
    if (envLevel != nullptr) {
        std::string level(envLevel);
        if (level == "DEBUG") {
            m_logLevel = LogLevel::DEBUG;
        } else if (level == "INFO") {
            m_logLevel = LogLevel::INFO;
        } else if (level == "WARN") {
            m_logLevel = LogLevel::WARN;
        } else if (level == "ERROR") {
            m_logLevel = LogLevel::ERROR;
        }
    }
}

Logger::~Logger() {
    closelog();
}

void Logger::log(LogLevel level, const char *format, va_list args) {
    if (level < m_logLevel) {
        return;
    }

    int priority;
    const char* levelStr;

    switch (level) {
        case LogLevel::DEBUG:
            priority = LOG_DEBUG;
            levelStr = "DEBUG";
            break;
        case LogLevel::INFO:
            priority = LOG_INFO;
            levelStr = "INFO";
            break;
        case LogLevel::WARN:
            priority = LOG_WARNING;
            levelStr = "WARN";
            break;
        case LogLevel::ERROR:
            priority = LOG_ERR;
            levelStr = "ERROR";
            break;
        default:
            priority = LOG_INFO;
            levelStr = "INFO";
            break;
    }

    vsyslog(priority, format, args);
}

void Logger::debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log(LogLevel::DEBUG, format, args);
    va_end(args);
}

void Logger::info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log(LogLevel::INFO, format, args);
    va_end(args);
}

void Logger::warn(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log(LogLevel::WARN, format, args);
    va_end(args);
}

void Logger::error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log(LogLevel::ERROR, format, args);
    va_end(args);
}

void Logger::setLogLevel(LogLevel level) {
    m_logLevel = level;
}

LogLevel Logger::getLogLevel() const {
    return m_logLevel;
}
#pragma endregion Logger