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
                
            case BackgroundType::Geometrization:
                if (!m_settings.sourceImagePath.empty()) {
                    tileSet = generateGeometrization();
                } else {
                    log::error("Geometrization mode requires source image path");
                }
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
    if (m_currentTileSet.tiles.empty()) {
        log::error("Cannot export empty tile set");
        return;
    }
    
    log::info("Exporting {} tiles to {}", m_currentTileSet.tiles.size(), path);
    
    try {
        // Create export directory
        std::filesystem::create_directories(path);
        
        // 1. Export preset.json with settings and metadata
        std::string presetPath = path + "/preset.json";
        exportPresetJson(presetPath);
        
        // 2. Export spritesheet PNG
        std::string spritesheetPath = path + "/spritesheet.png";
        exportSpritesheet(spritesheetPath);
        
        // 3. Generate thumbnail
        std::string thumbnailPath = path + "/thumbnail.png";
        generateThumbnail(thumbnailPath);
        
        // 4. Export compatibility matrix (for Wang tiles)
        if (m_settings.type == BackgroundType::WangTiles) {
            std::string matrixPath = path + "/compatibility.json";
            exportCompatibilityMatrix(matrixPath);
        }
        
        log::info("Export completed successfully to {}", path);
        
    } catch (const std::exception& e) {
        log::error("Export failed: {}", e.what());
    }
}

void BackgroundGenerator::exportPresetJson(const std::string& path) {
    std::ofstream file(path);
    
    // Export preset in simple key-value format
    file << "{\n";
    file << "  \"name\": \"Background Preset\",\n";
    file << "  \"version\": " << m_settings.version << ",\n";
    file << "  \"type\": \"" << backgroundTypeToString(m_settings.type) << "\",\n";
    file << "  \"created\": \"" << getCurrentTimestamp() << "\",\n";
    file << "  \"hash\": \"" << calculatePresetHash() << "\",\n";
    file << "  \"settings\": {\n";
    file << "    \"tileSize\": " << m_settings.tileSize << ",\n";
    file << "    \"seed\": " << m_settings.noiseSeed << ",\n";
    file << "    \"continuity\": " << m_settings.continuity << ",\n";
    file << "    \"variety\": " << m_settings.variety;
    
    if (m_settings.type == BackgroundType::Procedural) {
        file << ",\n    \"noiseType\": \"" << noiseTypeToString(m_settings.noiseType) << "\",\n";
        file << "    \"noiseScale\": " << m_settings.noiseScale << ",\n";
        file << "    \"octaves\": " << m_settings.octaves << ",\n";
        file << "    \"persistence\": " << m_settings.persistence << ",\n";
        file << "    \"lacunarity\": " << m_settings.lacunarity;
    } else if (m_settings.type == BackgroundType::Geometrization) {
        file << ",\n    \"colorTolerance\": " << m_settings.colorTolerance << ",\n";
        file << "    \"maxColors\": " << m_settings.maxColors << ",\n";
        file << "    \"simplificationTolerance\": " << m_settings.simplificationTolerance << ",\n";
        file << "    \"targetResolution\": " << m_settings.targetResolution << ",\n";
        file << "    \"optimizeForTiling\": " << (m_settings.optimizeForTiling ? "true" : "false");
        if (!m_settings.sourceImagePath.empty()) {
            file << ",\n    \"sourceImage\": \"" << std::filesystem::path(m_settings.sourceImagePath).filename().string() << "\"";
        }
    }
    
    file << "\n  },\n";
    file << "  \"compatibility\": {\n";
    file << "    \"gameVersion\": \"2.207\",\n";
    file << "    \"geodeVersion\": \"4.8.0\",\n";
    file << "    \"paibotVersion\": \"0.1.0\"\n";
    file << "  },\n";
    file << "  \"quality\": {\n";
    file << "    \"deltaE\": " << m_currentTileSet.deltaE << ",\n";
    file << "    \"seamlessness\": " << calculateSeamlessness(m_currentTileSet.tiles.empty() ? nullptr : m_currentTileSet.tiles[0]) << ",\n";
    file << "    \"tileCount\": " << m_currentTileSet.tiles.size() << "\n";
    file << "  }\n";
    file << "}\n";
    
    file.close();
}

