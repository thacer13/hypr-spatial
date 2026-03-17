#include "Scrolling.hpp"
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/state/FocusState.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/render/Renderer.hpp>

// --- Core Workspace & Camera Logic ---

void SWorkspaceData::recalculate(bool forceInstant) {
    if (!workspace) return;

    PHLMONITOR PMONITOR = workspace->m_monitor.lock();
    if (!PMONITOR) return;

    const CBox USABLE = layout->usableAreaFor(PMONITOR);
    const auto workAreaPos = layout->workAreaOnWorkspace(PMONITOR->m_activeWorkspace).pos();

    // The core rendering transformation pipeline
    for (const auto& WINDOW : windowDatas) {
        // screen_position = (world_position - camera_position) * camera.scale
        double screen_x = (WINDOW->world_x - camera.x) * camera.scale;
        double screen_y = (WINDOW->world_y - camera.y) * camera.scale;
        double screen_w = WINDOW->width * camera.scale;
        double screen_h = WINDOW->height * camera.scale;

        // Translate to monitor's usable area
        WINDOW->layoutBox = CBox{screen_x, screen_y, screen_w, screen_h}.translate(workAreaPos);

        layout->applyNodeDataToWindow(WINDOW, forceInstant);
    }
}

void SWorkspaceData::addWindow(SP<SScrollingWindowData> w) {
    windowDatas.push_back(w);
}

void SWorkspaceData::removeWindow(PHLWINDOW w) {
    std::erase_if(windowDatas, [&w](const auto& e) { return e->window.lock() == w; });
}

// --- Layout Manager ---

void CScrollingLayout::onEnable() {
    for (auto const& w : g_pCompositor->m_windows) {
        if (w->m_isFloating || !w->m_isMapped || w->isHidden()) continue;
        onWindowCreatedTiling(w);
    }
}

void CScrollingLayout::onDisable() {
    m_workspaceDatas.clear();
}

void CScrollingLayout::onWindowCreatedTiling(PHLWINDOW window, eDirection direction) {
    auto workspaceData = dataFor(window->m_workspace);

    if (!workspaceData) {
        workspaceData = m_workspaceDatas.emplace_back(makeShared<SWorkspaceData>(window->m_workspace, this));
        workspaceData->self = workspaceData;
    }

    auto windowData = makeShared<SScrollingWindowData>(window, workspaceData);

    // Initial Spawn Dimensions (Fallback defaults)
    windowData->width = 1200.f; 
    windowData->height = 800.f;

    // Center Spawn Policy: window.world_position = camera.center
    PHLMONITOR PMONITOR = window->m_monitor.lock();
    if (PMONITOR) {
        const CBox USABLE = usableAreaFor(PMONITOR);
        // Place the center of the window at the center of the current camera view
        windowData->world_x = workspaceData->camera.x + (USABLE.w / 2.f) - (windowData->width / 2.f);
        windowData->world_y = workspaceData->camera.y + (USABLE.h / 2.f) - (windowData->height / 2.f);
    }

    workspaceData->addWindow(windowData);
    workspaceData->recalculate();
}

void CScrollingLayout::onWindowRemovedTiling(PHLWINDOW window) {
    auto DATA = dataFor(window);
    if (!DATA) return;

    auto WS = DATA->workspace.lock();
    if (WS) {
        WS->removeWindow(window);
        WS->recalculate();
    }
}

