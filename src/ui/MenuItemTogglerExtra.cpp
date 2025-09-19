#include <ui/MenuItemTogglerExtra.hpp>

using namespace paibot;
using namespace geode::prelude;

MenuItemTogglerExtra* MenuItemTogglerExtra::create(
    cocos2d::CCNode* normalSprite,
    cocos2d::CCNode* selectedSprite,
    std::function<void(MenuItemTogglerExtra*)> const& callback
) {
    // Always provide a valid callback (no-op if empty)
    auto safeCallback = callback ? callback : [](MenuItemTogglerExtra*){};
    auto ret = new (std::nothrow) MenuItemTogglerExtra();
    if (ret && ret->init(normalSprite, selectedSprite, safeCallback)) {
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
    // Initialize CCMenuItemToggler with off/on sprites and a valid selector
    if (!CCMenuItemToggler::init(normalSprite, selectedSprite, this, menu_selector(MenuItemTogglerExtra::onToggle))) {
        return false;
    }
    // Store callback after base initialization so that any internal resets performed by
    // CCMenuItemToggler::init don't clobber the functor storage. In certain builds the base
    // initializer zeroes or fills parts of the object, which previously left m_callback in a
    // corrupted state (filled with 0xFF) and caused crashes when toggles were clicked.
    m_callback = callback ? callback : [](MenuItemTogglerExtra*) {};
    // Do not toggle during init to avoid invoking callbacks before the owner wires references

    return true;
}

void MenuItemTogglerExtra::toggle(bool toggled) {
    this->toggleWithCallback(toggled);
}

void MenuItemTogglerExtra::toggleSilent(bool toggled) {
    // Call base toggle without callback to avoid re-entrancy
    CCMenuItemToggler::toggle(toggled);
}

bool MenuItemTogglerExtra::isToggled() {
    return CCMenuItemToggler::isToggled();
}

void MenuItemTogglerExtra::setCallback(std::function<void(MenuItemTogglerExtra*)> const& callback) {
    m_callback = callback ? callback : [](MenuItemTogglerExtra*) {};
}

void MenuItemTogglerExtra::onToggle(cocos2d::CCObject*) {
    if (!m_callback) return;
    // Prevent re-entrant clicks while handling callback
    this->setClickable(false);
    this->retain();
    try {
        if (m_callback) m_callback(this);
    } catch (...) {
        // Swallow any exception to avoid crash
    }
    this->release();
    this->setClickable(true);
}