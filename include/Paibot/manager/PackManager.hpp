#pragma once

#include <Geode/Geode.hpp>
#include <util/IntegrityLogger.hpp>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace paibot {
    struct PackInfo {
        std::string name;
        std::string author;
        std::string version;
        std::string description;
        std::vector<std::string> backgrounds;
        std::string iconPath;
        std::string hash;
        bool isActive = false;
        bool hasConflicts = false;
        std::vector<std::string> conflicts;
        
        bool isValid() const {
            return !name.empty() && !version.empty() && !backgrounds.empty();
        }
    };
    
    struct PackCompatibility {
        std::string gameVersion;
        std::string geodeVersion;
        bool isSupported = true;
        std::string warningMessage;
    };
    
    class PackManager {
    private:
        static PackManager* s_instance;
        std::unordered_map<std::string, PackInfo> m_loadedPacks;
        std::vector<std::string> m_activePacks;
        std::string m_packsDirectory;
        bool m_integrityChecksEnabled = true;
        
        PackManager();
        
    public:
        static PackManager* get();
        static void destroy();
        
        ~PackManager();
        
        // Pack installation and management
        bool installPack(const std::string& packPath);
        bool uninstallPack(const std::string& packId);
        bool activatePack(const std::string& packId);
        bool deactivatePack(const std::string& packId);
        
        // Pack validation and integrity
        bool validatePack(const std::string& packPath);
        bool validatePackInfo(const PackInfo& pack);
        PackCompatibility checkCompatibility(const PackInfo& pack);
        std::vector<std::string> detectConflicts(const PackInfo& pack);
        
        // Pack discovery and loading
        void scanPacksDirectory();
        void loadPack(const std::string& packPath);
        void refreshPacks();
        
        // Pack information access
        std::vector<PackInfo> getAvailablePacks() const;
        std::vector<PackInfo> getActivePacks() const;
        PackInfo getPackInfo(const std::string& packId) const;
        bool isPackActive(const std::string& packId) const;
        
        // Configuration
        void setPacksDirectory(const std::string& directory);
        std::string getPacksDirectory() const { return m_packsDirectory; }
        void setIntegrityChecks(bool enabled) { m_integrityChecksEnabled = enabled; }
        
        // Integrity and verification
        bool verifyPackIntegrity(const std::string& packId);
        std::string calculatePackHash(const std::string& packPath);
        bool checkPackSignature(const std::string& packPath);
        
        // Utilities
        void savePackConfiguration();
        void loadPackConfiguration();
        std::string generatePackId(const PackInfo& pack) const;
        
    private:
        bool parsePackJson(const std::string& jsonPath, PackInfo& pack);
        bool validatePackStructure(const std::string& packPath);
        bool copyPackToDirectory(const std::string& sourcePath, const std::string& destPath);
        void resolveConflicts();
    };
}