void CScrollingLayout::applyNodeDataToWindow(SP<SScrollingWindowData> data, bool force) {
    const auto PWINDOW = data->window.lock();
    if (!PWINDOW || !validMapped(PWINDOW)) return;

    if (PWINDOW->isFullscreen() && !data->ignoreFullscreenChecks) return;

    PWINDOW->m_ruleApplicator->resetProps(Desktop::Rule::RULE_PROP_ALL, Desktop::Types::PRIORITY_LAYOUT);
    PWINDOW->updateWindowData();

    CBox nodeBox = data->layoutBox;
    nodeBox.round();

    PWINDOW->m_size = nodeBox.size();
    PWINDOW->m_position = nodeBox.pos();
    PWINDOW->updateWindowDecos();

    // Standard Hyprland gap/reserved area math
    auto calcPos = PWINDOW->m_position;
    auto calcSize = PWINDOW->m_size;
    const auto RESERVED = PWINDOW->getFullWindowReservedArea();
    calcPos = calcPos + RESERVED.topLeft;
    calcSize = calcSize - (RESERVED.topLeft + RESERVED.bottomRight);

    CBox wb = {calcPos, calcSize};
    wb.round();

    *PWINDOW->m_realSize = wb.size();
    *PWINDOW->m_realPosition = wb.pos();

    if (force) {
        g_pHyprRenderer->damageWindow(PWINDOW);
        PWINDOW->m_realPosition->warp();
        PWINDOW->m_realSize->warp();
        g_pHyprRenderer->damageWindow(PWINDOW);
    }
    PWINDOW->updateWindowDecos();
}

// --- Boilerplate implementations to satisfy IHyprLayout ---
bool CScrollingLayout::isWindowTiled(PHLWINDOW window) { return dataFor(window) != nullptr; }
void CScrollingLayout::recalculateMonitor(const MONITORID& id) {
    const auto PMONITOR = g_pCompositor->getMonitorFromID(id);
    if (PMONITOR && PMONITOR->m_activeWorkspace) {
        if (auto DATA = dataFor(PMONITOR->m_activeWorkspace)) DATA->recalculate();
    }
}
void CScrollingLayout::recalculateWindow(PHLWINDOW window) {
    if (window->m_workspace) {
        if (auto DATA = dataFor(window->m_workspace)) DATA->recalculate();
    }
}
void CScrollingLayout::resizeActiveWindow(const Vector2D& delta, eRectCorner corner, PHLWINDOW pWindow) {}
void CScrollingLayout::onBeginDragWindow() { IHyprLayout::onBeginDragWindow(); }
void CScrollingLayout::fullscreenRequestForWindow(PHLWINDOW pWindow, const eFullscreenMode CURRENT_EFFECTIVE_MODE, const eFullscreenMode EFFECTIVE_MODE) {}
std::any CScrollingLayout::layoutMessage(SLayoutMessageHeader header, std::string message) { return {}; }
SWindowRenderLayoutHints CScrollingLayout::requestRenderHints(PHLWINDOW a) { return {}; }
void CScrollingLayout::switchWindows(PHLWINDOW a, PHLWINDOW b) {}
void CScrollingLayout::moveWindowTo(PHLWINDOW, const std::string& dir, bool silent) {}
void CScrollingLayout::alterSplitRatio(PHLWINDOW, float, bool) {}
std::string CScrollingLayout::getLayoutName() { return "scrolling"; }
void CScrollingLayout::replaceWindowDataWith(PHLWINDOW from, PHLWINDOW to) {}
Vector2D CScrollingLayout::predictSizeForNewWindowTiled() { return Vector2D{}; }

// Helpers
SP<SWorkspaceData> CScrollingLayout::dataFor(PHLWORKSPACE ws) {
    for (const auto& e : m_workspaceDatas) if (e->workspace == ws) return e;
    return nullptr;
}

SP<SScrollingWindowData> CScrollingLayout::dataFor(PHLWINDOW w) {
    if (!w) return nullptr;
    for (const auto& e : m_workspaceDatas) {
        if (e->workspace != w->m_workspace) continue;
        for (const auto& d : e->windowDatas) {
            if (d->window.lock() == w) return d;
        }
    }
    return nullptr;
}

CBox CScrollingLayout::usableAreaFor(PHLMONITOR m) {
    return workAreaOnWorkspace(m->m_activeWorkspace).translate(-m->m_position);
}
