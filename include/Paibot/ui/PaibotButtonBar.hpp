#pragma once

#include <Geode/Geode.hpp>
#include <ui/MenuItemTogglerExtra.hpp>

namespace paibot {
    class BrushDrawer;
    
    class PaibotButtonBar : public cocos2d::CCNode {
    protected:
        geode::Ref<EditButtonBar> m_buttonBar;
        cocos2d::CCArray* m_buttons = nullptr;

        // Tool toggles (matching Allium's pattern)
    MenuItemTogglerExtra* m_lineToggle = nullptr;
    MenuItemTogglerExtra* m_curveToggle = nullptr;
    MenuItemTogglerExtra* m_freeToggle = nullptr;
    MenuItemTogglerExtra* m_polygonToggle = nullptr;
    MenuItemTogglerExtra* m_textToggle = nullptr;
        
        // New feature toggles
    MenuItemTogglerExtra* m_gradientBucketToggle = nullptr;
    MenuItemTogglerExtra* m_optimizerToggle = nullptr;
    MenuItemTogglerExtra* m_backgroundToggle = nullptr;
    MenuItemTogglerExtra* m_panToggle = nullptr;
        
        BrushDrawer* m_brushDrawer = nullptr;

    public:
        static PaibotButtonBar* create(EditorUI* editorUI);
        bool init(EditorUI* editorUI);

    void resetToggles(cocos2d::CCObject* sender);
    void onToggle(cocos2d::CCObject* sender);
        EditButtonBar* getButtonBar() const;
        BrushDrawer* getBrushDrawer() const;
    // Specific helpers
    void activateGradientBucket();
    MenuItemTogglerExtra* getGradientBucketToggle() const { return m_gradientBucketToggle; }

        // Button creation helpers (matching Allium's API)
        CCMenuItemSpriteExtra* addButton(
            std::string_view spriteName, std::string_view bgName, std::string_view id, 
            std::function<void(CCMenuItemSpriteExtra*)> const& callback
        );

        CCMenuItemSpriteExtra* addDefaultButton(
            std::string_view spriteName, std::string_view id, 
            std::function<void(CCMenuItemSpriteExtra*)> const& callback
        );

        MenuItemTogglerExtra* addToggle(
            std::string_view spriteName, std::string_view bgOnName, std::string_view bgOffName, std::string_view id, 
            std::function<void(MenuItemTogglerExtra*)> const& callback
        );

        MenuItemTogglerExtra* addDefaultToggle(
            std::string_view spriteName, std::string_view id,
            std::function<void(MenuItemTogglerExtra*)> const& callback
        );
    };
}