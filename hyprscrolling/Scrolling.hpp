#pragma once

#include <vector>
#include <hyprland/src/layout/IHyprLayout.hpp>
#include <hyprland/src/helpers/memory/Memory.hpp>
#include <hyprland/src/managers/HookSystemManager.hpp>

class CScrollingLayout;
struct SWorkspaceData;

// --- 2.2 Camera System ---
struct SCamera {
    float x = 0.f;
    float y = 0.f;
    float scale = 1.0f;
};

// --- 2.1 World Model (Window Data) ---
struct SScrollingWindowData {
    SScrollingWindowData(PHLWINDOW w, SP<SWorkspaceData> ws) : window(w), workspace(ws) {}

    PHLWINDOWREF       window;
    WP<SWorkspaceData> workspace;

    // Persistent world-space coordinates independent of the viewport
    float              world_x = 0.f;
    float              world_y = 0.f;
    float              width   = 0.f;
    float              height  = 0.f;

    bool               ignoreFullscreenChecks = false;
    PHLWORKSPACEREF    overrideWorkspace;

    CBox               layoutBox; // Used for passing computed screen-space to the renderer
};

// --- 8. Workspace Integration ---
struct SWorkspaceData {
    SWorkspaceData(PHLWORKSPACE w, CScrollingLayout* l) : workspace(w), layout(l) {}

    PHLWORKSPACEREF                       workspace;
    std::vector<SP<SScrollingWindowData>> windowDatas; // Flat list of windows, replacing columns
    
    // Each workspace maintains an independent world and camera state
    SCamera                               camera;

    void                                  addWindow(SP<SScrollingWindowData> w);
    void                                  removeWindow(PHLWINDOW w);

    void                                  recalculate(bool forceInstant = false);

    CScrollingLayout* layout = nullptr;
    WP<SWorkspaceData>                    self;
};

// --- Renderer Integration & Layout Manager ---
class CScrollingLayout : public IHyprLayout {
  public:
    virtual void                     onWindowCreatedTiling(PHLWINDOW, eDirection direction = DIRECTION_DEFAULT);
    virtual void                     onWindowRemovedTiling(PHLWINDOW);
    virtual bool                     isWindowTiled(PHLWINDOW);
    virtual void                     recalculateMonitor(const MONITORID&);
    virtual void                     recalculateWindow(PHLWINDOW);
    virtual void                     onBeginDragWindow();
    virtual void                     resizeActiveWindow(const Vector2D&, eRectCorner corner = CORNER_NONE, PHLWINDOW pWindow = nullptr);
    virtual void                     fullscreenRequestForWindow(PHLWINDOW pWindow, const eFullscreenMode CURRENT_EFFECTIVE_MODE, const eFullscreenMode EFFECTIVE_MODE);
    virtual std::any                 layoutMessage(SLayoutMessageHeader, std::string);
    virtual SWindowRenderLayoutHints requestRenderHints(PHLWINDOW);
    virtual void                     switchWindows(PHLWINDOW, PHLWINDOW);
    virtual void                     moveWindowTo(PHLWINDOW, const std::string& dir, bool silent);
    virtual void                     alterSplitRatio(PHLWINDOW, float, bool);
    virtual std::string              getLayoutName();
    virtual void                     replaceWindowDataWith(PHLWINDOW from, PHLWINDOW to);
    virtual Vector2D                 predictSizeForNewWindowTiled();

    virtual void                     onEnable();
    virtual void                     onDisable();

    CBox                             usableAreaFor(PHLMONITOR m);

  private:
    std::vector<SP<SWorkspaceData>> m_workspaceDatas;

    SP<HOOK_CALLBACK_FN>            m_configCallback;
    SP<HOOK_CALLBACK_FN>            m_focusCallback;

    // Config struct for tunable parameters (Phase 4 & 5)
    struct {
        float focus_visibility_threshold = 0.4f;
        float viewport_margin_ratio = 0.3f;
    } m_config;

    SP<SWorkspaceData>       dataFor(PHLWORKSPACE ws);
    SP<SScrollingWindowData> dataFor(PHLWINDOW w);

    void                     applyNodeDataToWindow(SP<SScrollingWindowData> node, bool instant);

    friend struct SWorkspaceData;
};
