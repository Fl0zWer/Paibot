#include <util/BrushDrawer.hpp>
#include <manager/BrushManager.hpp>
#include <cmath>
#include <numbers>

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
    if (!CCLayer::init()) return false;

    m_overlayDrawNode = CCDrawNode::create();
    this->addChild(m_overlayDrawNode);

    // Touch handling is enabled when the brush is explicitly started by the tool manager.
    this->setTouchEnabled(false);

    return true;
}

BrushDrawer::~BrushDrawer() {
    // Defensive: ensure listeners are removed even if a brush is destroyed mid-session.
    stop();
}

void BrushDrawer::start(cocos2d::CCNode* hostNode) {
    if (!hostNode) {
        return;
    }
    if (m_isActive && m_hostNode == hostNode) {
        return;
    }

    if (this->getParent() && this->getParent() != hostNode) {
        this->removeFromParentAndCleanup(false);
    }

    if (!this->getParent()) {
        hostNode->addChild(this);
    }

    m_points.clear();
    clearOverlay();

    m_hostNode = hostNode;
    m_isActive = true;

    // Attach listeners exactly once per activation.
    this->setTouchEnabled(true);
}

void BrushDrawer::stop() {
    if (!m_isActive && !this->getParent()) {
        return;
    }

    m_isDrawing = false;
    m_isActive = false;
    m_hostNode = nullptr;

    clearOverlay();
    m_points.clear();

    // Detach listeners to avoid residual callbacks when switching tools.
    this->setTouchEnabled(false);

    if (this->getParent()) {
        this->removeFromParentAndCleanup(true);
    }
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
    
    auto point = this->convertToNodeSpace(touch->getLocation());
    startDrawing(point);
    return true;
}

void BrushDrawer::ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    if (isSpacePressed()) {
        // Space is held - ignore touch moves for drawing
        return;
    }
    
    auto point = this->convertToNodeSpace(touch->getLocation());
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
    auto angle = std::atan2(static_cast<double>(diff.y), static_cast<double>(diff.x));
    
    // Snap to 90° increments to match the documented behaviour and avoid jitter.
    auto normalized = std::remainder(angle, std::numbers::pi_v<double> * 2.0);
    auto snappedAngle = std::round(normalized / (std::numbers::pi_v<double> / 2.0)) * (std::numbers::pi_v<double> / 2.0);
    auto length = static_cast<double>(ccpLength(diff));
    auto offsetX = length * std::cos(snappedAngle);
    auto offsetY = length * std::sin(snappedAngle);

    return {
        static_cast<float>(origin.x + offsetX),
        static_cast<float>(origin.y + offsetY)
    };
}
