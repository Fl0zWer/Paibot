#pragma once

#include <Geode/Geode.hpp>

namespace paibot {
    // Base class for all drawing tools (inspired by Allium's BrushDrawer)
    class BrushDrawer : public cocos2d::CCNode {
    protected:
        cocos2d::CCDrawNode* m_overlayDrawNode = nullptr;
        std::vector<cocos2d::CCPoint> m_points;
        bool m_isDrawing = false;
        
    public:
        static BrushDrawer* create();
        bool init() override;
        
        virtual void startDrawing(cocos2d::CCPoint const& point);
        virtual void updateDrawing(cocos2d::CCPoint const& point);
        virtual void finishDrawing();
        virtual void clearOverlay();
        virtual void updateLine();
        
        // Input handling (with Allium-style modifiers)
        virtual bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event);
        virtual void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event);
        virtual void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event);
        
        // Keyboard modifiers
        virtual bool isShiftPressed() const;  // 90° snap
        virtual bool isAltPressed() const;    // grid snap
        virtual bool isSpacePressed() const;  // pan mode
        
        // Helper methods
        cocos2d::CCPoint snapToGrid(cocos2d::CCPoint const& point) const;
        cocos2d::CCPoint snapToAngle(cocos2d::CCPoint const& point, cocos2d::CCPoint const& origin) const;
    };
}