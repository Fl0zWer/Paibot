#include <util/IntegrityLogger.hpp>
#include <filesystem>
#include <chrono>
#include <iomanip>

using namespace paibot;
using namespace geode::prelude;

std::unique_ptr<IntegrityLogger> IntegrityLogger::s_instance = nullptr;

IntegrityLogger* IntegrityLogger::get() {
    if (!s_instance) {
        s_instance = std::make_unique<IntegrityLogger>();
        s_instance->init();
    }
    return s_instance.get();
}

void IntegrityLogger::destroy() {
    if (s_instance) {
        s_instance->flush();
        s_instance.reset();
    }
}

IntegrityLogger::IntegrityLogger() = default;

IntegrityLogger::~IntegrityLogger() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

bool IntegrityLogger::init() {
    try {
        // Create logs directory in mod folder
        auto modPath = Mod::get()->getConfigDir();
        auto logsDir = modPath / "logs";
        std::filesystem::create_directories(logsDir);
        
        // Create integrity log file with timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream filename;
        filename << "paibot_integrity_" 
                 << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") 
                 << ".log";
        
        m_logPath = (logsDir / filename.str()).string();
        m_logFile.open(m_logPath, std::ios::out | std::ios::app);
        
        if (!m_logFile.is_open()) {
            log::error("Failed to open integrity log file: {}", m_logPath);
            return false;
        }
        
        // Write header
        m_logFile << "=== Paibot Integrity Log ===" << std::endl;
        m_logFile << "Started: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << std::endl;
        m_logFile << "Mod Version: " << Mod::get()->getVersion().toString() << std::endl;
        m_logFile << "================================" << std::endl;
        
        log::info("Integrity logging initialized: {}", m_logPath);
        return true;
        
    } catch (const std::exception& e) {
        log::error("Failed to initialize integrity logger: {}", e.what());
        return false;
    }
}

void IntegrityLogger::logHashCheck(const std::string& component, const std::string& hash, bool valid) {
    if (!m_logFile.is_open()) return;
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    m_logFile << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "] "
              << "HASH_CHECK " << component << " " << hash << " " 
              << (valid ? "OK" : "FAIL") << std::endl;
}

void IntegrityLogger::logHookStatus(const std::string& hookName, bool active) {
    if (!m_logFile.is_open()) return;
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    m_logFile << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "] "
              << "HOOK_STATUS " << hookName << " " 
              << (active ? "ACTIVE" : "INACTIVE") << std::endl;
}

void IntegrityLogger::logSettingsLoad(bool success, const std::string& details) {
    if (!m_logFile.is_open()) return;
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    m_logFile << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "] "
              << "SETTINGS_LOAD " << (success ? "OK" : "FAIL");
    if (!details.empty()) {
        m_logFile << " " << details;
    }
    m_logFile << std::endl;
}

void IntegrityLogger::logOperationStart(const std::string& operationId, const std::string& operation) {
    if (!m_logFile.is_open()) return;
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    m_logFile << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "] "
              << "OP_START " << operationId << " " << operation << std::endl;
}

void IntegrityLogger::logOperationEnd(const std::string& operationId, bool success, const std::string& details) {
    if (!m_logFile.is_open()) return;
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    m_logFile << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "] "
              << "OP_END " << operationId << " " << (success ? "OK" : "FAIL");
    if (!details.empty()) {
        m_logFile << " " << details;
    }
    m_logFile << std::endl;
}

void IntegrityLogger::logError(const std::string& component, const std::string& error) {
    if (!m_logFile.is_open()) return;
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    m_logFile << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "] "
              << "ERROR " << component << " " << error << std::endl;
    
    // Also log to Geode's main log
    log::error("[{}] {}", component, error);
}

void IntegrityLogger::logWarning(const std::string& component, const std::string& warning) {
    if (!m_logFile.is_open()) return;
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    m_logFile << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "] "
              << "WARN " << component << " " << warning << std::endl;
    
    // Also log to Geode's main log
    log::warn("[{}] {}", component, warning);
}

void IntegrityLogger::flush() {
    if (m_logFile.is_open()) {
        m_logFile.flush();
    }
}