#include <util/GradientBrushDrawer.hpp>
#include <manager/BrushManager.hpp>
#include <cmath>
#include <algorithm>

using namespace paibot;
using namespace geode::prelude;

GradientBrushDrawer* GradientBrushDrawer::create() {
    auto ret = new (std::nothrow) GradientBrushDrawer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GradientBrushDrawer::init() {
    if (!BrushDrawer::init()) return false;
    
    // Initialize with default gradient
    clearGradientStops();
    addGradientStop(0.0f, {255, 0, 0}, 1.0f);    // Red
    addGradientStop(1.0f, {0, 0, 255}, 1.0f);    // Blue
    
    return true;
}

void GradientBrushDrawer::startDrawing(cocos2d::CCPoint const& point) {
    BrushDrawer::startDrawing(point);
    m_startPoint = point;
    m_endPoint = point;
    
    // Initialize visited grid for flood fill
    auto screenSize = CCDirector::get()->getWinSize();
    int gridWidth = static_cast<int>(screenSize.width / 10) + 1;
    int gridHeight = static_cast<int>(screenSize.height / 10) + 1;
    
    m_visitedGrid.clear();
    m_visitedGrid.resize(gridHeight, std::vector<bool>(gridWidth, false));
    m_fillArea.clear();
}

void GradientBrushDrawer::updateDrawing(cocos2d::CCPoint const& point) {
    if (!m_isDrawing) return;
    
    cocos2d::CCPoint adjustedPoint = point;
    
    // Apply modifiers
    if (isAltPressed()) {
        adjustedPoint = snapToGrid(adjustedPoint);
    }
    
    if (isShiftPressed()) {
        adjustedPoint = snapToAngle(adjustedPoint, m_startPoint);
    }
    
    m_endPoint = adjustedPoint;
    
    // Draw preview based on gradient type
    clearOverlay();
    drawGradientPreview();
}

void GradientBrushDrawer::finishDrawing() {
    if (!m_isDrawing) return;
    
    BrushDrawer::finishDrawing();
    
    // Perform flood fill and generate gradient
    performFloodFill(m_startPoint);
    showPreview();
}

void GradientBrushDrawer::clearOverlay() {
    BrushDrawer::clearOverlay();
}

void GradientBrushDrawer::setGradientType(GradientType type) {
    m_gradientType = type;
}

void GradientBrushDrawer::addGradientStop(float position, cocos2d::ccColor3B color, float alpha) {
    GradientStop stop;
    stop.position = std::clamp(position, 0.0f, 1.0f);
    stop.color = color;
    stop.alpha = std::clamp(alpha, 0.0f, 1.0f);
    
    m_gradientStops.push_back(stop);
    
    // Keep stops sorted by position
    std::sort(m_gradientStops.begin(), m_gradientStops.end(), 
              [](const GradientStop& a, const GradientStop& b) {
                  return a.position < b.position;
              });
}

void GradientBrushDrawer::clearGradientStops() {
    m_gradientStops.clear();
}

void GradientBrushDrawer::performFloodFill(cocos2d::CCPoint const& seedPoint) {
    // Simplified flood fill - in real implementation, this would analyze the level geometry
    // For now, create a simple rectangular area
    
    m_fillArea.clear();
    
    // Create a simple rectangular fill area around the seed point
    float width = std::abs(m_endPoint.x - m_startPoint.x);
    float height = std::abs(m_endPoint.y - m_startPoint.y);
    
    if (width < 10) width = 100;
    if (height < 10) height = 100;
    
    m_fillArea.push_back({seedPoint.x - width/2, seedPoint.y - height/2});
    m_fillArea.push_back({seedPoint.x + width/2, seedPoint.y - height/2});
    m_fillArea.push_back({seedPoint.x + width/2, seedPoint.y + height/2});
    m_fillArea.push_back({seedPoint.x - width/2, seedPoint.y + height/2});
}

std::vector<cocos2d::CCPoint> GradientBrushDrawer::marchingSquares(const std::vector<std::vector<bool>>& grid) {
    // Simplified marching squares implementation
    // In real implementation, this would extract contours from the flood-filled grid
    return m_fillArea;
}

std::vector<cocos2d::CCPoint> GradientBrushDrawer::simplifyPolygon(const std::vector<cocos2d::CCPoint>& points) {
    // Simplified Douglas-Peucker algorithm
    // In real implementation, this would reduce polygon complexity
    return points;
}

void GradientBrushDrawer::generateGradientObjects() {
    if (m_fillArea.size() < 3) return;
    
    auto manager = BrushManager::get();
    int steps = manager->m_gradientSteps;
    
    // Generate gradient bands based on type
    for (int i = 0; i < steps; ++i) {
        float t = static_cast<float>(i) / (steps - 1);
        auto color = interpolateColor(t);
        
        std::vector<cocos2d::CCPoint> bandPoints;
        
        switch (m_gradientType) {
            case GradientType::Linear:
                bandPoints = generateLinearBands(t, (i + 1.0f) / steps);
                break;
            case GradientType::Radial:
                bandPoints = generateRadialRing(t * m_radius, (i + 1.0f) / steps * m_radius);
                break;
            case GradientType::Angular:
                bandPoints = generateAngularSector(t * 2 * M_PI, (i + 1.0f) / steps * 2 * M_PI);
                break;
        }
        
        // In real implementation, create actual GameObject instances here
        // For now, just draw to overlay
        if (bandPoints.size() >= 3 && m_overlayDrawNode) {
            m_overlayDrawNode->drawPolygon(
                bandPoints.data(), 
                bandPoints.size(), 
                ccc4FFromccc3B(color), 
                0, 
                ccc4FFromccc3B(color)
            );
        }
    }
}

cocos2d::ccColor3B GradientBrushDrawer::interpolateColor(float t) {
    if (m_gradientStops.empty()) return {255, 255, 255};
    if (m_gradientStops.size() == 1) return m_gradientStops[0].color;
    
    t = std::clamp(t, 0.0f, 1.0f);
    
    // Find the two stops to interpolate between
    for (size_t i = 0; i < m_gradientStops.size() - 1; ++i) {
        const auto& stop1 = m_gradientStops[i];
        const auto& stop2 = m_gradientStops[i + 1];
        
        if (t >= stop1.position && t <= stop2.position) {
            float localT = (t - stop1.position) / (stop2.position - stop1.position);
            
            // Linear interpolation in RGB space (HSV would be better)
            return {
                static_cast<GLubyte>(stop1.color.r + localT * (stop2.color.r - stop1.color.r)),
                static_cast<GLubyte>(stop1.color.g + localT * (stop2.color.g - stop1.color.g)),
                static_cast<GLubyte>(stop1.color.b + localT * (stop2.color.b - stop1.color.b))
            };
        }
    }
    
    return m_gradientStops.back().color;
}

std::vector<cocos2d::CCPoint> GradientBrushDrawer::generateLinearBands(float t1, float t2) {
    if (m_fillArea.size() < 3) return {};
    
    // Create linear gradient bands perpendicular to start->end direction
    auto direction = ccpNormalize(ccpSub(m_endPoint, m_startPoint));
    auto perpendicular = ccp(-direction.y, direction.x);
    
    auto center = ccpMidpoint(m_startPoint, m_endPoint);
    auto distance = ccpDistance(m_startPoint, m_endPoint);
    
    float offset1 = (t1 - 0.5f) * distance;
    float offset2 = (t2 - 0.5f) * distance;
    
    auto line1 = ccpAdd(center, ccpMult(direction, offset1));
    auto line2 = ccpAdd(center, ccpMult(direction, offset2));
    
    // Create a band between these two lines (simplified)
    return {
        ccpAdd(line1, ccpMult(perpendicular, 1000)),
        ccpAdd(line1, ccpMult(perpendicular, -1000)),
        ccpAdd(line2, ccpMult(perpendicular, -1000)),
        ccpAdd(line2, ccpMult(perpendicular, 1000))
    };
}

std::vector<cocos2d::CCPoint> GradientBrushDrawer::generateRadialRing(float innerRadius, float outerRadius) {
    std::vector<cocos2d::CCPoint> points;
    int segments = 32;
    
    auto center = m_startPoint;
    
    // Generate ring points
    for (int i = 0; i <= segments; ++i) {
        float angle = 2 * M_PI * i / segments;
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);
        
        // Outer ring
        points.push_back({
            center.x + outerRadius * cos_a,
            center.y + outerRadius * sin_a
        });
    }
    
    // Inner ring (reverse order for proper winding)
    for (int i = segments; i >= 0; --i) {
        float angle = 2 * M_PI * i / segments;
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);
        
        points.push_back({
            center.x + innerRadius * cos_a,
            center.y + innerRadius * sin_a
        });
    }
    
    return points;
}

