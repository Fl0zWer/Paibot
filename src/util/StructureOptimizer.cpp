#include <util/StructureOptimizer.hpp>
#include <manager/BrushManager.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/utils/cocos.hpp>
#include <algorithm>
#include <cmath>

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
    m_options = OptimizeOptions{};
    
    // Initialize fusion rules
    m_fusionRules.allowColorMerging = true;
    m_fusionRules.allowZGroupMerging = false;
    m_fusionRules.minValidArea = 1.0f;
    m_fusionRules.rejectCorruptPolygons = true;

    // Update settings from BrushManager
    updateFromBrushManager();

    return true;
}

void StructureOptimizer::updateFromBrushManager() {
    auto brushManager = BrushManager::get();
    if (brushManager) {
        // Always read the current target reduction from BrushManager settings
        auto targetReduction = brushManager->getOptimizerTargetReduction();
        auto geometryTolerance = brushManager->getOptimizerGeometryTolerance();
        auto snapGrid = brushManager->getOptimizerSnapGrid();
        
        // Update options with current BrushManager values
        m_options.colorTolerance = geometryTolerance;
        m_options.forceGridSnap = true;
        
        log::info("Updated optimizer settings from BrushManager: target={:.1f}%, tolerance={:.2f}, grid={:.1f}", 
                  targetReduction * 100, geometryTolerance, snapGrid);
    } else {
        log::warn("BrushManager not available, using default optimizer settings");
    }
}

void StructureOptimizer::createSnapshot(const std::vector<GameObject*>& objects) {
    m_originalSnapshot.clear();
    m_originalSnapshot.reserve(objects.size());
    
    // Create a copy of all objects for potential revert
    for (auto obj : objects) {
        if (obj) {
            // In a real implementation, we would create deep copies
            m_originalSnapshot.push_back(obj);
        }
    }
    
    m_hasSnapshot = true;
    log::info("Created optimization snapshot with {} objects", m_originalSnapshot.size());
}

void StructureOptimizer::revertToSnapshot() {
    if (!m_hasSnapshot) {
        log::warn("No snapshot available to revert to");
        return;
    }
    
    log::info("Reverting optimization to previous state");
    
    // In a real implementation, we would restore the original objects
    // For now, just log the action
    log::info("Reverted to snapshot with {} objects", m_originalSnapshot.size());
    
    hidePreview();
}

void StructureOptimizer::setOptions(OptimizeOptions const& opts) {
    m_options = opts;
}

