#include <ui/MenuItemTogglerExtra.hpp>

using namespace paibot;
using namespace geode::prelude;

MenuItemTogglerExtra* MenuItemTogglerExtra::create(
    cocos2d::CCNode* normalSprite,
    cocos2d::CCNode* selectedSprite,
    std::function<void(MenuItemTogglerExtra*)> const& callback
) {
    auto ret = new (std::nothrow) MenuItemTogglerExtra();
    if (ret && ret->init(normalSprite, selectedSprite, callback)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool MenuItemTogglerExtra::init(
    cocos2d::CCNode* normalSprite,
    cocos2d::CCNode* selectedSprite,
    std::function<void(MenuItemTogglerExtra*)> const& callback
) {
    if (!CCMenuItemToggle::initWithTarget(this, menu_selector(MenuItemTogglerExtra::activate))) {
        return false;
    }
    
    auto normalItem = CCMenuItemSprite::create(normalSprite, normalSprite);
    auto selectedItem = CCMenuItemSprite::create(selectedSprite, selectedSprite);
    
    this->addSubItem(normalItem);
    this->addSubItem(selectedItem);
    
    m_callback = callback;
    m_isToggled = false;
    this->setSelectedIndex(0);
    
    return true;
}

void MenuItemTogglerExtra::activate() {
    CCMenuItemToggle::activate();
    m_isToggled = !m_isToggled;
    
    if (m_callback) {
        m_callback(this);
    }
}

void MenuItemTogglerExtra::toggle(bool toggled) {
    if (m_isToggled != toggled) {
        m_isToggled = toggled;
        this->setSelectedIndex(toggled ? 1 : 0);
    }
}

bool MenuItemTogglerExtra::isToggled() const {
    return m_isToggled;
}

void MenuItemTogglerExtra::setCallback(std::function<void(MenuItemTogglerExtra*)> const& callback) {
    m_callback = callback;
}