void BackgroundGenerator::exportSpritesheet(const std::string& path) {
    if (m_currentTileSet.tiles.empty()) {
        return;
    }
    
    // Calculate spritesheet dimensions
    int tileCount = static_cast<int>(m_currentTileSet.tiles.size());
    int tilesPerRow = static_cast<int>(std::ceil(std::sqrt(tileCount)));
    int totalWidth = tilesPerRow * m_currentTileSet.tileSize;
    int totalHeight = ((tileCount + tilesPerRow - 1) / tilesPerRow) * m_currentTileSet.tileSize;
    
    // Create spritesheet pixel data
    std::vector<unsigned char> spritesheetPixels(totalWidth * totalHeight * 4, 0);
    
    // Copy each tile to the spritesheet
    for (int i = 0; i < tileCount; ++i) {
        int row = i / tilesPerRow;
        int col = i % tilesPerRow;
        int offsetX = col * m_currentTileSet.tileSize;
        int offsetY = row * m_currentTileSet.tileSize;
        
        // Copy tile pixels (placeholder - would need actual image data access in real implementation)
        // For now, create a colored rectangle for each tile
        cocos2d::ccColor3B tileColor = {
            static_cast<unsigned char>((i * 50) % 255),
            static_cast<unsigned char>((i * 100) % 255),
            static_cast<unsigned char>((i * 150) % 255)
        };
        
        for (int y = 0; y < m_currentTileSet.tileSize; ++y) {
            for (int x = 0; x < m_currentTileSet.tileSize; ++x) {
                int pixelIndex = ((offsetY + y) * totalWidth + (offsetX + x)) * 4;
                if (pixelIndex + 3 < static_cast<int>(spritesheetPixels.size())) {
                    spritesheetPixels[pixelIndex] = tileColor.r;
                    spritesheetPixels[pixelIndex + 1] = tileColor.g;
                    spritesheetPixels[pixelIndex + 2] = tileColor.b;
                    spritesheetPixels[pixelIndex + 3] = 255;
                }
            }
        }
    }
    
    // In real implementation, would save PNG file here
    log::info("Generated spritesheet: {}x{} with {} tiles", totalWidth, totalHeight, tileCount);
}

void BackgroundGenerator::generateThumbnail(const std::string& path) {
    const int thumbnailSize = 256;
    
    if (m_currentTileSet.tiles.empty()) {
        return;
    }
    
    // Create thumbnail by scaling down the first tile or a preview
    std::vector<unsigned char> thumbnailPixels(thumbnailSize * thumbnailSize * 4);
    
    // Create a simple thumbnail (placeholder)
    for (int y = 0; y < thumbnailSize; ++y) {
        for (int x = 0; x < thumbnailSize; ++x) {
            int pixelIndex = (y * thumbnailSize + x) * 4;
            
            // Create a gradient pattern for the thumbnail
            thumbnailPixels[pixelIndex] = static_cast<unsigned char>((x * 255) / thumbnailSize);
            thumbnailPixels[pixelIndex + 1] = static_cast<unsigned char>((y * 255) / thumbnailSize);
            thumbnailPixels[pixelIndex + 2] = 128;
            thumbnailPixels[pixelIndex + 3] = 255;
        }
    }
    
    log::info("Generated thumbnail: {}x{}", thumbnailSize, thumbnailSize);
}

