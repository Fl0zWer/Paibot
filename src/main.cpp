#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <ui/PaibotButtonBar.hpp>
#include <manager/BrushManager.hpp>

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
    PaibotButtonBar* m_paibotButtonBar = nullptr;
    
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) {
            return false;
        }

        // Initialize the brush manager
        BrushManager::get()->loadSettings();

        // Create and add the Paibot button bar (inspired by Allium's AlliumButtonBar)
        m_paibotButtonBar = PaibotButtonBar::create(this);
        if (m_paibotButtonBar) {
            this->addChild(m_paibotButtonBar);
            
            // Add the button bar to the editor's tab system
            auto buttonBar = m_paibotButtonBar->getButtonBar();
            if (buttonBar) {
                this->addChild(buttonBar);
                
                // Position it appropriately in the UI
                auto winSize = CCDirector::get()->getWinSize();
                buttonBar->setPosition({winSize.width / 2, 50});
            }
            
            log::info("Paibot Drawing Tool initialized successfully");
        } else {
            log::error("Failed to create Paibot button bar");
        }

        return true;
    }
    
    void onPlaytest() {
        // Clean up any active drawing operations before playtesting
        if (m_paibotButtonBar && m_paibotButtonBar->getBrushDrawer()) {
            m_paibotButtonBar->getBrushDrawer()->clearOverlay();
            m_paibotButtonBar->resetToggles(nullptr);
        }
        
        EditorUI::onPlaytest();
    }
    
    // Handle keyboard shortcuts (Allium-style modifiers)
    bool keyDown(enumKeyCodes key) {
        auto manager = BrushManager::get();
        
        // Update keyboard state for modifier tracking
        manager->updateKeyboardState();
        
        // Handle specific shortcuts
        switch (key) {
            case KEY_Space:
                // Space key for temporary pan mode (matching Allium behavior)
                manager->m_panEditorInBrush = true;
                return true;
                
            case KEY_G:
                // Quick access to gradient bucket
                if (m_paibotButtonBar && m_paibotButtonBar->m_gradientBucketToggle) {
                    m_paibotButtonBar->m_gradientBucketToggle->activate();
                    return true;
                }
                break;
                
            case KEY_O:
                // Quick access to optimizer
                log::info("Optimizer shortcut pressed");
                return true;
                
            case KEY_B:
                // Quick access to background generator
                log::info("Background generator shortcut pressed");
                return true;
        }
        
        return EditorUI::keyDown(key);
    }
    
    bool keyUp(enumKeyCodes key) {
        auto manager = BrushManager::get();
        
        switch (key) {
            case KEY_Space:
                // Release space key - exit pan mode
                manager->m_panEditorInBrush = false;
                return true;
        }
        
        return EditorUI::keyUp(key);
    }
};
