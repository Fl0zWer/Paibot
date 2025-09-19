#include <util/BrushDrawer.hpp>
#include <manager/BrushManager.hpp>
#include <cmath>

using namespace paibot;
using namespace geode::prelude;

BrushDrawer* BrushDrawer::create() {
    auto ret = new (std::nothrow) BrushDrawer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool BrushDrawer::init() {
    if (!CCNode::init()) return false;
    
    m_overlayDrawNode = CCDrawNode::create();
    this->addChild(m_overlayDrawNode);
    
    // Enable touch handling
    this->setTouchEnabled(true);
    
    return true;
}

void BrushDrawer::startDrawing(cocos2d::CCPoint const& point) {
    m_isDrawing = true;
    m_points.clear();
    m_points.push_back(point);
    
    // Clear previous overlay
    clearOverlay();
}

void BrushDrawer::updateDrawing(cocos2d::CCPoint const& point) {
    if (!m_isDrawing) return;
    
    cocos2d::CCPoint adjustedPoint = point;
    
    // Apply Allium-style modifiers
    if (isAltPressed()) {
        adjustedPoint = snapToGrid(adjustedPoint);
    }
    
    if (isShiftPressed() && !m_points.empty()) {
        adjustedPoint = snapToAngle(adjustedPoint, m_points[0]);
    }
    
    m_points.push_back(adjustedPoint);
    updateLine();
}

void BrushDrawer::finishDrawing() {
    m_isDrawing = false;
    // Override in subclasses to create actual game objects
}

void BrushDrawer::clearOverlay() {
    if (m_overlayDrawNode) {
        m_overlayDrawNode->clear();
    }
}

void BrushDrawer::updateLine() {
    if (!m_overlayDrawNode || m_points.size() < 2) return;
    
    clearOverlay();
    
    auto manager = BrushManager::get();
    auto color = manager->getBrushColor();
    auto width = manager->m_brushWidth;
    
    // Draw line segments
    for (size_t i = 1; i < m_points.size(); ++i) {
        m_overlayDrawNode->drawSegment(
            m_points[i - 1], 
            m_points[i], 
            width, 
            ccc4FFromccc3B(color)
        );
    }
}

bool BrushDrawer::ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    if (isSpacePressed()) {
        // Space is held - allow panning, don't start drawing
        return false;
    }
    
    auto point = touch->getLocationInNode(this);
    startDrawing(point);
    return true;
}

void BrushDrawer::ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    if (isSpacePressed()) {
        // Space is held - ignore touch moves for drawing
        return;
    }
    
    auto point = touch->getLocationInNode(this);
    updateDrawing(point);
}

void BrushDrawer::ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    if (m_isDrawing) {
        finishDrawing();
    }
}

bool BrushDrawer::isShiftPressed() const {
    return BrushManager::get()->isShiftPressed();
}

bool BrushDrawer::isAltPressed() const {
    return BrushManager::get()->isAltPressed();
}

bool BrushDrawer::isSpacePressed() const {
    return BrushManager::get()->isSpacePressed();
}

cocos2d::CCPoint BrushDrawer::snapToGrid(cocos2d::CCPoint const& point) const {
    auto gridSize = BrushManager::get()->getGridSize();
    return {
        std::round(point.x / gridSize) * gridSize,
        std::round(point.y / gridSize) * gridSize
    };
}

cocos2d::CCPoint BrushDrawer::snapToAngle(cocos2d::CCPoint const& point, cocos2d::CCPoint const& origin) const {
    auto diff = ccpSub(point, origin);
    auto angle = std::atan2(diff.y, diff.x);
    
    // Snap to 45° increments (90° as specified in requirements)
    auto snappedAngle = std::round(angle / (M_PI / 4)) * (M_PI / 4);
    auto length = ccpLength(diff);
    
    return {
        origin.x + length * std::cos(snappedAngle),
        origin.y + length * std::sin(snappedAngle)
    };
}