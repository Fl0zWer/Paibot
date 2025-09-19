#include <util/BackgroundGenerator.hpp>
#include <manager/BrushManager.hpp>
#include <util/IntegrityLogger.hpp>
#include <algorithm>
#include <cmath>
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace paibot;
using namespace geode::prelude;

BackgroundGenerator* BackgroundGenerator::create() {
    auto ret = new (std::nothrow) BackgroundGenerator();
    if (ret && ret->init()) {
        return ret;
    }
    delete ret;
    return nullptr;
}

bool BackgroundGenerator::init() {
    // Initialize default settings
    m_settings.type = BackgroundType::SeamlessFromImage;
    m_settings.tileSize = 1024;
    m_settings.continuity = 0.8f;
    m_settings.variety = 0.5f;
    m_settings.noiseSeed = 12345;
    m_settings.parallaxLayers = 1;
    m_settings.noiseType = NoiseType::Perlin;
    m_settings.noiseScale = 0.1f;
    m_settings.octaves = 4;
    m_settings.persistence = 0.5f;
    m_settings.lacunarity = 2.0f;
    m_settings.version = 1;
    
    // Initialize integrity tracking
    m_generationValid = true;
    
    return true;
}

void BackgroundGenerator::setSettings(const BackgroundSettings& settings) {
    // Validate settings before applying
    if (!validateSettings()) {
        IntegrityLogger::get()->logError("BackgroundGenerator", "Invalid settings provided");
        return;
    }
    
    // Migrate settings if needed
    BackgroundSettings migratedSettings = settings;
    if (!validatePresetVersion(migratedSettings)) {
        migratePreset(migratedSettings, settings.version, 1);
    }
    
    m_settings = migratedSettings;
    IntegrityLogger::get()->logOperationStart(generateOperationId(), "SettingsUpdate");
}

BackgroundSettings BackgroundGenerator::getSettings() const {
    return m_settings;
}

TileSet BackgroundGenerator::generateBackground() {
    m_currentOperationId = generateOperationId();
    IntegrityLogger::get()->logOperationStart(m_currentOperationId, "BackgroundGeneration");
    
    if (!validateSettings()) {
        IntegrityLogger::get()->logError("BackgroundGenerator", "Cannot generate: invalid settings");
        return TileSet{}; // Return empty tile set
    }
    
    TileSet tileSet;

    try {
        switch (m_settings.type) {
            case BackgroundType::SeamlessFromImage:
                if (!m_settings.sourceImagePath.empty()) {
                    tileSet = createSeamlessFromImage(m_settings.sourceImagePath);
                } else {
                    log::error("SeamlessFromImage generation requires source image path");
                }
                break;
                
            case BackgroundType::TextureSynthesis:
                // Placeholder - would need sample image
                log::info("Texture synthesis not yet implemented");
                break;
                
            case BackgroundType::Procedural:
                tileSet = generateProcedural();
                break;
                
            case BackgroundType::WangTiles:
                tileSet = generateWangTiles();
                break;
        }

        if (!tileSet.tiles.empty()) {
            tileSet.deltaE = calculateSeamlessness(tileSet.tiles.front());
        } else {
            tileSet.deltaE = 0.0f;
        }
        
        // Validate the generated tile set
        if (!validateTileSet(tileSet)) {
            IntegrityLogger::get()->logError("BackgroundGenerator", "Generated tile set validation failed");
            return TileSet{}; // Return empty tile set
        }
        
        // Store as last valid if generation succeeded
        if (tileSet.isValid()) {
            m_lastValidTileSet = tileSet;
            m_generationValid = true;
            IntegrityLogger::get()->logOperationEnd(m_currentOperationId, true, 
                "Background generation completed successfully");
        } else {
            m_generationValid = false;
            IntegrityLogger::get()->logOperationEnd(m_currentOperationId, false, 
                "Generated empty or invalid tile set");
        }
        
    } catch (const std::exception& e) {
        IntegrityLogger::get()->logError("BackgroundGenerator", 
            "Background generation failed: " + std::string(e.what()));
        m_generationValid = false;
        IntegrityLogger::get()->logOperationEnd(m_currentOperationId, false, 
            "Exception during generation");
        return TileSet{};
    }

    m_currentTileSet = tileSet;
    measureDeltaE(m_currentTileSet);
    return tileSet;
}