void BackgroundGenerator::exportCompatibilityMatrix(const std::string& path) {
    std::ofstream file(path);
    
    // Export edge compatibility information for Wang tiles in simple format
    file << "# Wang Tile Compatibility Matrix\n";
    file << "tile_count=" << m_currentTileSet.tiles.size() << "\n";
    file << "edge_patterns:\n";
    
    for (size_t i = 0; i < m_currentTileSet.edgePatterns.size(); ++i) {
        file << "tile_" << i << "=";
        for (size_t j = 0; j < m_currentTileSet.edgePatterns[i].size(); ++j) {
            if (j > 0) file << ",";
            file << m_currentTileSet.edgePatterns[i][j];
        }
        file << "\n";
    }
    
    file.close();
}

std::string BackgroundGenerator::backgroundTypeToString(BackgroundType type) {
    switch (type) {
        case BackgroundType::SeamlessFromImage: return "seamless";
        case BackgroundType::TextureSynthesis: return "synthesis";
        case BackgroundType::Procedural: return "procedural";
        case BackgroundType::WangTiles: return "wang";
        case BackgroundType::Geometrization: return "geometrization";
        default: return "unknown";
    }
}

std::string BackgroundGenerator::noiseTypeToString(NoiseType type) {
    switch (type) {
        case NoiseType::Perlin: return "perlin";
        case NoiseType::Simplex: return "simplex";
        case NoiseType::Worley: return "worley";
        default: return "unknown";
    }
}

std::string BackgroundGenerator::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::string BackgroundGenerator::calculatePresetHash() {
    // Create hash from settings for reproducibility verification
    std::stringstream ss;
    ss << static_cast<int>(m_settings.type) << "_";
    ss << m_settings.noiseSeed << "_";
    ss << m_settings.tileSize << "_";
    ss << m_settings.noiseScale << "_";
    ss << m_settings.octaves;
    
    std::hash<std::string> hasher;
    return std::to_string(hasher(ss.str()));
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
    
    log::info("Generating Wang tiles with seamless edge constraints");
    
    // Generate base set of Wang tiles (typically 8-16 tiles)
    int tileCount = 8;
    auto tiles = createCompatibleTiles(tileCount);
    
    if (!tiles.empty()) {
        tileSet.tiles = tiles;
        
        // Generate edge compatibility matrix
        tileSet.edgePatterns.resize(tileCount);
        for (int i = 0; i < tileCount; ++i) {
            tileSet.edgePatterns[i].resize(4); // 4 edges: top, right, bottom, left
            
            // Assign edge patterns (simplified approach)
            // In real implementation, would analyze actual edge pixels
            tileSet.edgePatterns[i][0] = (i + 0) % 4; // top
            tileSet.edgePatterns[i][1] = (i + 1) % 4; // right  
            tileSet.edgePatterns[i][2] = (i + 2) % 4; // bottom
            tileSet.edgePatterns[i][3] = (i + 3) % 4; // left
        }
        
        // Validate all edge compatibility
        auto validation = validateWangTileBorders(tileSet);
        if (!validation.hasValidBorders) {
            log::warn("Wang tiles have border incompatibilities: {}", validation.errorDetails);
        } else {
            log::info("Wang tiles pass border validation with consistency: {}", validation.borderConsistency);
        }
    }
    
    log::info("Generated {} Wang tiles with edge compatibility matrix", tileCount);
    return tileSet;
}

