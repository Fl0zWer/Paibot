#pragma once

#include <Geode/Geode.hpp>
#include <string>
#include <fstream>
#include <memory>

namespace paibot {
    class IntegrityLogger {
    private:
        static std::unique_ptr<IntegrityLogger> s_instance;
        std::ofstream m_logFile;
        std::string m_logPath;
        
        IntegrityLogger();
        
    public:
        static IntegrityLogger* get();
        static void destroy();
        
        ~IntegrityLogger();
        
        // Initialize logging system
        bool init();
        
        // Log integrity checks
        void logHashCheck(const std::string& component, const std::string& hash, bool valid);
        void logHookStatus(const std::string& hookName, bool active);
        void logSettingsLoad(bool success, const std::string& details = "");
        void logOperationStart(const std::string& operationId, const std::string& operation);
        void logOperationEnd(const std::string& operationId, bool success, const std::string& details = "");
        void logError(const std::string& component, const std::string& error);
        void logWarning(const std::string& component, const std::string& warning);
        
        // Flush logs to disk
        void flush();
        
        // Get log file path
        std::string getLogPath() const { return m_logPath; }
    };
}