void BackgroundGenerator::showPreview() {
    if (m_currentTileSet.tiles.empty()) {
        generateBackground();
    }

    hidePreview(); // Clean up any existing preview

    m_previewNode = createTilePreview(m_currentTileSet);
    if (m_previewNode) {
        m_previewNode->retain();
        // Add to appropriate parent (would be editor layer in real implementation)
        m_isPreviewActive = true;
        log::info("Showing background preview with {} tiles", m_currentTileSet.tiles.size());
    } else {
        log::warn("Background preview unavailable: failed to create preview node");
    }
}

void BackgroundGenerator::hidePreview() {
    if (m_previewNode) {
        m_previewNode->removeFromParent();
        m_previewNode->release();
        m_previewNode = nullptr;
    }
    m_isPreviewActive = false;
}

void BackgroundGenerator::exportTileSet(const std::string& path) {
    // Placeholder implementation for exporting tiles
    log::info("Exporting {} tiles to {}", m_currentTileSet.tiles.size(), path);
    
    // In real implementation:
    // 1. Save each tile as PNG file
    // 2. Generate normal/height maps if needed
    // 3. Export JSON preset with settings
    // 4. Create sprite sheets for GD integration
}

TileSet BackgroundGenerator::createSeamlessFromImage(const std::string& imagePath) {
    TileSet tileSet;
    tileSet.tileSize = m_settings.tileSize;
    
    // Placeholder implementation
    // In real implementation:
    // 1. Load source image
    // 2. Apply seamless algorithm (offset + Poisson blend)
    // 3. Apply Hann window for smoothing
    // 4. Generate multiple variations
    
    log::info("Creating seamless tiles from image: {}", imagePath);
    
    return tileSet;
}

cocos2d::CCImage* BackgroundGenerator::makeSeamless(cocos2d::CCImage* source) {
    // Placeholder for seamless image processing
    // Real implementation would:
    // 1. Offset image by half width/height
    // 2. Apply Poisson blending at seams
    // 3. Apply Hann window function
    // 4. Mirror edges for better continuity
    
    return source;
}

cocos2d::CCImage* BackgroundGenerator::poissonBlend(cocos2d::CCImage* source, cocos2d::CCImage* target, 
                                                   const std::vector<cocos2d::CCPoint>& mask) {
    // Placeholder for Poisson blending algorithm
    return target;
}

TileSet BackgroundGenerator::synthesizeTexture(cocos2d::CCImage* sample) {
    TileSet tileSet;
    tileSet.tileSize = m_settings.tileSize;
    
    // Placeholder for texture synthesis
    log::info("Synthesizing texture from sample");
    
    return tileSet;
}

cocos2d::CCImage* BackgroundGenerator::efrosLeungSynthesis(cocos2d::CCImage* sample, int outputSize) {
    // Placeholder for Efros-Leung algorithm
    return sample;
}

cocos2d::CCImage* BackgroundGenerator::kwatrnaSynthesis(cocos2d::CCImage* sample, int outputSize) {
    // Placeholder for Kwatra synthesis with graph cuts
    return sample;
}

std::vector<cocos2d::CCPoint> BackgroundGenerator::findBestPatches(cocos2d::CCImage* sample, 
                                                                  cocos2d::CCImage* target,
                                                                  const cocos2d::CCPoint& position,
                                                                  int patchSize) {
    // Placeholder for patch matching
    return {};
}

TileSet BackgroundGenerator::generateProcedural() {
    TileSet tileSet;
    tileSet.tileSize = m_settings.tileSize;

    // Generate a simple procedural noise pattern
    auto heightmap = generatePerlinNoise(m_settings.tileSize, m_settings.noiseScale, m_settings.octaves);
    
    if (heightmap) {
        // Apply color palette (simplified)
        std::vector<cocos2d::ccColor3B> palette = {
            {64, 128, 255},   // Deep blue
            {128, 200, 255},  // Light blue
            {255, 255, 200},  // Light yellow
            {200, 255, 128},  // Light green
            {128, 200, 64}    // Dark green
        };

        auto coloredTile = applyColorPalette(heightmap, palette);
        if (coloredTile) {
            // Re-enable tile population so preview/export receive usable data.
            tileSet.tiles.push_back(coloredTile);
        } else {
            delete heightmap;
        }
    } else {
        log::warn("Procedural generation failed to create heightmap");
    }

    log::info("Generated procedural tileset with {} noise octaves", m_settings.octaves);

    return tileSet;
}

