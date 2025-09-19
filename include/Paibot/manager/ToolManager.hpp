#pragma once

#include <Geode/Geode.hpp>
#include <map>

namespace paibot {
    class BrushDrawer;
    class MenuItemTogglerExtra;

    enum class ToolKind {
        None,
        Line,
        Curve,
        Freeform,
        Polygon,
        Text,
        Gradient
    };

    class ToolManager {
    private:
        static ToolManager* s_instance;

        std::map<ToolKind, MenuItemTogglerExtra*> m_toggleMap;
        geode::Ref<BrushDrawer> m_activeBrush;
        ToolKind m_activeKind = ToolKind::None;

        ToolManager() = default;
        ~ToolManager();

        BrushDrawer* createBrushForKind(ToolKind kind);
        void resetToggleStates(ToolKind newActive);
        void deactivateActiveBrush();

    public:
        ToolManager(ToolManager const&) = delete;
        ToolManager& operator=(ToolManager const&) = delete;

        static ToolManager* get();
        static void destroy();

        void registerToggle(ToolKind kind, MenuItemTogglerExtra* toggle);
        void unregisterToggle(MenuItemTogglerExtra* toggle);

        void switchTool(ToolKind kind);
        void clearActiveTool();

        ToolKind getActiveKind() const { return m_activeKind; }
        BrushDrawer* getActiveBrush() const { return m_activeBrush; }
    };
}
