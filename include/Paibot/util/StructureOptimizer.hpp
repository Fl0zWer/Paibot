#pragma once

#include <Geode/Geode.hpp>
#include <vector>
#include <string>
#include <memory>

namespace paibot {
    enum class OptimizationMode {
        VanillaSafe,     // Only use vanilla GD objects
        GeodeRuntime     // Use Geode runtime optimizations
    };
    
    struct OptimizationStats {
        int objectsBefore = 0;
        int objectsAfter = 0;
        float reductionPercentage = 0.0f;
        float deltaE = 0.0f;      // Visual difference measure
        float processingTime = 0.0f;
        std::string operationId;  // Unique ID for logging
    };
    
    // Scope & options for optimization run
    struct OptimizeOptions {
        // Scope
        bool useSelectionOnly = true;   // false -> whole scene (not yet wired)
        
        // Inclusion filters
        bool includeRotated = false;
        bool includeNonUniformScale = false;
        bool includeHidden = false;
        bool includeLocked = false;
        
        // Object kinds
        bool includeTiles = true;
        bool includeWalls = true;
        bool includeDecorRects = false;

        // Behavior
        bool keepExactSkins = true;           // if false, use canonical rectangular tile
        bool respectHoles = true;             // if false, fill holes of same cluster
        float colorTolerance = 0.0f;          // 0 = exact ccColor3B match
        int maxWidthCells = 256;              // limit per piece to avoid huge scales
        int maxHeightCells = 256;
        bool forceGridSnap = true;            // snap to grid for generated rectangles
        bool verboseLog = true;               // print detailed report to log
    };
    
    class StructureOptimizer {
    protected:
        OptimizationMode m_mode = OptimizationMode::VanillaSafe;
        
        // Snapshot for undo functionality
        std::vector<GameObject*> m_originalSnapshot;
        bool m_hasSnapshot = false;
        
        OptimizationStats m_lastStats;
        std::vector<GameObject*> m_previewObjects;
        bool m_isPreviewActive = false;
        OptimizeOptions m_options;
        
        // Fusion rules validation
        struct FusionRules {
            bool allowColorMerging = true;
            bool allowZGroupMerging = false;
            float minValidArea = 1.0f;
            bool rejectCorruptPolygons = true;
        } m_fusionRules;
        
    public:
        static StructureOptimizer* create();
        bool init();
        
        // Configuration - now reads from BrushManager
        void updateFromBrushManager();
        void setOptimizationMode(OptimizationMode mode);
        void setPreserveOptions(bool groupIDs, bool zOrder, bool channels, bool hitboxes);
        
        // Snapshot management for undo/revert
        void createSnapshot(const std::vector<GameObject*>& objects);
        bool hasSnapshot() const { return m_hasSnapshot; }
        void revertToSnapshot();
        void clearSnapshot();
        
        // Fusion rules configuration
        void setFusionRules(const FusionRules& rules) { m_fusionRules = rules; }
        FusionRules getFusionRules() const { return m_fusionRules; }
        
        // Options
        void setOptions(OptimizeOptions const& opts);
        OptimizeOptions getOptions() const;
        
        // Main optimization pipeline with integrity checks
        OptimizationStats optimizeSelection(const std::vector<GameObject*>& objects);
        OptimizationStats optimizeActiveSelection();
        void showPreview(const std::vector<GameObject*>& optimized);
        void hidePreview();
        void applyOptimization();
        
        // Optimization algorithms with validation
        std::vector<GameObject*> normalizeObjects(const std::vector<GameObject*>& objects);
        std::vector<GameObject*> mergeGeometric(const std::vector<GameObject*>& objects);
        std::vector<GameObject*> findPatterns(const std::vector<GameObject*>& objects);
        std::vector<GameObject*> polygonize(const std::vector<GameObject*>& objects);
        std::vector<GameObject*> coalesceTriggers(const std::vector<GameObject*>& objects);
        
        // Geometric merging with fusion rules
        std::vector<GameObject*> mergeLines(const std::vector<GameObject*>& objects);
        std::vector<GameObject*> mergeMosaics(const std::vector<GameObject*>& objects);
        std::vector<GameObject*> mergeSegments(const std::vector<GameObject*>& objects);
        std::vector<GameObject*> mergeOverlaps(const std::vector<GameObject*>& objects);
        std::vector<GameObject*> groupByColorAndZGroup(const std::vector<GameObject*>& objects);
        std::vector<GameObject*> mergeAdjacentBlocks(const std::vector<GameObject*>& objects);
        
        // Pattern recognition
        std::vector<GameObject*> createInstances(const std::vector<GameObject*>& objects);
        std::vector<GameObject*> createCustomObjects(const std::vector<GameObject*>& objects);
        
        // Validation with integrity checks
        float calculateDeltaE(const std::vector<GameObject*>& before, const std::vector<GameObject*>& after);
        bool validateOptimization(const std::vector<GameObject*>& original, const std::vector<GameObject*>& optimized);
        bool validatePolygon(const std::vector<geode::prelude::CCPoint>& vertices);
        bool validateFusion(GameObject* obj1, GameObject* obj2);
        
        // Statistics and logging
        OptimizationStats getLastStats() const;
        std::string generateReport() const;
        std::string generateUniqueOperationId() const;
    };
}

