#include <manager/PackManager.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace paibot;
using namespace geode::prelude;

PackManager* PackManager::s_instance = nullptr;

PackManager* PackManager::get() {
    if (!s_instance) {
        s_instance = new PackManager();
    }
    return s_instance;
}

void PackManager::destroy() {
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

PackManager::PackManager() {
    auto modPath = Mod::get()->getResourcesDir();
    m_packsDirectory = (modPath / "packs").string();
    
    // Create packs directory if it doesn't exist
    if (!std::filesystem::exists(m_packsDirectory)) {
        std::filesystem::create_directories(m_packsDirectory);
    }
    
    loadPackConfiguration();
    scanPacksDirectory();
}

PackManager::~PackManager() {
    savePackConfiguration();
}

bool PackManager::installPack(const std::string& packPath) {
    IntegrityLogger::get()->logOperationStart("PackInstall", "Installing pack: " + packPath);
    
    if (!validatePack(packPath)) {
        IntegrityLogger::get()->logError("PackManager", "Pack validation failed: " + packPath);
        return false;
    }
    
    PackInfo packInfo;
    if (!parsePackJson(packPath + "/pack.json", packInfo)) {
        IntegrityLogger::get()->logError("PackManager", "Failed to parse pack.json: " + packPath);
        return false;
    }
    
    // Check compatibility
    auto compatibility = checkCompatibility(packInfo);
    if (!compatibility.isSupported) {
        IntegrityLogger::get()->logError("PackManager", "Pack incompatible: " + compatibility.warningMessage);
        return false;
    }
    
    // Check for conflicts
    auto conflicts = detectConflicts(packInfo);
    if (!conflicts.empty()) {
        log::warn("Pack has conflicts with: {}", fmt::join(conflicts, ", "));
        packInfo.hasConflicts = true;
        packInfo.conflicts = conflicts;
    }
    
    // Generate pack ID and copy to packs directory
    std::string packId = generatePackId(packInfo);
    std::string destPath = m_packsDirectory + "/" + packId;
    
    if (!copyPackToDirectory(packPath, destPath)) {
        IntegrityLogger::get()->logError("PackManager", "Failed to copy pack to: " + destPath);
        return false;
    }
    
    // Calculate and store hash
    packInfo.hash = calculatePackHash(destPath);
    m_loadedPacks[packId] = packInfo;
    
    savePackConfiguration();
    
    IntegrityLogger::get()->logOperationEnd("PackInstall", true, "Pack installed successfully: " + packId);
    log::info("Installed pack: {} by {}", packInfo.name, packInfo.author);
    return true;
}

bool PackManager::uninstallPack(const std::string& packId) {
    auto it = m_loadedPacks.find(packId);
    if (it == m_loadedPacks.end()) {
        log::error("Pack not found: {}", packId);
        return false;
    }
    
    // Deactivate pack first
    deactivatePack(packId);
    
    // Remove from filesystem
    std::string packPath = m_packsDirectory + "/" + packId;
    try {
        std::filesystem::remove_all(packPath);
    } catch (const std::exception& e) {
        log::error("Failed to remove pack directory: {}", e.what());
        return false;
    }
    
    // Remove from loaded packs
    m_loadedPacks.erase(it);
    savePackConfiguration();
    
    log::info("Uninstalled pack: {}", packId);
    return true;
}

bool PackManager::activatePack(const std::string& packId) {
    auto it = m_loadedPacks.find(packId);
    if (it == m_loadedPacks.end()) {
        log::error("Cannot activate unknown pack: {}", packId);
        return false;
    }
    
    if (it->second.hasConflicts) {
        log::warn("Activating pack with conflicts: {}", packId);
    }
    
    if (std::find(m_activePacks.begin(), m_activePacks.end(), packId) == m_activePacks.end()) {
        m_activePacks.push_back(packId);
        it->second.isActive = true;
        savePackConfiguration();
        log::info("Activated pack: {}", it->second.name);
        return true;
    }
    
    return false; // Already active
}

bool PackManager::deactivatePack(const std::string& packId) {
    auto it = std::find(m_activePacks.begin(), m_activePacks.end(), packId);
    if (it != m_activePacks.end()) {
        m_activePacks.erase(it);
        
        auto packIt = m_loadedPacks.find(packId);
        if (packIt != m_loadedPacks.end()) {
            packIt->second.isActive = false;
        }
        
        savePackConfiguration();
        log::info("Deactivated pack: {}", packId);
        return true;
    }
    
    return false; // Not active
}

bool PackManager::validatePack(const std::string& packPath) {
    // Check if pack directory exists
    if (!std::filesystem::exists(packPath)) {
        return false;
    }
    
    // Check for required pack.json
    std::string packJsonPath = packPath + "/pack.json";
    if (!std::filesystem::exists(packJsonPath)) {
        return false;
    }
    
    // Validate pack structure
    return validatePackStructure(packPath);
}

bool PackManager::validatePackInfo(const PackInfo& pack) {
    if (!pack.isValid()) {
        return false;
    }
    
    // Check version format (simple validation)
    if (pack.version.empty() || pack.version.find_first_not_of("0123456789.") != std::string::npos) {
        return false;
    }
    
    return true;
}

PackCompatibility PackManager::checkCompatibility(const PackInfo& pack) {
    PackCompatibility compat;
    
    // Get current versions
    auto modInfo = Mod::get()->getMetadata();
    compat.gameVersion = "2.207"; // Would get from game in real implementation
    compat.geodeVersion = "4.8.0";
    
    // Simple compatibility check (would be more sophisticated in real implementation)
    compat.isSupported = true;
    
    // Check if pack requires newer versions
    // This would parse pack requirements in real implementation
    
    return compat;
}

std::vector<std::string> PackManager::detectConflicts(const PackInfo& pack) {
    std::vector<std::string> conflicts;
    
    // Check for background name conflicts
    for (const auto& [packId, loadedPack] : m_loadedPacks) {
        if (packId == generatePackId(pack)) continue; // Skip self
        
        for (const auto& bg1 : pack.backgrounds) {
            for (const auto& bg2 : loadedPack.backgrounds) {
                if (bg1 == bg2) {
                    conflicts.push_back(packId + " (background: " + bg1 + ")");
                }
            }
        }
    }
    
    return conflicts;
}

void PackManager::scanPacksDirectory() {
    if (!std::filesystem::exists(m_packsDirectory)) {
        return;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(m_packsDirectory)) {
        if (entry.is_directory()) {
            loadPack(entry.path().string());
        }
    }
}

void PackManager::loadPack(const std::string& packPath) {
    std::string packJsonPath = packPath + "/pack.json";
    if (!std::filesystem::exists(packJsonPath)) {
        return;
    }
    
    PackInfo packInfo;
    if (parsePackJson(packJsonPath, packInfo)) {
        std::string packId = generatePackId(packInfo);
        
        // Verify integrity if enabled
        if (m_integrityChecksEnabled) {
            std::string currentHash = calculatePackHash(packPath);
            if (!packInfo.hash.empty() && packInfo.hash != currentHash) {
                log::warn("Pack integrity check failed: {}", packId);
                return;
            }
            packInfo.hash = currentHash;
        }
        
        m_loadedPacks[packId] = packInfo;
        log::debug("Loaded pack: {}", packInfo.name);
    }
}

void PackManager::refreshPacks() {
    m_loadedPacks.clear();
    scanPacksDirectory();
}

std::vector<PackInfo> PackManager::getAvailablePacks() const {
    std::vector<PackInfo> packs;
    for (const auto& [packId, pack] : m_loadedPacks) {
        packs.push_back(pack);
    }
    return packs;
}

std::vector<PackInfo> PackManager::getActivePacks() const {
    std::vector<PackInfo> packs;
    for (const std::string& packId : m_activePacks) {
        auto it = m_loadedPacks.find(packId);
        if (it != m_loadedPacks.end()) {
            packs.push_back(it->second);
        }
    }
    return packs;
}

PackInfo PackManager::getPackInfo(const std::string& packId) const {
    auto it = m_loadedPacks.find(packId);
    if (it != m_loadedPacks.end()) {
        return it->second;
    }
    return PackInfo{}; // Return empty pack info if not found
}

bool PackManager::isPackActive(const std::string& packId) const {
    return std::find(m_activePacks.begin(), m_activePacks.end(), packId) != m_activePacks.end();
}

void PackManager::setPacksDirectory(const std::string& directory) {
    m_packsDirectory = directory;
    if (!std::filesystem::exists(m_packsDirectory)) {
        std::filesystem::create_directories(m_packsDirectory);
    }
}

bool PackManager::verifyPackIntegrity(const std::string& packId) {
    auto it = m_loadedPacks.find(packId);
    if (it == m_loadedPacks.end()) {
        return false;
    }
    
    std::string packPath = m_packsDirectory + "/" + packId;
    std::string currentHash = calculatePackHash(packPath);
    
    return currentHash == it->second.hash;
}

std::string PackManager::calculatePackHash(const std::string& packPath) {
    // Simplified hash calculation - in real implementation would use proper crypto
    std::hash<std::string> hasher;
    return std::to_string(hasher(packPath + std::to_string(std::filesystem::file_size(packPath + "/pack.json"))));
}

bool PackManager::checkPackSignature(const std::string& packPath) {
    // Placeholder for pack signature verification
    // In real implementation would verify digital signatures
    return true;
}

void PackManager::savePackConfiguration() {
    try {
        std::string configPath = m_packsDirectory + "/config.txt";
        std::ofstream file(configPath);
        
        file << "# Paibot Pack Configuration v1.0\n";
        file << "active_packs=" << m_activePacks.size() << "\n";
        
        for (const std::string& packId : m_activePacks) {
            file << "pack:" << packId << "\n";
        }
        
        file.close();
        
    } catch (const std::exception& e) {
        log::error("Failed to save pack configuration: {}", e.what());
    }
}

void PackManager::loadPackConfiguration() {
    try {
        std::string configPath = m_packsDirectory + "/config.txt";
        if (!std::filesystem::exists(configPath)) {
            return;
        }
        
        std::ifstream file(configPath);
        std::string line;
        
        while (std::getline(file, line)) {
            if (line.starts_with("pack:")) {
                std::string packId = line.substr(5);
                if (!packId.empty()) {
                    m_activePacks.push_back(packId);
                }
            }
        }
        
    } catch (const std::exception& e) {
        log::error("Failed to load pack configuration: {}", e.what());
    }
}

std::string PackManager::generatePackId(const PackInfo& pack) const {
    // Generate unique pack ID from name and author
    std::string id = pack.name + "_" + pack.author + "_" + pack.version;
    
    // Replace invalid characters
    std::replace_if(id.begin(), id.end(), [](char c) {
        return !std::isalnum(c) && c != '_' && c != '-';
    }, '_');
    
    return id;
}

bool PackManager::parsePackJson(const std::string& jsonPath, PackInfo& pack) {
    try {
        std::ifstream file(jsonPath);
        if (!file.is_open()) {
            return false;
        }
        
        // Simple JSON-like parsing (placeholder for proper JSON parser)
        std::string line;
        while (std::getline(file, line)) {
            // Remove whitespace and quotes
            line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
            line.erase(std::remove(line.begin(), line.end(), '"'), line.end());
            line.erase(std::remove(line.begin(), line.end(), ','), line.end());
            
            if (line.find("name:") == 0) {
                pack.name = line.substr(5);
            } else if (line.find("author:") == 0) {
                pack.author = line.substr(7);
            } else if (line.find("version:") == 0) {
                pack.version = line.substr(8);
            } else if (line.find("description:") == 0) {
                pack.description = line.substr(12);
            } else if (line.find("icon:") == 0) {
                pack.iconPath = line.substr(5);
            }
            // Note: This is a very simplified parser - real implementation would need proper JSON parsing
        }
        
        // Add some default backgrounds for testing
        pack.backgrounds = {"default_bg_1", "default_bg_2"};
        
        return pack.isValid();
        
    } catch (const std::exception& e) {
        log::error("Failed to parse pack.json: {}", e.what());
        return false;
    }
}

bool PackManager::validatePackStructure(const std::string& packPath) {
    // Check for required directories and files
    std::vector<std::string> requiredPaths = {
        packPath + "/pack.json",
        packPath + "/backgrounds"
    };
    
    for (const std::string& path : requiredPaths) {
        if (!std::filesystem::exists(path)) {
            return false;
        }
    }
    
    return true;
}

bool PackManager::copyPackToDirectory(const std::string& sourcePath, const std::string& destPath) {
    try {
        if (std::filesystem::exists(destPath)) {
            std::filesystem::remove_all(destPath);
        }
        
        std::filesystem::copy(sourcePath, destPath, 
            std::filesystem::copy_options::recursive | 
            std::filesystem::copy_options::overwrite_existing);
        
        return true;
        
    } catch (const std::exception& e) {
        log::error("Failed to copy pack: {}", e.what());
        return false;
    }
}

void PackManager::resolveConflicts() {
    // Placeholder for conflict resolution logic
    // Could implement priority-based resolution, user prompts, etc.
}