std::vector<cocos2d::CCPoint> GradientBrushDrawer::generateAngularSector(float startAngle, float endAngle) {
    std::vector<cocos2d::CCPoint> points;
    auto center = m_startPoint;
    float radius = m_radius;
    
    points.push_back(center);
    
    int segments = 16;
    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / segments;
        float angle = startAngle + t * (endAngle - startAngle);
        
        points.push_back({
            center.x + radius * std::cos(angle),
            center.y + radius * std::sin(angle)
        });
    }
    
    return points;
}

void GradientBrushDrawer::showPreview() {
    m_isPreviewMode = true;
    generateGradientObjects();
}

void GradientBrushDrawer::hidePreview() {
    m_isPreviewMode = false;
    clearOverlay();
}

void GradientBrushDrawer::applyGradient() {
    if (!m_isPreviewMode) return;
    
    // In real implementation, create actual GameObject instances
    // For now, just log the action
    log::info("Applying gradient with {} stops to {} area points", 
              m_gradientStops.size(), m_fillArea.size());
    
    hidePreview();
}

void GradientBrushDrawer::drawGradientPreview() {
    if (!m_overlayDrawNode) return;
    
    // Draw gradient direction indicator
    auto color = BrushManager::get()->getBrushColor();
    m_overlayDrawNode->drawSegment(m_startPoint, m_endPoint, 2.0f, ccc4FFromccc3B(color));
    
    // Draw gradient type indicator
    switch (m_gradientType) {
        case GradientType::Linear:
            // Draw perpendicular lines at start and end
            {
                auto direction = ccpNormalize(ccpSub(m_endPoint, m_startPoint));
                auto perpendicular = ccpMult(ccp(-direction.y, direction.x), 20);
                m_overlayDrawNode->drawSegment(
                    ccpAdd(m_startPoint, perpendicular), 
                    ccpSub(m_startPoint, perpendicular), 
                    1.0f, ccc4FFromccc3B(color)
                );
                m_overlayDrawNode->drawSegment(
                    ccpAdd(m_endPoint, perpendicular), 
                    ccpSub(m_endPoint, perpendicular), 
                    1.0f, ccc4FFromccc3B(color)
                );
            }
            break;
        case GradientType::Radial:
            // Draw circle at start point
            m_overlayDrawNode->drawCircle(m_startPoint, ccpDistance(m_startPoint, m_endPoint), 0, 32, false, 1.0f, ccc4FFromccc3B(color));
            break;
        case GradientType::Angular:
            // Draw arc indicator
            // Simplified - just draw the direction line
            break;
    }
}