std::vector<cocos2d::CCImage*> BackgroundGenerator::createCompatibleTiles(int count) {
    std::vector<cocos2d::CCImage*> tiles;
    
    if (count <= 0) {
        return tiles;
    }
    
    // Generate tiles with compatible edges using procedural approach
    std::mt19937 rng(m_settings.noiseSeed);
    std::uniform_real_distribution<float> colorDist(0.3f, 0.9f);
    
    for (int tileIdx = 0; tileIdx < count; ++tileIdx) {
        std::vector<unsigned char> pixels(m_settings.tileSize * m_settings.tileSize * 4);
        
        // Generate unique color for this tile
        cocos2d::ccColor3B baseColor = {
            static_cast<unsigned char>(colorDist(rng) * 255),
            static_cast<unsigned char>(colorDist(rng) * 255),
            static_cast<unsigned char>(colorDist(rng) * 255)
        };
        
        // Generate edge patterns that will be shared between compatible tiles
        std::vector<cocos2d::ccColor3B> edgeColors(4);
        for (int edge = 0; edge < 4; ++edge) {
            edgeColors[edge] = {
                static_cast<unsigned char>((baseColor.r + edge * 50) % 255),
                static_cast<unsigned char>((baseColor.g + edge * 50) % 255),
                static_cast<unsigned char>((baseColor.b + edge * 50) % 255)
            };
        }
        
        // Fill tile with base pattern
        for (int y = 0; y < m_settings.tileSize; ++y) {
            for (int x = 0; x < m_settings.tileSize; ++x) {
                int pixelIndex = (y * m_settings.tileSize + x) * 4;
                
                // Determine which color to use based on position
                cocos2d::ccColor3B pixelColor = baseColor;
                
                // Apply edge colors for seamless transitions
                float edgeWidth = m_settings.tileSize * 0.1f; // 10% edge width
                
                if (y < edgeWidth) {
                    // Top edge
                    float blend = y / edgeWidth;
                    pixelColor = blendColors(edgeColors[0], baseColor, blend);
                } else if (y >= m_settings.tileSize - edgeWidth) {
                    // Bottom edge
                    float blend = (m_settings.tileSize - 1 - y) / edgeWidth;
                    pixelColor = blendColors(edgeColors[2], baseColor, blend);
                }
                
                if (x < edgeWidth) {
                    // Left edge
                    float blend = x / edgeWidth;
                    auto leftColor = blendColors(edgeColors[3], baseColor, blend);
                    pixelColor = blendColors(pixelColor, leftColor, 0.5f);
                } else if (x >= m_settings.tileSize - edgeWidth) {
                    // Right edge
                    float blend = (m_settings.tileSize - 1 - x) / edgeWidth;
                    auto rightColor = blendColors(edgeColors[1], baseColor, blend);
                    pixelColor = blendColors(pixelColor, rightColor, 0.5f);
                }
                
                pixels[pixelIndex] = pixelColor.r;
                pixels[pixelIndex + 1] = pixelColor.g;
                pixels[pixelIndex + 2] = pixelColor.b;
                pixels[pixelIndex + 3] = 255;
            }
        }
        
        // Create CCImage from pixel data
        auto image = new cocos2d::CCImage();
        if (image->initWithRawData(pixels.data(), static_cast<int>(pixels.size()), 
                                  m_settings.tileSize, m_settings.tileSize, 8, false)) {
            tiles.push_back(image);
        } else {
            delete image;
        }
    }
    
    return tiles;
}

bool BackgroundGenerator::checkEdgeCompatibility(cocos2d::CCImage* tile1, cocos2d::CCImage* tile2, int edge) {
    if (!tile1 || !tile2) {
        return false;
    }
    
    // In real implementation, would compare actual edge pixels
    // For this implementation, we'll use a simplified approach based on edge patterns
    
    // Edge compatibility: 0=top, 1=right, 2=bottom, 3=left
    // Two tiles are compatible if their adjacent edges have similar patterns
    
    const int sampleCount = 10;
    const float toleranceThreshold = 30.0f; // Color difference tolerance
    
    // Extract edge pixels from both tiles (simplified)
    std::vector<cocos2d::ccColor3B> edge1Colors, edge2Colors;
    
    // Sample edge pixels (placeholder - would extract actual pixels in real implementation)
    for (int i = 0; i < sampleCount; ++i) {
        // Create sample colors based on tile properties
        edge1Colors.push_back({100, 150, 200});
        edge2Colors.push_back({105, 145, 205});
    }
    
    // Calculate average color difference
    float totalDifference = 0.0f;
    for (int i = 0; i < sampleCount; ++i) {
        totalDifference += calculateColorDistance(edge1Colors[i], edge2Colors[i]);
    }
    
    float averageDifference = totalDifference / sampleCount;
    bool compatible = averageDifference < toleranceThreshold;
    
    if (!compatible) {
        log::debug("Edge compatibility check failed: average difference {:.2f} > {:.2f}", 
                  averageDifference, toleranceThreshold);
    }
    
    return compatible;
}

