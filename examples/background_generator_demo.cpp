/**
 * Example usage of the enhanced Background Generator system
 * This demonstrates the three modes: Procedural, Wang Tiles, and Geometrization
 */

#include <util/BackgroundGenerator.hpp>
#include <manager/PackManager.hpp>
#include <Geode/Geode.hpp>

using namespace paibot;
using namespace geode::prelude;

void demonstrateProceduralGeneration() {
    log::info("=== Procedural Background Generation Demo ===");
    
    auto generator = BackgroundGenerator::create();
    if (!generator) {
        log::error("Failed to create BackgroundGenerator");
        return;
    }
    
    // Configure procedural settings
    BackgroundSettings settings;
    settings.type = BackgroundType::Procedural;
    settings.tileSize = 1024;
    settings.noiseSeed = 42;
    settings.noiseType = NoiseType::Perlin;
    settings.noiseScale = 0.15f;
    settings.octaves = 4;
    settings.persistence = 0.5f;
    settings.lacunarity = 2.0f;
    
    generator->setSettings(settings);
    
    // Generate background
    auto tileSet = generator->generateBackground();
    if (tileSet.isValid()) {
        log::info("Generated procedural background with {} tiles", tileSet.tiles.size());
        log::info("Seamlessness (Delta E): {:.2f}", tileSet.deltaE);
        
        // Export the result
        generator->exportTileSet("procedural_export");
        log::info("Exported procedural background to 'procedural_export' directory");
    }
}

void demonstrateWangTileGeneration() {
    log::info("=== Wang Tiles Generation Demo ===");
    
    auto generator = BackgroundGenerator::create();
    if (!generator) {
        log::error("Failed to create BackgroundGenerator");
        return;
    }
    
    // Configure Wang tile settings
    BackgroundSettings settings;
    settings.type = BackgroundType::WangTiles;
    settings.tileSize = 512;
    settings.noiseSeed = 123;
    settings.continuity = 0.9f;  // High continuity for seamless edges
    settings.variety = 0.6f;
    
    generator->setSettings(settings);
    
    // Generate Wang tiles
    auto tileSet = generator->generateBackground();
    if (tileSet.isValid()) {
        log::info("Generated Wang tile set with {} tiles", tileSet.tiles.size());
        
        // Validate edge compatibility
        auto validation = generator->validateWangTileBorders(tileSet);
        if (validation.hasValidBorders) {
            log::info("Wang tiles pass validation - Border consistency: {:.2f}", validation.borderConsistency);
            
            // Generate a sample layout
            auto layout = generator->generateTileLayout(8, 6);
            log::info("Generated {}x{} tile layout", layout[0].size(), layout.size());
        } else {
            log::warn("Wang tile validation failed: {}", validation.errorDetails);
        }
        
        // Export with compatibility matrix
        generator->exportTileSet("wang_tiles_export");
        log::info("Exported Wang tiles to 'wang_tiles_export' directory");
    }
}

void demonstrateGeometrizationMode() {
    log::info("=== Geometrization Mode Demo ===");
    
    auto generator = BackgroundGenerator::create();
    if (!generator) {
        log::error("Failed to create BackgroundGenerator");
        return;
    }
    
    // Configure geometrization settings
    BackgroundSettings settings;
    settings.type = BackgroundType::Geometrization;
    settings.sourceImagePath = "sample_image.png";  // Would be actual image path
    settings.colorTolerance = 0.15f;
    settings.maxColors = 8;
    settings.simplificationTolerance = 1.0f;
    settings.targetResolution = 512;
    settings.optimizeForTiling = true;
    
    generator->setSettings(settings);
    
    // Generate geometric pattern
    auto tileSet = generator->generateBackground();
    if (tileSet.isValid()) {
        log::info("Generated geometric pattern with {} regions", tileSet.tiles.size());
        log::info("Pattern optimized for tiling: {}", settings.optimizeForTiling ? "Yes" : "No");
        
        // Export the geometric pattern
        generator->exportTileSet("geometric_export");
        log::info("Exported geometric pattern to 'geometric_export' directory");
    }
}

void demonstratePackManagement() {
    log::info("=== Pack Management Demo ===");
    
    auto packManager = PackManager::get();
    
    // Scan for available packs
    packManager->scanPacksDirectory();
    auto availablePacks = packManager->getAvailablePacks();
    
    log::info("Found {} available packs", availablePacks.size());
    
    for (const auto& pack : availablePacks) {
        log::info("Pack: {} v{} by {}", pack.name, pack.version, pack.author);
        log::info("  Description: {}", pack.description);
        log::info("  Backgrounds: {}", pack.backgrounds.size());
        log::info("  Active: {}", pack.isActive ? "Yes" : "No");
        
        if (pack.hasConflicts) {
            log::warn("  Conflicts detected with: {}", fmt::join(pack.conflicts, ", "));
        }
    }
    
    // Demonstrate pack installation (would be actual pack path)
    // packManager->installPack("path/to/new/pack");
    
    // Demonstrate pack activation
    if (!availablePacks.empty()) {
        std::string packId = packManager->generatePackId(availablePacks[0]);
        if (packManager->activatePack(packId)) {
            log::info("Successfully activated pack: {}", availablePacks[0].name);
        }
    }
}

void demonstrateIntegrityAndValidation() {
    log::info("=== Integrity and Validation Demo ===");
    
    auto generator = BackgroundGenerator::create();
    if (!generator) {
        log::error("Failed to create BackgroundGenerator");
        return;
    }
    
    // Test with invalid settings
    BackgroundSettings invalidSettings;
    invalidSettings.type = BackgroundType::Procedural;
    invalidSettings.tileSize = -1;  // Invalid
    invalidSettings.octaves = 20;   // Invalid
    invalidSettings.noiseScale = 2.0f;  // Invalid
    
    log::info("Testing validation with invalid settings...");
    generator->setSettings(invalidSettings);  // Should fail validation
    
    // Test with valid settings
    BackgroundSettings validSettings;
    validSettings.type = BackgroundType::Procedural;
    validSettings.tileSize = 512;
    validSettings.noiseSeed = 12345;
    validSettings.octaves = 3;
    validSettings.noiseScale = 0.1f;
    
    log::info("Testing with valid settings...");
    generator->setSettings(validSettings);
    
    auto tileSet = generator->generateBackground();
    
    // Test preset versioning
    log::info("Current preset version: {}", validSettings.version);
    
    if (generator->validatePresetVersion(validSettings)) {
        log::info("Preset version is current");
    } else {
        log::info("Preset needs migration");
        BackgroundSettings migratedSettings = validSettings;
        generator->migratePreset(migratedSettings, 0, 1);
        log::info("Migrated preset to version: {}", migratedSettings.version);
    }
}

/**
 * Main demonstration function
 * This would typically be called from a mod initialization or UI handler
 */
void demonstrateBackgroundGeneratorSystem() {
    log::info("🎨 Paibot Background Generator System Demo");
    log::info("============================================");
    
    // Initialize pack manager
    auto packManager = PackManager::get();
    log::info("Pack manager initialized. Packs directory: {}", packManager->getPacksDirectory());
    
    // Demonstrate each mode
    demonstrateProceduralGeneration();
    log::info("");
    
    demonstrateWangTileGeneration();
    log::info("");
    
    demonstrateGeometrizationMode();
    log::info("");
    
    demonstratePackManagement();
    log::info("");
    
    demonstrateIntegrityAndValidation();
    log::info("");
    
    log::info("✅ Background Generator demonstration completed");
    
    // Cleanup
    PackManager::destroy();
}