#include <ui/PaibotButtonBar.hpp>
#include <manager/BrushManager.hpp>
#include <util/BrushDrawer.hpp>
#include <util/LineBrushDrawer.hpp>
#include <util/GradientBrushDrawer.hpp>
#include <util/StructureOptimizer.hpp>
#include <util/BackgroundGenerator.hpp>
#include <Geode/ui/GeodeUI.hpp>

using namespace paibot;
using namespace geode::prelude;

PaibotButtonBar* PaibotButtonBar::create(EditorUI* editorUI) {
    auto ret = new (std::nothrow) PaibotButtonBar();
    if (ret && ret->init(editorUI)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool PaibotButtonBar::init(EditorUI* editorUI) {
    if (!CCNode::init()) return false;

    m_buttons = CCArray::create();
    auto winSize = CCDirector::get()->getWinSize();
    auto winBottom = CCDirector::get()->getScreenBottom();
    auto offset = ccp(winSize.width / 2 - 5.f, winBottom + editorUI->m_toolbarHeight - 6.f);
    auto rows = GameManager::get()->getIntGameVariable("0050");
    auto cols = GameManager::get()->getIntGameVariable("0049");

    // Line Tool (matching Allium's pattern)
    m_lineToggle = this->addDefaultToggle(
        "GJ_button_01.png", "line-toggle",
        [=, this](CCMenuItemToggler* sender) {
            if (!sender->isToggled()) return;
            m_brushDrawer = LineBrushDrawer::create();
            LevelEditorLayer::get()->m_objectLayer->addChild(m_brushDrawer);
            m_brushDrawer->setID("line-drawer");
        }
    );

    // Curve Tool
    m_curveToggle = this->addDefaultToggle(
        "GJ_button_01.png", "curve-toggle",
        [=, this](CCMenuItemToggler* sender) {
            if (!sender->isToggled()) return;
            m_brushDrawer = BrushDrawer::create(); // TODO: Create CurveBrushDrawer
            LevelEditorLayer::get()->m_objectLayer->addChild(m_brushDrawer);
            m_brushDrawer->setID("brush-drawer");
        }
    );

    // Freeform Tool
    m_freeToggle = this->addDefaultToggle(
        "GJ_button_01.png", "free-toggle",
        [=, this](CCMenuItemToggler* sender) {
            if (!sender->isToggled()) return;
            m_brushDrawer = BrushDrawer::create(); // TODO: Create FreeBrushDrawer
            LevelEditorLayer::get()->m_objectLayer->addChild(m_brushDrawer);
            m_brushDrawer->setID("brush-drawer");
        }
    );

    // Polygon Tool
    m_polygonToggle = this->addDefaultToggle(
        "GJ_button_01.png", "polygon-toggle",
        [=, this](CCMenuItemToggler* sender) {
            if (!sender->isToggled()) return;
            m_brushDrawer = BrushDrawer::create(); // TODO: Create PolygonBrushDrawer
            LevelEditorLayer::get()->m_objectLayer->addChild(m_brushDrawer);
            m_brushDrawer->setID("brush-drawer");
        }
    );

    // Text Tool
    m_textToggle = this->addDefaultToggle(
        "GJ_button_01.png", "text-toggle",
        [=, this](CCMenuItemToggler* sender) {
            if (!sender->isToggled()) return;
            m_brushDrawer = BrushDrawer::create(); // TODO: Create TextBrushDrawer
            LevelEditorLayer::get()->m_objectLayer->addChild(m_brushDrawer);
            m_brushDrawer->setID("brush-drawer");
        }
    );

    // NEW: Gradient Bucket Tool
    m_gradientBucketToggle = this->addDefaultToggle(
        "GJ_button_01.png", "gradient-bucket-toggle",
        [=, this](CCMenuItemToggler* sender) {
            if (!sender->isToggled()) return;
            m_brushDrawer = GradientBrushDrawer::create();
            LevelEditorLayer::get()->m_objectLayer->addChild(m_brushDrawer);
            m_brushDrawer->setID("gradient-drawer");
        }
    );

    // NEW: Structure Optimizer Button
    auto optimizerButton = this->addDefaultButton(
        "GJ_button_01.png", "optimizer-button",
        [](CCMenuItemSpriteExtra* sender) {
            // TODO: Open optimizer dialog
            log::info("Structure Optimizer clicked");
        }
    );

    // NEW: Background Generator Button
    auto backgroundButton = this->addDefaultButton(
        "GJ_button_01.png", "background-button",
        [](CCMenuItemSpriteExtra* sender) {
            // TODO: Open background generator dialog
            log::info("Background Generator clicked");
        }
    );

    // Pan Toggle (matching Allium's pan toggle)
    auto panToggle = this->addDefaultToggle(
        "GJ_button_01.png", "pan-toggle", 
        [](CCMenuItemToggler* sender) {
            BrushManager::get()->m_panEditorInBrush = !BrushManager::get()->m_panEditorInBrush;
        }
    );
    BrushManager::get()->m_panEditorInBrush = false;

    // Settings Button
    auto settingButton = this->addDefaultButton(
        "GJ_button_01.png", "setting-button",
        [](CCMenuItemSpriteExtra* sender) {
            geode::openSettingsPopup(Mod::get());
        }
    );

    // Finalize Button
    auto finalizeButton = this->addDefaultButton(
        "GJ_button_01.png", "finalize-button",
        [this](auto sender) {
            if (m_brushDrawer) {
                m_brushDrawer->clearOverlay();
                m_brushDrawer->updateLine();
            }
        }
    );

    m_buttonBar = EditButtonBar::create(m_buttons, offset, 0, false, cols, rows);
    m_buttonBar->setID("paibot-tab-bar");

    return true;
}

void PaibotButtonBar::resetToggles(CCObject* sender) {
        auto s = dynamic_cast<MenuItemTogglerExtra*>(sender);
        auto quietOff = [s](MenuItemTogglerExtra* t) {
            if (t && t != s) {
                t->toggleSilent(false);
            }
        };
        quietOff(m_lineToggle);
        quietOff(m_curveToggle);
        quietOff(m_freeToggle);
        quietOff(m_polygonToggle);
        quietOff(m_textToggle);
        quietOff(m_gradientBucketToggle);
    
    if (m_brushDrawer) {
        m_brushDrawer->clearOverlay();
        m_brushDrawer->updateLine();
        m_brushDrawer->removeFromParent();
        m_brushDrawer = nullptr;
    }
    EditorUI::get()->deselectAll();
}

EditButtonBar* PaibotButtonBar::getButtonBar() const {
    return m_buttonBar;
}

BrushDrawer* PaibotButtonBar::getBrushDrawer() const {
    return m_brushDrawer;
}

CCMenuItemSpriteExtra* PaibotButtonBar::addDefaultButton(
    std::string_view spriteName, std::string_view id, 
    std::function<void(CCMenuItemSpriteExtra*)> const& callback
) {
    return this->addButton(spriteName, "GJ_button_01.png", id, callback);
}

CCMenuItemSpriteExtra* PaibotButtonBar::addButton(
    std::string_view spriteName, std::string_view bgName, std::string_view id, 
    std::function<void(CCMenuItemSpriteExtra*)> const& callback
) {
    auto sprite = CCSprite::create(spriteName.data());
    auto bg = CCSprite::create(bgName.data());
    if (!sprite) {
        // Fallback to a simple colored sprite if image not found
        sprite = CCSprite::create();
        sprite->setColor({255, 255, 255});
        sprite->setTextureRect({0, 0, 20, 20});
    }
    if (!bg) {
        bg = CCSprite::create();
        bg->setColor({128, 128, 128});
        bg->setTextureRect({0, 0, 30, 30});
    }
    bg->addChildAtPosition(sprite, Anchor::Center, ccp(0, 0));

    auto button = CCMenuItemExt::createSpriteExtra(
        bg,
        [=](CCObject* sender) {
            callback(static_cast<CCMenuItemSpriteExtra*>(sender));
        }
    );
    button->setID(id.data());
    m_buttons->addObject(button);
    return button;
}

MenuItemTogglerExtra* PaibotButtonBar::addDefaultToggle(
    std::string_view spriteName, std::string_view id,
    std::function<void(MenuItemTogglerExtra*)> const& callback
) {
    return this->addToggle(spriteName, "GJ_button_01.png", "GJ_button_02.png", id, callback);
}

MenuItemTogglerExtra* PaibotButtonBar::addToggle(
    std::string_view spriteName, std::string_view bgOnName, std::string_view bgOffName, std::string_view id, 
    std::function<void(MenuItemTogglerExtra*)> const& callback
) {
    auto sprite = CCSprite::create(spriteName.data());
    auto bgOff = CCSprite::create(bgOffName.data());
    if (!sprite) {
        sprite = CCSprite::create();
        sprite->setColor({255, 255, 255});
        sprite->setTextureRect({0, 0, 20, 20});
    }
    if (!bgOff) {
        bgOff = CCSprite::create();
        bgOff->setColor({128, 128, 128});
        bgOff->setTextureRect({0, 0, 30, 30});
    }
    bgOff->addChildAtPosition(sprite, Anchor::Center, ccp(0, 0));

    sprite = CCSprite::create(spriteName.data());
    auto bgOn = CCSprite::create(bgOnName.data());
    if (!sprite) {
        sprite = CCSprite::create();
        sprite->setColor({255, 255, 255});
        sprite->setTextureRect({0, 0, 20, 20});
    }
    if (!bgOn) {
        bgOn = CCSprite::create();
        bgOn->setColor({200, 200, 255});
        bgOn->setTextureRect({0, 0, 30, 30});
    }
    bgOn->addChildAtPosition(sprite, Anchor::Center, ccp(0, 0));

    // Initialize custom wrapper that wires callbacks safely (off, on)
    auto button = MenuItemTogglerExtra::create(
        bgOff,
        bgOn,
        callback
    );
    button->setID(id.data());
    m_buttons->addObject(button);
    return button;
}

void PaibotButtonBar::activateGradientBucket() {
    if (m_gradientBucketToggle) {
        m_gradientBucketToggle->toggle(true);
        if (m_gradientBucketToggle->isToggled() && m_brushDrawer == nullptr) {
            // Simulate the callback behavior used in init
            this->resetToggles(m_gradientBucketToggle);
            m_brushDrawer = GradientBrushDrawer::create();
            if (m_brushDrawer) {
                LevelEditorLayer::get()->m_objectLayer->addChild(m_brushDrawer);
                m_brushDrawer->setID("gradient-drawer");
            }
        }
    }
}

// No onToggle needed; MenuItemTogglerExtra handles callbacks