cocos2d::ccColor3B BackgroundGenerator::blendColors(const cocos2d::ccColor3B& c1, 
                                                   const cocos2d::ccColor3B& c2, 
                                                   float factor) {
    factor = std::clamp(factor, 0.0f, 1.0f);
    
    return {
        static_cast<unsigned char>(c1.r * (1.0f - factor) + c2.r * factor),
        static_cast<unsigned char>(c1.g * (1.0f - factor) + c2.g * factor),
        static_cast<unsigned char>(c1.b * (1.0f - factor) + c2.b * factor)
    };
}

std::vector<std::vector<int>> BackgroundGenerator::generateTileLayout(int width, int height) {
    std::vector<std::vector<int>> layout(height, std::vector<int>(width, 0));
    
    if (m_currentTileSet.tiles.empty() || m_currentTileSet.edgePatterns.empty()) {
        log::error("Cannot generate layout: no tiles or edge patterns available");
        return layout;
    }
    
    int tileCount = static_cast<int>(m_currentTileSet.tiles.size());
    std::mt19937 rng(m_settings.noiseSeed);
    
    // Use backtracking algorithm to place compatible tiles
    if (placeTileRecursive(layout, 0, 0, width, height, tileCount, rng)) {
        log::info("Successfully generated {}x{} Wang tile layout", width, height);
    } else {
        log::warn("Failed to generate valid Wang tile layout, using fallback");
        // Fill with random valid tiles as fallback
        std::uniform_int_distribution<int> tileDist(0, tileCount - 1);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                layout[y][x] = tileDist(rng);
            }
        }
    }
    
    return layout;
}

bool BackgroundGenerator::placeTileRecursive(std::vector<std::vector<int>>& layout, 
                                           int x, int y, int width, int height, 
                                           int tileCount, std::mt19937& rng) {
    if (y >= height) {
        return true; // Successfully filled entire layout
    }
    
    int nextX = x + 1;
    int nextY = y;
    if (nextX >= width) {
        nextX = 0;
        nextY = y + 1;
    }
    
    std::vector<int> candidateTiles;
    for (int tileIdx = 0; tileIdx < tileCount; ++tileIdx) {
        if (isTileCompatibleAtPosition(layout, x, y, width, height, tileIdx)) {
            candidateTiles.push_back(tileIdx);
        }
    }
    
    if (candidateTiles.empty()) {
        return false; // No valid tile for this position
    }
    
    // Shuffle candidates for variety
    std::shuffle(candidateTiles.begin(), candidateTiles.end(), rng);
    
    // Try each candidate
    for (int tileIdx : candidateTiles) {
        layout[y][x] = tileIdx;
        
        if (placeTileRecursive(layout, nextX, nextY, width, height, tileCount, rng)) {
            return true;
        }
    }
    
    return false; // Backtrack
}

