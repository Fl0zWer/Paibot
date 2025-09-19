#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

/**
 * MenuLayer modification that draws a simple label to prove that
 * the mod is loaded and hooks work correctly.
 */
class $modify(BaseMenuLayer, MenuLayer) {
public:
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        if (Mod::get()->getSettingValue<bool>("show-menu-label")) {
            auto winSize = CCDirector::sharedDirector()->getWinSize();

            auto label = CCLabelBMFont::create("Base Geode Mod", "bigFont.fnt");
            label->setPosition({winSize.width / 2.f, winSize.height - 40.f});
            label->setScale(0.6f);
            this->addChild(label);
        }

        log::info("BaseMenuLayer initialised; menu label added if enabled.");

        return true;
    }
};
