#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <ui/PaibotButtonBar.hpp>
#include <manager/BrushManager.hpp>
#include <manager/ToolManager.hpp>
#include <util/BrushDrawer.hpp>

using namespace geode::prelude;
using namespace paibot;

/**
 * EditorUI modification that integrates the Paibot drawing tool
 * with UI/UX parity to Allium, plus new features:
 * - Gradient Paint Bucket
 * - Structure Optimizer  
 * - Seamless Background Generator
 */
class $modify(PaibotEditorUI, EditorUI) {
    struct Fields {
        PaibotButtonBar* m_paibotButtonBar = nullptr;
    };
    
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) {
            return false;
        }

        // Initialize the brush manager with integrity checks
        auto brushManager = BrushManager::get();
        brushManager->loadSettings();
        
        // Check resource integrity if enabled
        if (brushManager->isIntegrityValid()) {
            log::info("Resource integrity checks passed");
        } else {
            log::warn("Resource integrity checks failed - some features may be disabled");
        }
        
        // Validate Geode interface version
        if (!validateGeodeCompatibility()) {
            log::error("Geode compatibility check failed");
            return false;
        }

        // Create and add the Paibot button bar (inspired by Allium's AlliumButtonBar)
        m_fields->m_paibotButtonBar = PaibotButtonBar::create(this);
        if (m_fields->m_paibotButtonBar) {
            this->addChild(m_fields->m_paibotButtonBar);
            
            // Add the button bar to the editor's tab system
            auto buttonBar = m_fields->m_paibotButtonBar->getButtonBar();
            if (buttonBar) {
                this->addChild(buttonBar);
                
                // Position it appropriately in the UI
                auto winSize = CCDirector::get()->getWinSize();
                buttonBar->setPosition(ccp(winSize.width / 2, 50));
            }
            
            log::info("Paibot Drawing Tool initialized successfully");
        } else {
            log::error("Failed to create Paibot button bar");
        }

        return true;
    }
    
    bool validateGeodeCompatibility() {
        // Check if current Geode version is compatible
        auto geodeVersion = Loader::get()->getLoadedMod("geode.loader")->getVersion();
        log::info("Geode version: {}", geodeVersion.toString());
        
        // Check game version compatibility
        auto gameVersion = Mod::get()->getSavedValue<std::string>("game-version", "unknown");
        auto supportedBuilds = std::vector<std::string>{"2.207", "2.2074"};
        
        bool compatible = false;
        for (const auto& build : supportedBuilds) {
            if (gameVersion.find(build) != std::string::npos) {
                compatible = true;
                break;
            }
        }
        
        if (!compatible) {
            log::warn("Game version {} may not be fully supported", gameVersion);
            
            // Enable safe mode if version is unsupported
            if (auto brushManager = BrushManager::get()) {
                brushManager->setSafeMode(true);
                log::info("Safe mode enabled due to version incompatibility");
            }
        }
        
        return true; // Allow loading even with warnings
    }

    void onPlaytest(cocos2d::CCObject* sender) {
        if (auto manager = ToolManager::get()) {
            if (auto brush = manager->getActiveBrush()) {
                brush->clearOverlay();
            }
        }
        if (m_fields->m_paibotButtonBar) {
            m_fields->m_paibotButtonBar->resetToggles(nullptr);
        }
        EditorUI::onPlaytest(sender);
    }
    
    // Handle keyboard shortcuts (Allium-style modifiers)
    void keyDown(cocos2d::enumKeyCodes key) {
        auto manager = BrushManager::get();
        
        // Update keyboard state for modifier tracking
        manager->updateKeyboardState();
        
        // Handle specific shortcuts
        switch (key) {
            case cocos2d::KEY_Space:
                // Space key for temporary pan mode (matching Allium behavior)
                manager->m_panEditorInBrush = true;
                break;
                
            case cocos2d::KEY_G:
                // Quick access to gradient bucket
                if (m_fields->m_paibotButtonBar) m_fields->m_paibotButtonBar->activateGradientBucket();
                break;
                
            case cocos2d::KEY_O:
                // Quick access to optimizer
                log::info("Optimizer shortcut pressed");
                break;
                
            case cocos2d::KEY_B:
                // Quick access to background generator
                log::info("Background generator shortcut pressed");
                break;
        }
        
        EditorUI::keyDown(key);
    }
    
    void keyUp(cocos2d::enumKeyCodes key) {
        auto manager = BrushManager::get();
        
        switch (key) {
            case cocos2d::KEY_Space:
                // Release space key - exit pan mode
                manager->m_panEditorInBrush = false;
                break;
        }
        
        EditorUI::keyUp(key);
    }
};