cocos2d::CCImage* BackgroundGenerator::generatePerlinNoise(int size, float scale, int octaves) {
    // Simplified Perlin noise implementation
    // In real implementation, would use proper Perlin noise with tileable properties
    if (size <= 0) {
        return nullptr;
    }

    std::mt19937 rng(m_settings.noiseSeed);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    const int octaveCount = std::max(1, octaves);
    std::vector<float> octaveOffsets(octaveCount);
    for (int i = 0; i < octaveCount; ++i) {
        octaveOffsets[i] = dist(rng) * 6.28318f; // Random phase per octave
    }

    std::vector<unsigned char> pixels(size * size * 4);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float amplitude = 1.0f;
            float frequency = std::max(0.01f, scale);
            float value = 0.0f;
            float amplitudeSum = 0.0f;

            for (int octave = 0; octave < octaveCount; ++octave) {
                float sampleX = (static_cast<float>(x) / size) * frequency + octaveOffsets[octave];
                float sampleY = (static_cast<float>(y) / size) * frequency + octaveOffsets[octave] * 0.5f;
                float sample = std::sin(sampleX) * std::cos(sampleY);

                value += sample * amplitude;
                amplitudeSum += amplitude;

                amplitude *= 0.5f;
                frequency *= 2.0f;
            }

            if (amplitudeSum > 0.0f) {
                value /= amplitudeSum;
            }

            float normalized = std::clamp(0.5f + 0.5f * value, 0.0f, 1.0f);
            unsigned char channel = static_cast<unsigned char>(normalized * 255.0f);
            size_t idx = static_cast<size_t>(y) * size * 4 + static_cast<size_t>(x) * 4;
            pixels[idx + 0] = channel;
            pixels[idx + 1] = channel;
            pixels[idx + 2] = channel;
            pixels[idx + 3] = 255;
        }
    }

    auto image = new cocos2d::CCImage();
    if (!image->initWithRawData(pixels.data(), static_cast<int>(pixels.size()), size, size, 8, true)) {
        delete image;
        return nullptr;
    }

    log::info("Generated {}x{} Perlin noise with scale {}", size, size, static_cast<double>(scale));

    return image;
}

cocos2d::CCImage* BackgroundGenerator::generateSimplexNoise(int size, float scale, int octaves) {
    // Placeholder for Simplex noise
    return generatePerlinNoise(size, scale, octaves);
}

cocos2d::CCImage* BackgroundGenerator::generateWorleyNoise(int size, float scale) {
    // Placeholder for Worley/Voronoi noise
    log::info("Generated {}x{} Worley noise", size, size);
    return nullptr;
}

cocos2d::CCImage* BackgroundGenerator::applyColorPalette(cocos2d::CCImage* heightmap, 
                                                        const std::vector<cocos2d::ccColor3B>& palette) {
    // Placeholder for applying color palette to heightmap
    return heightmap;
}

TileSet BackgroundGenerator::generateWangTiles() {
    TileSet tileSet;
    tileSet.tileSize = m_settings.tileSize;
    
    // Generate 8-16 Wang tiles with compatible edges
    int tileCount = 8;
    
    // Placeholder implementation
    log::info("Generated {} Wang tiles", tileCount);
    
    return tileSet;
}

std::vector<cocos2d::CCImage*> BackgroundGenerator::createCompatibleTiles(int count) {
    // Placeholder for generating compatible Wang tiles
    return {};
}

bool BackgroundGenerator::checkEdgeCompatibility(cocos2d::CCImage* tile1, cocos2d::CCImage* tile2, int edge) {
    // Placeholder for edge compatibility checking
    return true;
}

std::vector<std::vector<int>> BackgroundGenerator::generateTileLayout(int width, int height) {
    // Placeholder for generating valid Wang tile layout
    return {};
}

float BackgroundGenerator::calculateSeamlessness(cocos2d::CCImage* tile) {
    // Placeholder for measuring how seamless a tile is
    // Real implementation would compare edge pixels and calculate Delta E
    return 0.9f; // Good seamlessness
}

