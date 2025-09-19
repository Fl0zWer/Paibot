#include <manager/BrushManager.hpp>
#include <algorithm>

using namespace paibot;
using namespace geode::prelude;

BrushManager* BrushManager::s_instance = nullptr;

BrushManager* BrushManager::get() {
    if (!s_instance) {
        s_instance = new BrushManager();
    }
    return s_instance;
}

void BrushManager::destroy() {
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

BrushManager::BrushManager() {
    loadSettings();
}

BrushManager::~BrushManager() {
    saveSettings();
}

void BrushManager::loadSettings() {
    auto mod = Mod::get();
    m_brushWidth = mod->getSettingValue<double>("brush-line-width");
    m_brushColorId = mod->getSettingValue<int64_t>("brush-color-id");
    m_gradientSteps = static_cast<int>(mod->getSettingValue<int64_t>("gradient-steps"));
    setOptimizerTargetReduction(mod->getSettingValue<double>("optimizer-target-reduction"));
    m_seamlessTileSize = static_cast<int>(mod->getSettingValue<int64_t>("seamless-tile-size"));
    m_drawObjectId = static_cast<int>(mod->getSettingValue<int64_t>("draw-object-id"));
}

void BrushManager::saveSettings() {
    auto mod = Mod::get();
    mod->setSavedValue("brush-line-width", m_brushWidth);
    mod->setSavedValue("brush-color-id", m_brushColorId);
    mod->setSavedValue("gradient-steps", m_gradientSteps);
    mod->setSavedValue("optimizer-target-reduction", m_optimizerTargetReduction);
    mod->setSavedValue("seamless-tile-size", m_seamlessTileSize);
    mod->setSavedValue("draw-object-id", m_drawObjectId);
}

void BrushManager::setOptimizerTargetReduction(float reduction) {
    // Clamp to the UI range so runtime changes stay in sync with the slider limits
    m_optimizerTargetReduction = std::clamp(reduction, 0.1f, 0.9f);
}

float BrushManager::getOptimizerTargetReduction() const {
    return m_optimizerTargetReduction;
}

void BrushManager::updateKeyboardState() {
    // Note: Real keyboard state checking would require platform-specific code
    // For now, we'll use placeholder implementations
}

bool BrushManager::isShiftPressed() const {
    // Placeholder - in real implementation, check platform-specific key state
    return false;
}

bool BrushManager::isAltPressed() const {
    // Placeholder - in real implementation, check platform-specific key state
    return false;
}

bool BrushManager::isSpacePressed() const {
    // Placeholder - in real implementation, check platform-specific key state
    return m_panEditorInBrush;
}

cocos2d::ccColor3B BrushManager::getBrushColor() const {
    // Simple color mapping based on color ID
    // In real implementation, this would map to actual GD color scheme
    switch (m_brushColorId % 10) {
        case 0: return {255, 255, 255}; // White
        case 1: return {255, 0, 0};     // Red
        case 2: return {0, 255, 0};     // Green
        case 3: return {0, 0, 255};     // Blue
        case 4: return {255, 255, 0};   // Yellow
        case 5: return {255, 0, 255};   // Magenta
        case 6: return {0, 255, 255};   // Cyan
        case 7: return {255, 128, 0};   // Orange
        case 8: return {128, 0, 255};   // Purple
        case 9: return {128, 128, 128}; // Gray
        default: return {255, 255, 255};
    }
}

float BrushManager::getGridSize() const {
    // Return the editor grid size - in real implementation, get from EditorUI
    return 30.0f;
}