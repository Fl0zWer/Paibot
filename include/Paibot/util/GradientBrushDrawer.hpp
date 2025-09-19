#pragma once

#include <util/BrushDrawer.hpp>
#include <util/IntegrityLogger.hpp>
#include <string>
#include <memory>

namespace paibot {
    enum class GradientType {
        Linear,
        Radial,
        Angular
    };
    
    struct GradientStop {
        float position;     // 0.0 to 1.0
        cocos2d::ccColor3B color;
        float alpha;
    };
    
    struct GradientCache {
        std::string operationId;
        int seed;
        GradientType type;
        std::vector<GradientStop> stops;
        cocos2d::CCPoint startPoint;
        cocos2d::CCPoint endPoint;
        float radius;
        std::vector<cocos2d::CCPoint> result;
        bool isValid;
    };
    
    class GradientBrushDrawer : public BrushDrawer {
    protected:
        GradientType m_gradientType = GradientType::Linear;
        std::vector<GradientStop> m_gradientStops;
        cocos2d::CCPoint m_startPoint;
        cocos2d::CCPoint m_endPoint;
        float m_radius = 100.0f;
        float m_tolerance = 5.0f;
        int m_maxObjects = 500;
        bool m_isPreviewMode = false;
        bool m_pendingApply = false; // if true, next click applies
        
        // Deterministic caching
        GradientCache m_cache;
        GradientCache m_lastValidCache;
        int m_currentSeed = 42;
        
        // Preview and validation
        std::vector<GameObject*> m_previewObjects;
        bool m_hasPreview = false;
        bool m_interpolationValid = true;
        
        // Flood fill state
        std::vector<std::vector<bool>> m_visitedGrid;
        std::vector<cocos2d::CCPoint> m_fillArea;
        
    public:
        static GradientBrushDrawer* create();
        bool init() override;
        
        // Override drawing methods
        bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
        void startDrawing(cocos2d::CCPoint const& point) override;
        void updateDrawing(cocos2d::CCPoint const& point) override;
        void finishDrawing() override;
        void clearOverlay() override;
        
        // Gradient configuration with validation
        void setGradientType(GradientType type);
        void addGradientStop(float position, cocos2d::ccColor3B color, float alpha = 1.0f);
        void clearGradientStops();
        bool validateGradientStops();
        
        // Deterministic caching
        void setSeed(int seed) { m_currentSeed = seed; }
        int getSeed() const { return m_currentSeed; }
        void updateCache();
        bool isCacheValid() const;
        void invalidateCache();
        
        // Preview system with confirmation
        void showPreview();
        void hidePreview();
        void applyGradient();
        bool hasValidPreview() const { return m_hasPreview && m_interpolationValid; }
        
        // HSV interpolation with error handling
        cocos2d::ccColor3B interpolateColorHSV(float t);
        bool validateHSVInterpolation();
        void revertToLastValid();
        
        // Flood fill algorithm
        void performFloodFill(cocos2d::CCPoint const& seedPoint);
        std::vector<cocos2d::CCPoint> marchingSquares(const std::vector<std::vector<bool>>& grid);
        std::vector<cocos2d::CCPoint> simplifyPolygon(const std::vector<cocos2d::CCPoint>& points);
        
        // Gradient generation
        void generateGradientObjects();
        cocos2d::ccColor3B interpolateColor(float t);
        std::vector<cocos2d::CCPoint> generateLinearBands(float t1, float t2);
        std::vector<cocos2d::CCPoint> generateRadialRing(float innerRadius, float outerRadius);
        std::vector<cocos2d::CCPoint> generateAngularSector(float startAngle, float endAngle);
        // Map a world point to a 0..1 t along current gradient
        float tForPoint(cocos2d::CCPoint const& p) const;

        // Boundary helpers
        void clampFillToNearbyObjects(float maxDistance = 30.f);
        
        // Utility methods
        std::string generateOperationId() const;

    protected:
        void drawGradientPreview();
        cocos2d::ccColor3B rgbToHsv(cocos2d::ccColor3B rgb);
        cocos2d::ccColor3B hsvToRgb(cocos2d::ccColor3B hsv);
    };
}