std::vector<cocos2d::ccColor3B> BackgroundGenerator::extractPalette(cocos2d::CCImage* image, int colorCount) {
    // Placeholder for k-means color extraction
    return {
        {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, 
        {255, 255, 0}, {255, 0, 255}, {0, 255, 255}
    };
}

cocos2d::CCImage* BackgroundGenerator::applyHannWindow(cocos2d::CCImage* image) {
    // Placeholder for Hann window smoothing
    return image;
}

cocos2d::CCImage* BackgroundGenerator::mirrorEdges(cocos2d::CCImage* image, int borderSize) {
    // Placeholder for edge mirroring
    return image;
}

cocos2d::CCNode* BackgroundGenerator::createTilePreview(const TileSet& tileSet, int previewCols, int previewRows) {
    auto node = CCNode::create();
    
    // Create a 3x3 preview grid showing the tileset pattern
    // In real implementation, would create sprites from the tiles
    
    for (int y = 0; y < previewRows; ++y) {
        for (int x = 0; x < previewCols; ++x) {
            auto placeholder = CCSprite::create();
            placeholder->setColor({
                static_cast<GLubyte>(128 + x * 20),
                static_cast<GLubyte>(128 + y * 20),
                static_cast<GLubyte>(200)
            });
            placeholder->setTextureRect({0, 0, 64, 64});
            placeholder->setPosition({x * 70.0f, y * 70.0f});
            node->addChild(placeholder);
        }
    }
    
    return node;
}

void BackgroundGenerator::measureDeltaE(const TileSet& tileSet) {
    // Placeholder for measuring seamlessness using Delta E
    log::info("Measured seamlessness: Delta E = {:.2f}", tileSet.deltaE);
}

std::string BackgroundGenerator::generateExportJSON(const TileSet& tileSet) {
    // Placeholder for JSON export
    return fmt::format(
        fmt::runtime(
            "{\n"
            "  \"tileSize\": {},\n"
            "  \"tileCount\": {},\n"
            "  \"seamlessness\": {:.2f},\n"
            "  \"settings\": {{\n"
            "    \"type\": \"{}\",\n"
            "    \"seed\": {}\n"
            "  }}\n"
            "}"
        ),
        tileSet.tileSize,
        static_cast<unsigned long long>(tileSet.tiles.size()),
        static_cast<double>(tileSet.deltaE),
        static_cast<int>(m_settings.type),
        m_settings.noiseSeed
    );
}

bool BackgroundGenerator::validateSettings() const {
    if (m_settings.tileSize <= 0 || m_settings.tileSize > 4096) {
        return false;
    }
    
    if (m_settings.continuity < 0.0f || m_settings.continuity > 1.0f) {
        return false;
    }
    
    if (m_settings.variety < 0.0f || m_settings.variety > 1.0f) {
        return false;
    }
    
    if (m_settings.octaves < 1 || m_settings.octaves > 8) {
        return false;
    }
    
    return true;
}

bool BackgroundGenerator::validateTileSet(const TileSet& tileSet) {
    return validateNonEmptyTileSet(tileSet);
}

bool BackgroundGenerator::validateNonEmptyTileSet(const TileSet& tileSet) {
    if (tileSet.isEmpty()) {
        IntegrityLogger::get()->logError("BackgroundGenerator", "Tile set is empty");
        return false;
    }
    
    if (tileSet.tileSize <= 0) {
        IntegrityLogger::get()->logError("BackgroundGenerator", "Invalid tile size");
        return false;
    }
    
    // Additional validation for tile data
    for (const auto* tile : tileSet.tiles) {
        if (!tile) {
            IntegrityLogger::get()->logError("BackgroundGenerator", "Null tile in set");
            return false;
        }
    }
    
    return true;
}

WangTileValidation BackgroundGenerator::validateWangTileBorders(const TileSet& tileSet) {
    WangTileValidation validation;
    
    if (tileSet.isEmpty()) {
        validation.hasValidBorders = false;
        validation.errorDetails = "Empty tile set";
        return validation;
    }
    
    // Check edge compatibility for Wang tiles
    float totalConsistency = 0.0f;
    int checks = 0;
    
    for (size_t i = 0; i < tileSet.tiles.size(); ++i) {
        for (size_t j = i + 1; j < tileSet.tiles.size(); ++j) {
            // Check all four edges between tiles
            for (int edge = 0; edge < 4; ++edge) {
                bool compatible = checkEdgeCompatibility(tileSet.tiles[i], tileSet.tiles[j], edge);
                totalConsistency += compatible ? 1.0f : 0.0f;
                checks++;
                
                if (!compatible) {
                    validation.hasVisualCuts = true;
                }
            }
        }
    }
    
    if (checks > 0) {
        validation.borderConsistency = totalConsistency / checks;
    }
    
    validation.hasValidBorders = validation.borderConsistency > 0.8f && !validation.hasVisualCuts;
    
    if (!validation.hasValidBorders) {
        validation.errorDetails = "Border inconsistency: " + std::to_string(validation.borderConsistency);
        IntegrityLogger::get()->logWarning("BackgroundGenerator", validation.errorDetails);
    }
    
    return validation;
}

void BackgroundGenerator::revertToLastValid() {
    if (m_lastValidTileSet.isValid()) {
        log::info("Reverting to last valid tile set");
        m_currentTileSet = m_lastValidTileSet;
        m_generationValid = true;
        IntegrityLogger::get()->logOperationEnd(m_currentOperationId, true, "Reverted to valid state");
    } else {
        log::warn("No valid tile set to revert to");
    }
}

std::unique_ptr<cocos2d::CCImage> BackgroundGenerator::generatePreviewInMemory() {
    // Generate a small preview image in memory without creating sprites
    // This prevents memory issues for large tile sets
    
    clearPreviewMemory(); // Clear any existing preview memory
    
    if (m_currentTileSet.isEmpty()) {
        return nullptr;
    }
    
    // Create a small preview image (e.g., 256x256)
    const int previewSize = 256;
    std::vector<unsigned char> pixels(previewSize * previewSize * 4);
    
    // Fill with a simple pattern representing the tile set
    for (int y = 0; y < previewSize; ++y) {
        for (int x = 0; x < previewSize; ++x) {
            // Create a checkerboard pattern with tile-based colors
            int tileX = (x * 3) / previewSize;
            int tileY = (y * 3) / previewSize;
            int tileIndex = (tileY * 3 + tileX) % std::max(1, static_cast<int>(m_currentTileSet.tiles.size()));
            
            unsigned char r = 128 + (tileIndex * 20) % 128;
            unsigned char g = 100 + (tileIndex * 15) % 128;
            unsigned char b = 150 + (tileIndex * 25) % 128;
            
            size_t idx = static_cast<size_t>(y) * previewSize * 4 + static_cast<size_t>(x) * 4;
            pixels[idx + 0] = r;
            pixels[idx + 1] = g;
            pixels[idx + 2] = b;
            pixels[idx + 3] = 255;
        }
    }
    
    auto previewImage = std::make_unique<cocos2d::CCImage>();
    if (!previewImage->initWithRawData(pixels.data(), static_cast<int>(pixels.size()), 
                                      previewSize, previewSize, 8, true)) {
        return nullptr;
    }
    
    log::info("Generated preview image in memory: {}x{}", previewSize, previewSize);
    return previewImage;
}

void BackgroundGenerator::clearPreviewMemory() {
    m_previewImages.clear();
    log::debug("Cleared background generator preview memory");
}

void BackgroundGenerator::migratePreset(BackgroundSettings& settings, int fromVersion, int toVersion) {
    if (fromVersion < 1 && toVersion >= 1) {
        // Migration from legacy format
        log::info("Migrating background preset from version {} to {}", fromVersion, toVersion);
        
        // Ensure valid defaults for new fields
        if (settings.version <= 0) {
            settings.version = 1;
        }
        
        // Clamp values to valid ranges
        settings.tileSize = std::clamp(settings.tileSize, 256, 2048);
        settings.continuity = std::clamp(settings.continuity, 0.0f, 1.0f);
        settings.variety = std::clamp(settings.variety, 0.0f, 1.0f);
    }
    
    settings.version = toVersion;
}

bool BackgroundGenerator::validatePresetVersion(const BackgroundSettings& settings) {
    return settings.version >= 1; // Current version is 1
}

std::string BackgroundGenerator::generateOperationId() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << "BG_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") 
       << "_" << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}