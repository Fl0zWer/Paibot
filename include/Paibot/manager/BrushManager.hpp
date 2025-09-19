#pragma once

#include <Geode/Geode.hpp>

namespace paibot {
    // Singleton manager for brush state and global drawing options (inspired by Allium's BrushManager)
    class BrushManager {
    private:
        static BrushManager* s_instance;
        
    public:
        // Drawing state
        bool m_panEditorInBrush = false;
        bool m_snapToGrid = false;
        bool m_snapToAngle = false;
        
        // Brush properties (matching Allium's settings)
        float m_brushWidth = 5.0f;
        int m_brushColorId = 1011;
        float m_curveDetail = 0.4f;
        float m_freeThreshold = 0.4f;
        
        // New feature properties
        int m_gradientSteps = 32;
        float m_optimizerTargetReduction = 0.6f;
        int m_seamlessTileSize = 1024;
        // ID of the object to place when drawing actual editor objects
        int m_drawObjectId = 211;

    public:
        static BrushManager* get();
        static void destroy();
        
        BrushManager();
        ~BrushManager();
        
        // Settings synchronization
        void loadSettings();
        void saveSettings();

        // Optimizer configuration helpers
        void setOptimizerTargetReduction(float reduction);
        float getOptimizerTargetReduction() const;

        // Input state management
        void updateKeyboardState();
        bool isShiftPressed() const;
        bool isAltPressed() const;
        bool isSpacePressed() const;
        
        // Utility methods
        cocos2d::ccColor3B getBrushColor() const;
        float getGridSize() const;
        int getDrawObjectId() const { return m_drawObjectId; }
    };
}

