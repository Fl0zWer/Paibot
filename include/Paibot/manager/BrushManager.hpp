#pragma once

#include <Geode/Geode.hpp>
#include <string>
#include <fstream>

namespace paibot {
    // Singleton manager for brush state and global drawing options (inspired by Allium's BrushManager)
    class BrushManager {
    private:
        static BrushManager* s_instance;
        
        // Settings version for migration support
        int m_settingsVersion = 1;
        
        // Resource integrity
        std::string m_resourcesHash;
        bool m_integrityValid = true;
        
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
        int m_gradientSeed = 42;
        float m_optimizerTargetReduction = 0.6f;
        float m_optimizerGeometryTolerance = 0.1f;
        float m_optimizerSnapGrid = 15.0f;
        int m_seamlessTileSize = 1024;
        int m_bgSize = 1024;
        
        // Safety and integrity settings
        bool m_safeMode = false;
        bool m_enableIntegrityChecks = true;
        
        // ID of the object to place when drawing actual editor objects
        int m_drawObjectId = 211;

    public:
        static BrushManager* get();
        static void destroy();
        
        BrushManager();
        ~BrushManager();
        
        // Settings synchronization with validation and migration
        void loadSettings();
        void saveSettings();
        void saveSettingsAtomic();
        void migrateSettings(int fromVersion, int toVersion);
        bool validateSettings();
        
        // Resource integrity management
        bool verifyResourceIntegrity();
        void calculateResourceHash();
        bool isIntegrityValid() const { return m_integrityValid; }
        void setIntegrityValid(bool valid) { m_integrityValid = valid; }

        // Optimizer configuration helpers
        void setOptimizerTargetReduction(float reduction);
        float getOptimizerTargetReduction() const;
        void setOptimizerGeometryTolerance(float tolerance);
        float getOptimizerGeometryTolerance() const;
        void setOptimizerSnapGrid(float grid);
        float getOptimizerSnapGrid() const;

        // Input state management
        void updateKeyboardState();
        bool isShiftPressed() const;
        bool isAltPressed() const;
        bool isSpacePressed() const;
        
        // Safety mode
        bool isSafeMode() const { return m_safeMode; }
        void setSafeMode(bool safe) { m_safeMode = safe; }
        
        // Utility methods
        cocos2d::ccColor3B getBrushColor() const;
        float getGridSize() const;
        int getDrawObjectId() const { return m_drawObjectId; }
        int getSettingsVersion() const { return m_settingsVersion; }
    };
}

