#include <util/GradientBrushDrawer.hpp>
#include <manager/BrushManager.hpp>
#include <util/IntegrityLogger.hpp>
#include <cmath>
#include <algorithm>
#include <limits>
#include <numbers>
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace paibot;
using namespace geode::prelude;

namespace {
    constexpr float kTwoPi = std::numbers::pi_v<float> * 2.0f;
}

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
    
    // Initialize cache
    m_cache.isValid = false;
    m_lastValidCache.isValid = false;
    m_interpolationValid = true;
    
    // Get seed from BrushManager
    if (auto brushManager = BrushManager::get()) {
        m_currentSeed = brushManager->getSavedValue<int>("gradient-seed", 42);
    }
    
    return true;
}

bool GradientBrushDrawer::ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    if (isSpacePressed()) {
        return false;
    }

    auto point = this->convertToNodeSpace(touch->getLocation());

    if (m_isPreviewMode) {
        if (m_pendingApply) {
            applyGradient();
            m_pendingApply = false;
            return false;
        }
        hidePreview();
    }

    m_pendingApply = false;
    startDrawing(point);
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
    m_radius = ccpDistance(m_startPoint, m_endPoint);

    // Draw preview based on gradient type
    clearOverlay();
    drawGradientPreview();
}

void GradientBrushDrawer::finishDrawing() {
    if (!m_isDrawing) return;
    
    BrushDrawer::finishDrawing();

    // Perform flood fill and generate gradient
    performFloodFill(m_startPoint);
    clampFillToNearbyObjects();
    showPreview();
    m_pendingApply = true;
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
    m_radius = std::max(m_radius, ccpDistance(m_startPoint, m_endPoint));

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
                bandPoints = generateAngularSector(t * kTwoPi, (i + 1.0f) / steps * kTwoPi);
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
        float angle = kTwoPi * i / segments;
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
        float angle = kTwoPi * i / segments;
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
    if (m_fillArea.empty()) {
        log::warn("Gradient preview aborted: no fill area computed");
        return;
    }
    
    // Validate gradient configuration before showing preview
    if (!validateGradientStops()) {
        log::error("Cannot show preview: invalid gradient configuration");
        return;
    }
    
    // Update cache with current parameters
    updateCache();
    
    // Store current state as last valid if interpolation succeeds
    if (validateHSVInterpolation()) {
        m_lastValidCache = m_cache;
        IntegrityLogger::get()->logOperationEnd(m_cache.operationId, true, "Preview generated successfully");
    } else {
        IntegrityLogger::get()->logOperationEnd(m_cache.operationId, false, "HSV interpolation validation failed");
        return;
    }
    
    m_isPreviewMode = true;
    m_hasPreview = true;
    generateGradientObjects();
    
    log::info("Gradient preview shown with {} stops, seed {}", m_gradientStops.size(), m_currentSeed);
}

void GradientBrushDrawer::hidePreview() {
    m_isPreviewMode = false;
    m_hasPreview = false;
    m_pendingApply = false;
    clearOverlay();
    
    if (m_cache.isValid) {
        IntegrityLogger::get()->logOperationEnd(m_cache.operationId, false, "Preview cancelled");
    }
}

void GradientBrushDrawer::applyGradient() {
    if (!m_isPreviewMode || !m_hasPreview) {
        log::warn("Cannot apply gradient: no valid preview available");
        return;
    }
    
    if (!hasValidPreview()) {
        log::error("Cannot apply gradient: preview validation failed");
        return;
    }
    
    // Check safe mode
    if (auto brushManager = BrushManager::get()) {
        if (brushManager->isSafeMode()) {
            log::warn("Gradient application blocked by safe mode");
            return;
        }
    }
    
    // In real implementation, create actual GameObject instances
    // For now, just log the action
    log::info("Applying gradient with {} stops to {} area points (operation: {})", 
              m_gradientStops.size(), m_fillArea.size(), m_cache.operationId);
    
    if (m_cache.isValid) {
        IntegrityLogger::get()->logOperationEnd(m_cache.operationId, true, 
            "Gradient applied successfully");
    }
    
    hidePreview();
    m_pendingApply = false;
}

void GradientBrushDrawer::drawGradientPreview() {
    if (!m_overlayDrawNode) return;

    // Draw gradient direction indicator
    auto color = BrushManager::get()->getBrushColor();
    m_overlayDrawNode->drawSegment(m_startPoint, m_endPoint, 2.0f, ccc4FFromccc3B(color));

    if (m_radius <= std::numeric_limits<float>::epsilon()) {
        m_radius = ccpDistance(m_startPoint, m_endPoint);
    }
    
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
            m_overlayDrawNode->drawCircle(
                m_startPoint,
                ccpDistance(m_startPoint, m_endPoint),
                ccc4FFromccc3B(color),
                1.0f,
                ccc4FFromccc3B(color),
                32
            );
            break;
        case GradientType::Angular:
            // Draw arc indicator - show radius to communicate sweep size
            m_overlayDrawNode->drawCircle(
                m_startPoint,
                std::max(m_radius, 1.0f),
                ccc4FFromccc3B(color),
                1.0f,
                ccc4FFromccc3B(color),
                16
            );
            break;
    }
}

