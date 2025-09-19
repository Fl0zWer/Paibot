#pragma once

#include <Geode/Geode.hpp>
#include <util/IntegrityLogger.hpp>
#include <vector>
#include <string>
#include <memory>

namespace paibot {
    enum class BackgroundType {
        SeamlessFromImage,
        TextureSynthesis,
        Procedural,
        WangTiles
    };
    
    enum class NoiseType {
        Perlin,
        Simplex,
        Worley
    };
    
    struct BackgroundSettings {
        BackgroundType type = BackgroundType::SeamlessFromImage;
        int tileSize = 1024;
        float continuity = 0.8f;
        float variety = 0.5f;
        int noiseSeed = 12345;
        int parallaxLayers = 1;
        std::string sourceImagePath;
        NoiseType noiseType = NoiseType::Perlin;
        float noiseScale = 0.1f;
        int octaves = 4;
        float persistence = 0.5f;
        float lacunarity = 2.0f;
        int version = 1; // For preset versioning
    };
    
    struct TileSet {
        std::vector<cocos2d::CCImage*> tiles;
        std::vector<std::vector<int>> edgePatterns;  // Edge compatibility matrix
        int tileSize;
        float deltaE;  // Seamlessness measure
        bool isEmpty() const { return tiles.empty(); }
        bool isValid() const { return !isEmpty() && tileSize > 0; }
    };
    
    struct WangTileValidation {
        bool hasValidBorders = true;
        bool hasVisualCuts = false;
        float borderConsistency = 1.0f;
        std::string errorDetails;
    };
    
    class BackgroundGenerator {
    protected:
        BackgroundSettings m_settings;
        TileSet m_currentTileSet;
        bool m_isPreviewActive = false;
        cocos2d::CCNode* m_previewNode = nullptr;
        
        // Integrity and validation
        std::string m_currentOperationId;
        TileSet m_lastValidTileSet;
        bool m_generationValid = true;
        
        // Memory management for previews
        std::vector<std::unique_ptr<cocos2d::CCImage>> m_previewImages;
        
    public:
        static BackgroundGenerator* create();
        bool init();
        
        // Configuration with validation
        void setSettings(const BackgroundSettings& settings);
        BackgroundSettings getSettings() const;
        bool validateSettings() const;
        
        // Main generation methods with integrity checks
        TileSet generateBackground();
        void showPreview();
        void hidePreview();
        void exportTileSet(const std::string& path);
        
        // Validation and integrity
        bool validateTileSet(const TileSet& tileSet);
        bool validateNonEmptyTileSet(const TileSet& tileSet);
        WangTileValidation validateWangTileBorders(const TileSet& tileSet);
        void revertToLastValid();
        
        // Memory preview generation
        std::unique_ptr<cocos2d::CCImage> generatePreviewInMemory();
        void clearPreviewMemory();
        
        // Preset versioning
        void migratePreset(BackgroundSettings& settings, int fromVersion, int toVersion);
        bool validatePresetVersion(const BackgroundSettings& settings);
        
        // Seamless from image
        TileSet createSeamlessFromImage(const std::string& imagePath);
        cocos2d::CCImage* makeSeamless(cocos2d::CCImage* source);
        cocos2d::CCImage* poissonBlend(cocos2d::CCImage* source, cocos2d::CCImage* target, 
                                       const std::vector<cocos2d::CCPoint>& mask);
        
        // Texture synthesis
        TileSet synthesizeTexture(cocos2d::CCImage* sample);
        cocos2d::CCImage* efrosLeungSynthesis(cocos2d::CCImage* sample, int outputSize);
        cocos2d::CCImage* kwatrnaSynthesis(cocos2d::CCImage* sample, int outputSize);
        std::vector<cocos2d::CCPoint> findBestPatches(cocos2d::CCImage* sample, 
                                                      cocos2d::CCImage* target,
                                                      const cocos2d::CCPoint& position,
                                                      int patchSize);
        
        // Procedural generation
        TileSet generateProcedural();
        cocos2d::CCImage* generatePerlinNoise(int size, float scale, int octaves);
        cocos2d::CCImage* generateSimplexNoise(int size, float scale, int octaves);
        cocos2d::CCImage* generateWorleyNoise(int size, float scale);
        cocos2d::CCImage* applyColorPalette(cocos2d::CCImage* heightmap, 
                                            const std::vector<cocos2d::ccColor3B>& palette);
        
        // Wang tiles with border validation
        TileSet generateWangTiles();
        std::vector<cocos2d::CCImage*> createCompatibleTiles(int count);
        bool checkEdgeCompatibility(cocos2d::CCImage* tile1, cocos2d::CCImage* tile2, int edge);
        std::vector<std::vector<int>> generateTileLayout(int width, int height);
        
        // Utility methods
        float calculateSeamlessness(cocos2d::CCImage* tile);
        std::vector<cocos2d::ccColor3B> extractPalette(cocos2d::CCImage* image, int colorCount);
        cocos2d::CCImage* applyHannWindow(cocos2d::CCImage* image);
        cocos2d::CCImage* mirrorEdges(cocos2d::CCImage* image, int borderSize);
        
        // Preview and export
        cocos2d::CCNode* createTilePreview(const TileSet& tileSet, int previewCols = 3, int previewRows = 3);
        void measureDeltaE(const TileSet& tileSet);
        std::string generateExportJSON(const TileSet& tileSet);
        
        // Operation management
        std::string generateOperationId() const;
        bool hasValidGeneration() const { return m_generationValid; }
    };
}