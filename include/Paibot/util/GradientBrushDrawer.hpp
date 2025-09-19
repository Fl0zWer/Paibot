#pragma once

#include <util/BrushDrawer.hpp>

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
        
        // Flood fill state
        std::vector<std::vector<bool>> m_visitedGrid;
        std::vector<cocos2d::CCPoint> m_fillArea;
        
    public:
        static GradientBrushDrawer* create();
        bool init() override;
        
        // Override drawing methods
        void startDrawing(cocos2d::CCPoint const& point) override;
        void updateDrawing(cocos2d::CCPoint const& point) override;
        void finishDrawing() override;
        void clearOverlay() override;
        
        // Gradient configuration
        void setGradientType(GradientType type);
        void addGradientStop(float position, cocos2d::ccColor3B color, float alpha = 1.0f);
        void clearGradientStops();
        
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
        
        // Preview system
        void showPreview();
        void hidePreview();
        void applyGradient();

    protected:
        void drawGradientPreview();
    };
}