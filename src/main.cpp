#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <ui/PaibotButtonBar.hpp>
#include <manager/BrushManager.hpp>
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

        // Initialize the brush manager
        BrushManager::get()->loadSettings();

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

    void onPlaytest(cocos2d::CCObject* sender) {
        if (m_fields->m_paibotButtonBar && m_fields->m_paibotButtonBar->getBrushDrawer()) {
            m_fields->m_paibotButtonBar->getBrushDrawer()->clearOverlay();
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
