#pragma once

#include <Geode/Geode.hpp>
#include <vector>

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
    };
    
    class StructureOptimizer {
    protected:
        OptimizationMode m_mode = OptimizationMode::VanillaSafe;
        int m_targetCount = 2000;
        float m_geometryTolerance = 0.1f;
        float m_colorTolerance = 2.0f;  // Delta E threshold
        float m_maxScale = 10.0f;
        bool m_preserveGroupIDs = true;
        bool m_preserveZOrder = true;
        bool m_preserveChannels = true;
        bool m_noTouchHitboxes = true;
        float m_visualTolerance = 1.0f;
    float m_optimizerTargetReduction = 0.3f; // 30% default target
        
        OptimizationStats m_lastStats;
        std::vector<GameObject*> m_previewObjects;
        bool m_isPreviewActive = false;
        
    public:
        static StructureOptimizer* create();
        bool init();
        
        // Configuration
        void setOptimizationMode(OptimizationMode mode);
        void setTargetCount(int count);
        void setGeometryTolerance(float tolerance);
        void setColorTolerance(float deltaE);
        void setPreserveOptions(bool groupIDs, bool zOrder, bool channels, bool hitboxes);
        
        // Main optimization pipeline
        OptimizationStats optimizeSelection(const std::vector<GameObject*>& objects);
        void showPreview(const std::vector<GameObject*>& optimized);
        void hidePreview();
        void applyOptimization();
        
        // Optimization algorithms
        std::vector<GameObject*> normalizeObjects(const std::vector<GameObject*>& objects);
        std::vector<GameObject*> mergeGeometric(const std::vector<GameObject*>& objects);
        std::vector<GameObject*> findPatterns(const std::vector<GameObject*>& objects);
        std::vector<GameObject*> polygonize(const std::vector<GameObject*>& objects);
        std::vector<GameObject*> coalesceTriggers(const std::vector<GameObject*>& objects);
        
        // Geometric merging
        std::vector<GameObject*> mergeLines(const std::vector<GameObject*>& objects);
        std::vector<GameObject*> mergeMosaics(const std::vector<GameObject*>& objects);
        std::vector<GameObject*> mergeSegments(const std::vector<GameObject*>& objects);
        std::vector<GameObject*> mergeOverlaps(const std::vector<GameObject*>& objects);
        
        // Pattern recognition
        std::vector<GameObject*> createInstances(const std::vector<GameObject*>& objects);
        std::vector<GameObject*> createCustomObjects(const std::vector<GameObject*>& objects);
        
        // Validation
        float calculateDeltaE(const std::vector<GameObject*>& before, const std::vector<GameObject*>& after);
        bool validateOptimization(const std::vector<GameObject*>& original, const std::vector<GameObject*>& optimized);
        
        // Statistics
        OptimizationStats getLastStats() const;
        std::string generateReport() const;
    };
}