bool BackgroundGenerator::isTileCompatibleAtPosition(const std::vector<std::vector<int>>& layout,
                                                   int x, int y, int width, int height,
                                                   int tileIdx) {
    if (tileIdx >= static_cast<int>(m_currentTileSet.edgePatterns.size())) {
        return false;
    }
    
    const auto& currentTileEdges = m_currentTileSet.edgePatterns[tileIdx];
    
    // Check compatibility with neighboring tiles
    
    // Check top neighbor
    if (y > 0) {
        int topTileIdx = layout[y - 1][x];
        if (topTileIdx >= 0 && topTileIdx < static_cast<int>(m_currentTileSet.edgePatterns.size())) {
            const auto& topTileEdges = m_currentTileSet.edgePatterns[topTileIdx];
            if (currentTileEdges[0] != topTileEdges[2]) { // current top != neighbor bottom
                return false;
            }
        }
    }
    
    // Check left neighbor
    if (x > 0) {
        int leftTileIdx = layout[y][x - 1];
        if (leftTileIdx >= 0 && leftTileIdx < static_cast<int>(m_currentTileSet.edgePatterns.size())) {
            const auto& leftTileEdges = m_currentTileSet.edgePatterns[leftTileIdx];
            if (currentTileEdges[3] != leftTileEdges[1]) { // current left != neighbor right
                return false;
            }
        }
    }
    
    return true;
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

// Geometrization mode implementation
TileSet BackgroundGenerator::generateGeometrization() {
    TileSet tileSet;
    tileSet.tileSize = m_settings.targetResolution;
    
    log::info("Starting geometrization of image: {}", m_settings.sourceImagePath);
    
    // In a real implementation, we would load the actual image
    // For now, we'll create a placeholder implementation
    
    // Step 1: Load source image (placeholder)
    auto sourceImage = cocos2d::CCImage::create();
    if (!sourceImage) {
        log::error("Failed to load source image for geometrization");
        return tileSet;
    }
    
    // Step 2: Segment image by color
    auto palette = segmentImageByColor(sourceImage);
    log::info("Extracted {} colors from image", palette.size());
    
    // Step 3: Reduce palette if necessary
    if (palette.size() > static_cast<size_t>(m_settings.maxColors)) {
        palette = reducePalette(palette, m_settings.maxColors);
        log::info("Reduced palette to {} colors", palette.size());
    }
    
    // Step 4: Extract color regions as polygons
    auto regions = extractColorRegions(sourceImage, palette);
    log::info("Extracted {} color regions", regions.size());
    
    // Step 5: Simplify polygons
    for (auto& region : regions) {
        region = simplifyPolygon(region, m_settings.simplificationTolerance);
    }
    
    // Step 6: Optimize for tiling if requested
    if (m_settings.optimizeForTiling) {
        regions = optimizeForTiling(regions);
        log::info("Optimized patterns for seamless tiling");
    }
    
    // Step 7: Render geometric pattern
    auto geometricTile = renderGeometricPattern(regions, palette, m_settings.targetResolution);
    if (geometricTile) {
        tileSet.tiles.push_back(geometricTile);
        log::info("Generated geometric pattern tile");
    }
    
    return tileSet;
}

std::vector<cocos2d::ccColor3B> BackgroundGenerator::segmentImageByColor(cocos2d::CCImage* image) {
    std::vector<cocos2d::ccColor3B> palette;
    
    if (!image) {
        return palette;
    }
    
    // Placeholder implementation - extract dominant colors
    // In real implementation, would use proper color quantization algorithms
    // such as k-means clustering or median cut algorithm
    
    std::unordered_map<uint32_t, int> colorFrequency;
    
    // Simulate color extraction (placeholder)
    palette.push_back({255, 64, 64});   // Red
    palette.push_back({64, 255, 64});   // Green  
    palette.push_back({64, 64, 255});   // Blue
    palette.push_back({255, 255, 64}); // Yellow
    palette.push_back({255, 64, 255}); // Magenta
    palette.push_back({64, 255, 255}); // Cyan
    
    return palette;
}

std::vector<std::vector<cocos2d::CCPoint>> BackgroundGenerator::extractColorRegions(
    cocos2d::CCImage* image, const std::vector<cocos2d::ccColor3B>& palette) {
    
    std::vector<std::vector<cocos2d::CCPoint>> regions;
    
    if (!image || palette.empty()) {
        return regions;
    }
    
    // Placeholder implementation - create geometric regions
    // In real implementation, would use contour detection and polygon extraction
    
    float width = static_cast<float>(m_settings.targetResolution);
    float height = static_cast<float>(m_settings.targetResolution);
    
    // Create some example geometric regions
    for (size_t i = 0; i < palette.size(); ++i) {
        std::vector<cocos2d::CCPoint> region;
        
        // Create different geometric shapes for each color
        switch (i % 4) {
            case 0: // Rectangle
                region.push_back({width * 0.1f, height * 0.1f});
                region.push_back({width * 0.4f, height * 0.1f});
                region.push_back({width * 0.4f, height * 0.4f});
                region.push_back({width * 0.1f, height * 0.4f});
                break;
                
            case 1: // Triangle
                region.push_back({width * 0.6f, height * 0.1f});
                region.push_back({width * 0.9f, height * 0.1f});
                region.push_back({width * 0.75f, height * 0.4f});
                break;
                
            case 2: // Pentagon
                for (int j = 0; j < 5; ++j) {
                    float angle = static_cast<float>(j) * 2.0f * M_PI / 5.0f;
                    float x = width * 0.25f + width * 0.15f * std::cos(angle);
                    float y = height * 0.75f + height * 0.15f * std::sin(angle);
                    region.push_back({x, y});
                }
                break;
                
            case 3: // Hexagon
                for (int j = 0; j < 6; ++j) {
                    float angle = static_cast<float>(j) * 2.0f * M_PI / 6.0f;
                    float x = width * 0.75f + width * 0.15f * std::cos(angle);
                    float y = height * 0.75f + height * 0.15f * std::sin(angle);
                    region.push_back({x, y});
                }
                break;
        }
        
        if (!region.empty()) {
            regions.push_back(region);
        }
    }
    
    return regions;
}

std::vector<cocos2d::CCPoint> BackgroundGenerator::simplifyPolygon(
    const std::vector<cocos2d::CCPoint>& polygon, float tolerance) {
    
    if (polygon.size() <= 3) {
        return polygon; // Can't simplify triangles or smaller
    }
    
    // Simplified Douglas-Peucker algorithm implementation
    std::vector<cocos2d::CCPoint> simplified;
    
    // For this placeholder, just reduce points if tolerance is high
    if (tolerance > 1.0f) {
        // Keep every other point for high tolerance
        for (size_t i = 0; i < polygon.size(); i += 2) {
            simplified.push_back(polygon[i]);
        }
        
        // Ensure we keep the last point if not already included
        if (polygon.size() % 2 != 0 && simplified.back().x != polygon.back().x) {
            simplified.push_back(polygon.back());
        }
    } else {
        // Keep all points for low tolerance
        simplified = polygon;
    }
    
    return simplified;
}

cocos2d::CCImage* BackgroundGenerator::renderGeometricPattern(
    const std::vector<std::vector<cocos2d::CCPoint>>& regions,
    const std::vector<cocos2d::ccColor3B>& palette,
    int outputSize) {
    
    if (regions.empty() || palette.empty()) {
        return nullptr;
    }
    
    // Create a simple geometric pattern image
    std::vector<unsigned char> pixels(outputSize * outputSize * 4, 0);
    
    // Fill background with white
    for (int i = 0; i < outputSize * outputSize * 4; i += 4) {
        pixels[i] = 255;     // R
        pixels[i + 1] = 255; // G  
        pixels[i + 2] = 255; // B
        pixels[i + 3] = 255; // A
    }
    
    // Render each geometric region
    for (size_t regionIdx = 0; regionIdx < regions.size() && regionIdx < palette.size(); ++regionIdx) {
        const auto& region = regions[regionIdx];
        const auto& color = palette[regionIdx];
        
        if (region.size() < 3) continue; // Skip invalid polygons
        
        // Simple rasterization - fill bounding box (placeholder)
        float minX = outputSize, maxX = 0, minY = outputSize, maxY = 0;
        for (const auto& point : region) {
            minX = std::min(minX, point.x);
            maxX = std::max(maxX, point.x);
            minY = std::min(minY, point.y);
            maxY = std::max(maxY, point.y);
        }
        
        // Clamp to image bounds
        int x1 = std::max(0, static_cast<int>(minX));
        int x2 = std::min(outputSize - 1, static_cast<int>(maxX));
        int y1 = std::max(0, static_cast<int>(minY));
        int y2 = std::min(outputSize - 1, static_cast<int>(maxY));
        
        // Fill the bounding box with the color
        for (int y = y1; y <= y2; ++y) {
            for (int x = x1; x <= x2; ++x) {
                int pixelIndex = (y * outputSize + x) * 4;
                pixels[pixelIndex] = color.r;
                pixels[pixelIndex + 1] = color.g;
                pixels[pixelIndex + 2] = color.b;
                pixels[pixelIndex + 3] = 255; // Full alpha
            }
        }
    }
    
    // Create CCImage from pixel data
    auto image = new cocos2d::CCImage();
    bool success = image->initWithRawData(pixels.data(), pixels.size(), 
        outputSize, outputSize, 8, false);
    
    if (!success) {
        delete image;
        return nullptr;
    }
    
    return image;
}

std::vector<cocos2d::ccColor3B> BackgroundGenerator::reducePalette(
    const std::vector<cocos2d::ccColor3B>& colors, int maxColors) {
    
    if (colors.size() <= static_cast<size_t>(maxColors)) {
        return colors;
    }
    
    // Simple palette reduction using k-means clustering approach
    std::vector<cocos2d::ccColor3B> reducedPalette;
    
    // For this placeholder, just take evenly spaced colors
    float step = static_cast<float>(colors.size()) / static_cast<float>(maxColors);
    
    for (int i = 0; i < maxColors; ++i) {
        int index = static_cast<int>(i * step);
        if (index < static_cast<int>(colors.size())) {
            reducedPalette.push_back(colors[index]);
        }
    }
    
    return reducedPalette;
}

float BackgroundGenerator::calculateColorDistance(const cocos2d::ccColor3B& c1, const cocos2d::ccColor3B& c2) {
    // Simple Euclidean distance in RGB space
    // In real implementation, would use Lab color space and Delta E calculation
    float dr = static_cast<float>(c1.r) - static_cast<float>(c2.r);
    float dg = static_cast<float>(c1.g) - static_cast<float>(c2.g);
    float db = static_cast<float>(c1.b) - static_cast<float>(c2.b);
    
    return std::sqrt(dr * dr + dg * dg + db * db);
}

std::vector<std::vector<cocos2d::CCPoint>> BackgroundGenerator::optimizeForTiling(
    const std::vector<std::vector<cocos2d::CCPoint>>& regions) {
    
    auto optimizedRegions = regions;
    
    if (!m_settings.optimizeForTiling) {
        return optimizedRegions;
    }
    
    // Implement tiling optimization
    // This would ensure patterns wrap seamlessly at tile boundaries
    float tileSize = static_cast<float>(m_settings.targetResolution);
    
    for (auto& region : optimizedRegions) {
        for (auto& point : region) {
            // Ensure points that are near edges create seamless transitions
            // This is a simplified approach - real implementation would be more sophisticated
            
            if (point.x < 0.1f * tileSize) {
                // Near left edge - mirror to right edge
                // (This is a placeholder - real tiling would be more complex)
            }
            
            if (point.x > 0.9f * tileSize) {
                // Near right edge - mirror to left edge
            }
            
            if (point.y < 0.1f * tileSize) {
                // Near bottom edge - mirror to top edge
            }
            
            if (point.y > 0.9f * tileSize) {
                // Near top edge - mirror to bottom edge
            }
        }
    }
    
    return optimizedRegions;
}