#include <util/StructureOptimizer.hpp>
#include <manager/BrushManager.hpp>
#include <algorithm>

using namespace paibot;
using namespace geode::prelude;

StructureOptimizer* StructureOptimizer::create() {
    auto ret = new (std::nothrow) StructureOptimizer();
    if (ret && ret->init()) {
        return ret;
    }
    delete ret;
    return nullptr;
}

bool StructureOptimizer::init() {
    // Initialize default settings
    m_mode = OptimizationMode::VanillaSafe;
    m_targetCount = 2000;
    m_geometryTolerance = 0.1f;
    m_colorTolerance = 2.0f;
    m_maxScale = 10.0f;
    m_preserveGroupIDs = true;
    m_preserveZOrder = true;
    m_preserveChannels = true;
    m_noTouchHitboxes = true;
    m_visualTolerance = 1.0f;
    
    return true;
}

void StructureOptimizer::setOptimizationMode(OptimizationMode mode) {
    m_mode = mode;
}

void StructureOptimizer::setTargetCount(int count) {
    m_targetCount = std::max(100, count);
}

void StructureOptimizer::setGeometryTolerance(float tolerance) {
    m_geometryTolerance = std::max(0.01f, tolerance);
}

void StructureOptimizer::setColorTolerance(float deltaE) {
    m_colorTolerance = std::max(0.1f, deltaE);
}

void StructureOptimizer::setPreserveOptions(bool groupIDs, bool zOrder, bool channels, bool hitboxes) {
    m_preserveGroupIDs = groupIDs;
    m_preserveZOrder = zOrder;
    m_preserveChannels = channels;
    m_noTouchHitboxes = hitboxes;
}

OptimizationStats StructureOptimizer::optimizeSelection(const std::vector<GameObject*>& objects) {
    // Placeholder implementation
    OptimizationStats stats;
    float targetReduction = 0.3f;
    if (auto* brushManager = BrushManager::get()) {
        // Pull the latest slider value every time we run so the optimizer stays in sync with the UI.
        targetReduction = std::clamp(brushManager->getOptimizerTargetReduction(), 0.1f, 0.9f);
    }

    stats.objectsBefore = objects.size();
    stats.objectsAfter = static_cast<int>(objects.size() * (1.0f - targetReduction));
    stats.reductionPercentage = targetReduction * 100.0f;
    stats.deltaE = 0.5f; // Simulated low visual difference
    stats.processingTime = 1.0f;
    
    m_lastStats = stats;
    
    log::info("Structure optimization: {} -> {} objects ({}% reduction)", 
              stats.objectsBefore, stats.objectsAfter, stats.reductionPercentage);
    
    return stats;
}

void StructureOptimizer::showPreview(const std::vector<GameObject*>& optimized) {
    m_isPreviewActive = true;
    // In real implementation, show visual preview of optimized structure
    log::info("Showing optimization preview for {} objects", optimized.size());
}

void StructureOptimizer::hidePreview() {
    m_isPreviewActive = false;
    // In real implementation, hide preview overlay
    log::info("Hiding optimization preview");
}

void StructureOptimizer::applyOptimization() {
    if (!m_isPreviewActive) return;
    
    // In real implementation, replace original objects with optimized ones
    log::info("Applying structure optimization");
    hidePreview();
}

std::vector<GameObject*> StructureOptimizer::normalizeObjects(const std::vector<GameObject*>& objects) {
    // Placeholder - normalize object transforms, colors, etc.
    return objects;
}

std::vector<GameObject*> StructureOptimizer::mergeGeometric(const std::vector<GameObject*>& objects) {
    // Placeholder - merge lines, segments, overlapping objects
    return objects;
}

std::vector<GameObject*> StructureOptimizer::findPatterns(const std::vector<GameObject*>& objects) {
    // Placeholder - find repeated patterns and create instances
    return objects;
}

std::vector<GameObject*> StructureOptimizer::polygonize(const std::vector<GameObject*>& objects) {
    // Placeholder - convert small objects to polygon representations  
    return objects;
}

std::vector<GameObject*> StructureOptimizer::coalesceTriggers(const std::vector<GameObject*>& objects) {
    // Placeholder - merge compatible triggers
    return objects;
}

std::vector<GameObject*> StructureOptimizer::mergeLines(const std::vector<GameObject*>& objects) {
    // Placeholder - merge collinear line segments
    return objects;
}

std::vector<GameObject*> StructureOptimizer::mergeMosaics(const std::vector<GameObject*>& objects) {
    // Placeholder - merge tile patterns into larger objects
    return objects;
}

std::vector<GameObject*> StructureOptimizer::mergeSegments(const std::vector<GameObject*>& objects) {
    // Placeholder - merge adjacent segments
    return objects;
}

std::vector<GameObject*> StructureOptimizer::mergeOverlaps(const std::vector<GameObject*>& objects) {
    // Placeholder - merge overlapping objects
    return objects;
}

std::vector<GameObject*> StructureOptimizer::createInstances(const std::vector<GameObject*>& objects) {
    // Placeholder - create instances of repeated patterns
    return objects;
}

std::vector<GameObject*> StructureOptimizer::createCustomObjects(const std::vector<GameObject*>& objects) {
    // Placeholder - create custom object definitions
    return objects;
}

float StructureOptimizer::calculateDeltaE(const std::vector<GameObject*>& before, const std::vector<GameObject*>& after) {
    // Placeholder - calculate visual difference using Delta E
    return 0.5f;
}

bool StructureOptimizer::validateOptimization(const std::vector<GameObject*>& original, const std::vector<GameObject*>& optimized) {
    auto deltaE = calculateDeltaE(original, optimized);
    return deltaE <= m_colorTolerance;
}

OptimizationStats StructureOptimizer::getLastStats() const {
    return m_lastStats;
}

std::string StructureOptimizer::generateReport() const {
    return fmt::format(
        "Optimization Report:\n"
        "Objects Before: {}\n"
        "Objects After: {}\n"
        "Reduction: {:.1f}%\n"
        "Visual Difference (ΔE): {:.2f}\n"
        "Processing Time: {:.2f}s",
        m_lastStats.objectsBefore,
        m_lastStats.objectsAfter,
        m_lastStats.reductionPercentage,
        m_lastStats.deltaE,
        m_lastStats.processingTime
    );
}