#include <manager/ToolManager.hpp>
#include <ui/MenuItemTogglerExtra.hpp>
#include <util/BrushDrawer.hpp>
#include <util/LineBrushDrawer.hpp>
#include <util/GradientBrushDrawer.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>

using namespace geode::prelude;

namespace paibot {
    ToolManager* ToolManager::s_instance = nullptr;

    ToolManager* ToolManager::get() {
        if (!s_instance) {
            s_instance = new ToolManager();
        }
        return s_instance;
    }

    void ToolManager::destroy() {
        if (s_instance) {
            delete s_instance;
            s_instance = nullptr;
        }
    }

    ToolManager::~ToolManager() {
        // Ensure listeners are detached before destruction to avoid leaks.
        deactivateActiveBrush();
        m_toggleMap.clear();
    }

    void ToolManager::registerToggle(ToolKind kind, MenuItemTogglerExtra* toggle) {
        if (!toggle) {
            return;
        }
        m_toggleMap[kind] = toggle;
        toggle->toggleSilent(false);
    }

    void ToolManager::unregisterToggle(MenuItemTogglerExtra* toggle) {
        for (auto it = m_toggleMap.begin(); it != m_toggleMap.end(); ) {
            if (it->second == toggle) {
                it = m_toggleMap.erase(it);
            } else {
                ++it;
            }
        }
    }

    void ToolManager::switchTool(ToolKind kind) {
        if (kind == ToolKind::None) {
            clearActiveTool();
            return;
        }

        resetToggleStates(kind);

        auto* editorLayer = LevelEditorLayer::get();
        auto* objectLayer = editorLayer ? editorLayer->m_objectLayer : nullptr;

        if (m_activeKind == kind) {
            if (m_activeBrush && objectLayer) {
                // Idempotent activation: ensure listeners are attached exactly once.
                m_activeBrush->start(objectLayer);
            }
            return;
        }

        deactivateActiveBrush();

        if (auto* newBrush = createBrushForKind(kind)) {
            if (objectLayer) {
                newBrush->start(objectLayer);
                m_activeBrush = newBrush;
                m_activeKind = kind;
            } else {
                log::warn("Unable to attach brush: editor layer not ready");
                m_activeKind = ToolKind::None;
                resetToggleStates(ToolKind::None);
            }
        } else {
            m_activeKind = ToolKind::None;
            resetToggleStates(ToolKind::None);
        }
    }

    void ToolManager::clearActiveTool() {
        resetToggleStates(ToolKind::None);
        deactivateActiveBrush();
        m_activeKind = ToolKind::None;
    }

    BrushDrawer* ToolManager::createBrushForKind(ToolKind kind) {
        switch (kind) {
            case ToolKind::Line:
                return LineBrushDrawer::create();
            case ToolKind::Gradient:
                return GradientBrushDrawer::create();
            case ToolKind::Curve:
            case ToolKind::Freeform:
            case ToolKind::Polygon:
            case ToolKind::Text:
                // TODO: Replace with specific brush implementations when available.
                return BrushDrawer::create();
            case ToolKind::None:
            default:
                break;
        }
        return nullptr;
    }

    void ToolManager::resetToggleStates(ToolKind newActive) {
        for (auto& [kind, toggle] : m_toggleMap) {
            if (!toggle) continue;
            bool shouldEnable = (kind == newActive && newActive != ToolKind::None);
            toggle->toggleSilent(shouldEnable);
        }
    }

    void ToolManager::deactivateActiveBrush() {
        if (m_activeBrush) {
            m_activeBrush->stop();
            m_activeBrush = nullptr;
        }
        m_activeKind = ToolKind::None;
    }
}
