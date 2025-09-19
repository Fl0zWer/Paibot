#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/CCMenuItemToggler.hpp>

namespace paibot {
    // Custom toggle button class inspired by Allium's MenuItemTogglerExtra
    class MenuItemTogglerExtra : public CCMenuItemToggler {
    protected:
        std::function<void(MenuItemTogglerExtra*)> m_callback; // legacy; not used for invoke
        void onToggle(cocos2d::CCObject*);
        
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

        ~MenuItemTogglerExtra() override = default;

        // Ensure callback invocation regardless of selector wiring
        void activate() override;

        // Toggle and invoke callback
        void toggle(bool toggled);
        // Toggle without invoking callback (for programmatic resets)
        void toggleSilent(bool toggled);
        bool isToggled() const;

        void setCallback(std::function<void(MenuItemTogglerExtra*)> const& callback);
    };
}

