#pragma once

#include <Geode/Geode.hpp>

namespace paibot {
    // Custom toggle button class inspired by Allium's MenuItemTogglerExtra
    class MenuItemTogglerExtra : public cocos2d::CCMenuItemToggle {
    protected:
        std::function<void(MenuItemTogglerExtra*)> m_callback;
        bool m_isToggled = false;
        
    public:
        static MenuItemTogglerExtra* create(
            cocos2d::CCNode* normalSprite,
            cocos2d::CCNode* selectedSprite,
            std::function<void(MenuItemTogglerExtra*)> const& callback
        );
        
        bool init(
            cocos2d::CCNode* normalSprite,
            cocos2d::CCNode* selectedSprite,
            std::function<void(MenuItemTogglerExtra*)> const& callback
        );
        
        void activate() override;
        void toggle(bool toggled);
        bool isToggled() const;
        
        void setCallback(std::function<void(MenuItemTogglerExtra*)> const& callback);
    };
}