float GradientBrushDrawer::tForPoint(cocos2d::CCPoint const& p) const {
    switch (m_gradientType) {
        case GradientType::Linear: {
            auto direction = ccpSub(m_endPoint, m_startPoint);
            auto length = ccpLength(direction);
            if (length <= std::numeric_limits<float>::epsilon()) {
                return 0.0f;
            }
            auto normalized = ccpNormalize(direction);
            auto projection = ccpDot(ccpSub(p, m_startPoint), normalized);
            return std::clamp(0.5f + projection / length, 0.0f, 1.0f);
        }
        case GradientType::Radial: {
            if (m_radius <= std::numeric_limits<float>::epsilon()) {
                return 0.0f;
            }
            auto distance = ccpDistance(p, m_startPoint);
            return std::clamp(distance / m_radius, 0.0f, 1.0f);
        }
        case GradientType::Angular: {
            auto forward = ccpSub(m_endPoint, m_startPoint);
            if (ccpLengthSQ(forward) <= std::numeric_limits<float>::epsilon()) {
                return 0.0f;
            }
            auto angle = std::atan2(static_cast<double>(forward.y), static_cast<double>(forward.x));
            auto toPoint = std::atan2(static_cast<double>(p.y - m_startPoint.y), static_cast<double>(p.x - m_startPoint.x));
            auto delta = std::fmod(static_cast<float>(toPoint - angle + kTwoPi), kTwoPi);
            return std::clamp(delta / kTwoPi, 0.0f, 1.0f);
        }
    }
    return 0.0f;
}

void GradientBrushDrawer::clampFillToNearbyObjects(float maxDistance) {
    if (m_fillArea.empty()) {
        return;
    }

    float maxDistanceSq = maxDistance * maxDistance;
    for (auto& point : m_fillArea) {
        auto offset = ccpSub(point, m_startPoint);
        float lengthSq = offset.x * offset.x + offset.y * offset.y;
        if (lengthSq > maxDistanceSq && lengthSq > std::numeric_limits<float>::epsilon()) {
            float scale = maxDistance / std::sqrt(lengthSq);
            point.x = m_startPoint.x + offset.x * scale;
            point.y = m_startPoint.y + offset.y * scale;
        }
    }
}

bool GradientBrushDrawer::validateGradientStops() {
    if (m_gradientStops.empty()) {
        IntegrityLogger::get()->logError("GradientBrush", "No gradient stops defined");
        return false;
    }
    
    if (m_gradientStops.size() < 2) {
        IntegrityLogger::get()->logError("GradientBrush", "Need at least 2 gradient stops");
        return false;
    }
    
    // Validate positions are in [0,1] range and sorted
    for (size_t i = 0; i < m_gradientStops.size(); ++i) {
        const auto& stop = m_gradientStops[i];
        if (stop.position < 0.0f || stop.position > 1.0f) {
            IntegrityLogger::get()->logError("GradientBrush", "Invalid gradient stop position");
            return false;
        }
        
        if (i > 0 && stop.position < m_gradientStops[i-1].position) {
            IntegrityLogger::get()->logError("GradientBrush", "Gradient stops not in order");
            return false;
        }
    }
    
    return true;
}

void GradientBrushDrawer::updateCache() {
    m_cache.operationId = generateOperationId();
    m_cache.seed = m_currentSeed;
    m_cache.type = m_gradientType;
    m_cache.stops = m_gradientStops;
    m_cache.startPoint = m_startPoint;
    m_cache.endPoint = m_endPoint;
    m_cache.radius = m_radius;
    m_cache.result = m_fillArea;
    m_cache.isValid = true;
    
    IntegrityLogger::get()->logOperationStart(m_cache.operationId, "GradientGeneration");
}

bool GradientBrushDrawer::isCacheValid() const {
    return m_cache.isValid && 
           m_cache.seed == m_currentSeed &&
           m_cache.type == m_gradientType &&
           m_cache.stops.size() == m_gradientStops.size();
}

void GradientBrushDrawer::invalidateCache() {
    m_cache.isValid = false;
    IntegrityLogger::get()->logWarning("GradientBrush", "Cache invalidated");
}

