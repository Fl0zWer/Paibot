#pragma once

#include <util/BrushDrawer.hpp>

namespace paibot {
    class LineBrushDrawer : public BrushDrawer {
    protected:
        bool m_snapToAngle = false;
        
    public:
        static LineBrushDrawer* create();
        bool init() override;
        
        void startDrawing(cocos2d::CCPoint const& point) override;
        void updateDrawing(cocos2d::CCPoint const& point) override;
        void finishDrawing() override;
        
        // Line-specific methods
        void createLineObjects();
        float calculateLineThickness() const;
    };
}