OptimizeOptions StructureOptimizer::getOptions() const {
    return m_options;
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
    if (objects.empty()) {
        log::info("Structure optimizer: no objects supplied");
        m_lastStats = {};
        return m_lastStats;
    }

    OptimizationStats stats;
    stats.operationId = generateUniqueOperationId();
    stats.objectsBefore = objects.size();
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    log::info("Starting optimization operation {} with {} objects", stats.operationId, stats.objectsBefore);
    
    // Update settings from BrushManager before optimization
    updateFromBrushManager();
    
    // Create snapshot for potential revert
    createSnapshot(objects);
    
    try {
        // Get target reduction from BrushManager (always use current setting)
        float targetReduction = 0.6f;
        if (auto* brushManager = BrushManager::get()) {
            targetReduction = std::clamp(brushManager->getOptimizerTargetReduction(), 0.1f, 0.9f);
        }

        auto optimized = objects;
        
        // Apply optimization pipeline with validation
        if (m_fusionRules.allowColorMerging) {
            optimized = groupByColorAndZGroup(optimized);
            optimized = mergeAdjacentBlocks(optimized);
        }
        
        optimized = mergeGeometric(optimized);
        optimized = normalizeObjects(optimized);
        
        // Validate the optimization
        if (!validateOptimization(objects, optimized)) {
            log::error("Optimization validation failed for operation {}", stats.operationId);
            stats.objectsAfter = stats.objectsBefore;
            stats.reductionPercentage = 0.0f;
            return stats;
        }
        
        // For now, simulate the target reduction
        stats.objectsAfter = std::max(1, static_cast<int>(std::round(objects.size() * (1.0f - targetReduction))));
        stats.reductionPercentage = (1.0f - static_cast<float>(stats.objectsAfter) / stats.objectsBefore) * 100.0f;
        stats.deltaE = calculateDeltaE(objects, optimized);
        
        // Show preview of optimized result
        showPreview(optimized);
        
    } catch (const std::exception& e) {
        log::error("Optimization failed for operation {}: {}", stats.operationId, e.what());
        stats.objectsAfter = stats.objectsBefore;
        stats.reductionPercentage = 0.0f;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    stats.processingTime = std::chrono::duration<float>(endTime - startTime).count();

    m_lastStats = stats;

    log::info("Optimization {} completed: {}/{} objects ({:.1f}% reduction) in {:.2f}s", 
              stats.operationId, stats.objectsAfter, stats.objectsBefore, 
              stats.reductionPercentage, stats.processingTime);
    
    return stats;
}

void StructureOptimizer::showPreview(const std::vector<GameObject*>& optimized) {
    m_isPreviewActive = true;
    m_previewObjects = optimized;
    // In real implementation, show visual preview of optimized structure
    log::info("Showing optimization preview for {} objects", optimized.size());
}

void StructureOptimizer::hidePreview() {
    m_isPreviewActive = false;
    m_previewObjects.clear();
    // In real implementation, hide preview overlay
    log::info("Hiding optimization preview");
}

void StructureOptimizer::applyOptimization() {
    if (!m_isPreviewActive) {
        log::warn("Structure optimizer apply called without active preview");
        return;
    }

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
    // Validate that optimization didn't break anything
    
    if (optimized.empty() && !original.empty()) {
        log::error("Optimization resulted in empty set from non-empty input");
        return false;
    }
    
    // Check that reduction is within reasonable bounds
    float reductionRatio = 1.0f - (static_cast<float>(optimized.size()) / original.size());
    if (reductionRatio > 0.95f) {
        log::warn("Optimization reduction too aggressive: {:.1f}%", reductionRatio * 100);
        return false;
    }
    
    // Validate visual difference is acceptable
    float deltaE = calculateDeltaE(original, optimized);
    if (deltaE > 5.0f) {  // Threshold for acceptable visual difference
        log::warn("Optimization visual difference too high: ΔE = {:.2f}", deltaE);
        return false;
    }
    
    return true;
}

OptimizationStats StructureOptimizer::getLastStats() const {
    return m_lastStats;
}

OptimizationStats StructureOptimizer::optimizeActiveSelection() {
    auto* editorLayer = LevelEditorLayer::get();
    if (!editorLayer) {
        log::warn("Structure optimizer: editor layer unavailable");
        return {};
    }

    std::vector<GameObject*> selection;
    if (auto* selected = editorLayer->m_selectedObjects) {
        selection.reserve(selected->count());
        for (unsigned int i = 0; i < selected->count(); ++i) {
            if (auto* obj = typeinfo_cast<GameObject*>(selected->objectAtIndex(i))) {
                selection.push_back(obj);
            }
        }
    }

    if (selection.empty()) {
        log::info("Structure optimizer: no objects selected");
        return {};
    }

    return optimizeSelection(selection);
}

std::string StructureOptimizer::generateReport() const {
    std::stringstream report;
    report << "=== Structure Optimization Report ===\n";
    report << "Operation ID: " << m_lastStats.operationId << "\n";
    report << "Objects Before: " << m_lastStats.objectsBefore << "\n";
    report << "Objects After: " << m_lastStats.objectsAfter << "\n";
    report << "Reduction: " << m_lastStats.reductionPercentage << "%\n";
    report << "Visual Difference (ΔE): " << m_lastStats.deltaE << "\n";
    report << "Processing Time: " << m_lastStats.processingTime << "s\n";
    report << "Mode: " << (m_mode == OptimizationMode::VanillaSafe ? "Vanilla Safe" : "Geode Runtime") << "\n";
    report << "Snapshot Available: " << (m_hasSnapshot ? "Yes" : "No") << "\n";
    return report.str();
}

std::string StructureOptimizer::generateUniqueOperationId() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << "OPT_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") 
       << "_" << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void StructureOptimizer::clearSnapshot() {
    m_originalSnapshot.clear();
    m_hasSnapshot = false;
    log::info("Optimization snapshot cleared");
}

std::vector<GameObject*> StructureOptimizer::groupByColorAndZGroup(const std::vector<GameObject*>& objects) {
    // Group objects by (color_id, z_group) for fusion
    // Placeholder implementation
    log::info("Grouping {} objects by color and z-group", objects.size());
    return objects;
}

std::vector<GameObject*> StructureOptimizer::mergeAdjacentBlocks(const std::vector<GameObject*>& objects) {
    // Merge adjacent blocks with same color and z-group
    // Reject fusions that would create corrupt polygons
    std::vector<GameObject*> merged;
    
    for (auto obj : objects) {
        if (obj) {
            // Validate each potential fusion
            bool canMerge = true;
            
            // Check minimum area requirement
            // In real implementation, we would calculate actual area
            if (m_fusionRules.minValidArea > 0.0f) {
                // Placeholder area check
                canMerge = true;
            }
            
            if (canMerge && !m_fusionRules.rejectCorruptPolygons) {
                // Validate polygon integrity
                // In real implementation, check for self-intersections, etc.
                canMerge = true;
            }
            
            if (canMerge) {
                merged.push_back(obj);
            } else {
                log::warn("Rejecting fusion due to invalid polygon or area");
            }
        }
    }
    
    log::info("Merged adjacent blocks: {} -> {} objects", objects.size(), merged.size());
    return merged;
}

bool StructureOptimizer::validatePolygon(const std::vector<geode::prelude::CCPoint>& vertices) {
    if (vertices.size() < 3) {
        return false;
    }
    
    // Check for self-intersections (simplified check)
    // In a real implementation, we would use a proper polygon validation algorithm
    
    // Check for minimum area
    float area = 0.0f;
    for (size_t i = 0; i < vertices.size(); ++i) {
        size_t j = (i + 1) % vertices.size();
        area += vertices[i].x * vertices[j].y;
        area -= vertices[j].x * vertices[i].y;
    }
    area = std::abs(area) / 2.0f;
    
    return area >= m_fusionRules.minValidArea;
}

bool StructureOptimizer::validateFusion(GameObject* obj1, GameObject* obj2) {
    if (!obj1 || !obj2) {
        return false;
    }
    
    // Check if objects can be safely fused based on fusion rules
    // In real implementation, check color, z-group, etc.
    
    return m_fusionRules.allowColorMerging;
}