cocos2d::ccColor3B GradientBrushDrawer::interpolateColorHSV(float t) {
    if (!validateHSVInterpolation()) {
        log::error("HSV interpolation validation failed, reverting to last valid");
        revertToLastValid();
        return {128, 128, 128}; // Gray fallback
    }
    
    try {
        if (m_gradientStops.empty()) {
            return {0, 0, 0};
        }
        
        t = std::clamp(t, 0.0f, 1.0f);
        
        // Find the two stops to interpolate between
        size_t i = 0;
        while (i < m_gradientStops.size() - 1 && m_gradientStops[i + 1].position < t) {
            ++i;
        }
        
        if (i >= m_gradientStops.size() - 1) {
            return m_gradientStops.back().color;
        }
        
        const auto& stop1 = m_gradientStops[i];
        const auto& stop2 = m_gradientStops[i + 1];
        
        float range = stop2.position - stop1.position;
        if (range <= 0.0f) {
            return stop1.color;
        }
        
        float localT = (t - stop1.position) / range;
        
        // Convert to HSV and interpolate
        auto hsv1 = rgbToHsv(stop1.color);
        auto hsv2 = rgbToHsv(stop2.color);
        
        // Handle hue wraparound
        float h1 = hsv1.r / 255.0f * 360.0f;
        float h2 = hsv2.r / 255.0f * 360.0f;
        
        float hDiff = h2 - h1;
        if (hDiff > 180.0f) hDiff -= 360.0f;
        if (hDiff < -180.0f) hDiff += 360.0f;
        
        float h = h1 + hDiff * localT;
        if (h < 0.0f) h += 360.0f;
        if (h >= 360.0f) h -= 360.0f;
        
        float s = hsv1.g / 255.0f + (hsv2.g / 255.0f - hsv1.g / 255.0f) * localT;
        float v = hsv1.b / 255.0f + (hsv2.b / 255.0f - hsv1.b / 255.0f) * localT;
        
        auto hsvResult = cocos2d::ccColor3B{
            static_cast<GLubyte>(h / 360.0f * 255.0f),
            static_cast<GLubyte>(s * 255.0f),
            static_cast<GLubyte>(v * 255.0f)
        };
        
        auto result = hsvToRgb(hsvResult);
        m_interpolationValid = true;
        return result;
        
    } catch (const std::exception& e) {
        IntegrityLogger::get()->logError("GradientBrush", "HSV interpolation failed: " + std::string(e.what()));
        m_interpolationValid = false;
        revertToLastValid();
        return {128, 128, 128}; // Gray fallback
    }
}

bool GradientBrushDrawer::validateHSVInterpolation() {
    // Check for invalid color values
    for (const auto& stop : m_gradientStops) {
        // Basic RGB validation - more sophisticated checks could be added
        if (stop.alpha < 0.0f || stop.alpha > 1.0f) {
            return false;
        }
    }
    return true;
}

void GradientBrushDrawer::revertToLastValid() {
    if (m_lastValidCache.isValid) {
        log::info("Reverting gradient to last valid state");
        m_gradientStops = m_lastValidCache.stops;
        m_gradientType = m_lastValidCache.type;
        m_startPoint = m_lastValidCache.startPoint;
        m_endPoint = m_lastValidCache.endPoint;
        m_radius = m_lastValidCache.radius;
        m_fillArea = m_lastValidCache.result;
        m_interpolationValid = true;
        IntegrityLogger::get()->logOperationEnd(m_lastValidCache.operationId, true, "Reverted to valid state");
    } else {
        log::warn("No valid gradient state to revert to");
    }
}

std::string GradientBrushDrawer::generateOperationId() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << "GRAD_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") 
       << "_" << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

cocos2d::ccColor3B GradientBrushDrawer::rgbToHsv(cocos2d::ccColor3B rgb) {
    float r = rgb.r / 255.0f;
    float g = rgb.g / 255.0f;
    float b = rgb.b / 255.0f;
    
    float max = std::max({r, g, b});
    float min = std::min({r, g, b});
    float diff = max - min;
    
    float h = 0.0f;
    if (diff > 0.0001f) {
        if (max == r) {
            h = 60.0f * std::fmod((g - b) / diff, 6.0f);
        } else if (max == g) {
            h = 60.0f * ((b - r) / diff + 2.0f);
        } else {
            h = 60.0f * ((r - g) / diff + 4.0f);
        }
    }
    if (h < 0.0f) h += 360.0f;
    
    float s = (max > 0.0001f) ? (diff / max) : 0.0f;
    float v = max;
    
    return {
        static_cast<GLubyte>(h / 360.0f * 255.0f),
        static_cast<GLubyte>(s * 255.0f),
        static_cast<GLubyte>(v * 255.0f)
    };
}

cocos2d::ccColor3B GradientBrushDrawer::hsvToRgb(cocos2d::ccColor3B hsv) {
    float h = hsv.r / 255.0f * 360.0f;
    float s = hsv.g / 255.0f;
    float v = hsv.b / 255.0f;
    
    float c = v * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    
    float r, g, b;
    
    if (h >= 0.0f && h < 60.0f) {
        r = c; g = x; b = 0.0f;
    } else if (h >= 60.0f && h < 120.0f) {
        r = x; g = c; b = 0.0f;
    } else if (h >= 120.0f && h < 180.0f) {
        r = 0.0f; g = c; b = x;
    } else if (h >= 180.0f && h < 240.0f) {
        r = 0.0f; g = x; b = c;
    } else if (h >= 240.0f && h < 300.0f) {
        r = x; g = 0.0f; b = c;
    } else {
        r = c; g = 0.0f; b = x;
    }
    
    return {
        static_cast<GLubyte>((r + m) * 255.0f),
        static_cast<GLubyte>((g + m) * 255.0f),
        static_cast<GLubyte>((b + m) * 255.0f)
    };
}