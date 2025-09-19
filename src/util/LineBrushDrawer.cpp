#include <util/LineBrushDrawer.hpp>
#include <manager/BrushManager.hpp>

using namespace paibot;
using namespace geode::prelude;

LineBrushDrawer* LineBrushDrawer::create() {
    auto ret = new (std::nothrow) LineBrushDrawer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool LineBrushDrawer::init() {
    if (!BrushDrawer::init()) return false;
    
    m_snapToAngle = false;
    
    return true;
}

void LineBrushDrawer::startDrawing(cocos2d::CCPoint const& point) {
    BrushDrawer::startDrawing(point);
    
    // Enable angle snapping for lines
    m_snapToAngle = isShiftPressed();
}

void LineBrushDrawer::updateDrawing(cocos2d::CCPoint const& point) {
    if (!m_isDrawing || m_points.empty()) return;
    
    cocos2d::CCPoint adjustedPoint = point;
    
    // Apply modifiers (Allium-style)
    if (isAltPressed()) {
        adjustedPoint = snapToGrid(adjustedPoint);
    }
    
    if (isShiftPressed()) {
        adjustedPoint = snapToAngle(adjustedPoint, m_points[0]);
    }
    
    // For lines, we only need start and end points
    if (m_points.size() > 1) {
        m_points[1] = adjustedPoint;
    } else {
        m_points.push_back(adjustedPoint);
    }
    
    updateLine();
}

void LineBrushDrawer::finishDrawing() {
    if (!m_isDrawing || m_points.size() < 2) {
        BrushDrawer::finishDrawing();
        return;
    }
    
    createLineObjects();
    BrushDrawer::finishDrawing();
}

void LineBrushDrawer::createLineObjects() {
    if (m_points.size() < 2) return;
    
    auto start = m_points[0];
    auto end = m_points[1];
    auto thickness = calculateLineThickness();
    
    // In real implementation, create actual GameObject instances
    // For now, just log the action
    log::info("Creating line from ({:.1f}, {:.1f}) to ({:.1f}, {:.1f}) with thickness {:.1f}",
              start.x, start.y, end.x, end.y, thickness);
    
    // Real implementation would:
    // 1. Calculate line direction and perpendicular
    // 2. Create rectangle objects along the line
    // 3. Apply proper color and blending
    // 4. Add to undo stack
}

float LineBrushDrawer::calculateLineThickness() const {
    return BrushManager::get()->m_brushWidth;
}