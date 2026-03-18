#define WLR_USE_UNSTABLE

#include <unistd.h>

#include <hyprland/src/includes.hpp>
#include <sstream>

#define private public
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/managers/KeybindManager.hpp>
#include <hyprland/src/managers/SeatManager.hpp>
#undef private

#include <hyprutils/string/VarList.hpp>
using namespace Hyprutils::String;

#include "globals.hpp"
#include "Scrolling.hpp"

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

UP<CScrollingLayout> g_pScrollingLayout;

inline std::any g_keyboardFocusHook;
inline std::any g_pointerFocusHook;

inline SP<HOOK_CALLBACK_FN> g_pActiveWindowHook;

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH        = __hyprland_api_get_hash();
    const std::string CLIENT_HASH = __hyprland_api_get_client_hash();

    if (HASH != CLIENT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[hyprscrolling] Failure in initialization: Version mismatch (headers ver is not equal to running hyprland ver)",
                                     CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[hs] Version mismatch");
    }

    bool success = true;

    g_keyboardFocusHook = g_pSeatManager->m_events.keyboardFocusChange.registerListener([](auto...) {
        
        // Since the active window pointer was moved out of the Compositor,
        // we just loop through the window list and ask which one is currently active!
        for (auto& window : g_pCompositor->m_windows) {
            
            // If it's valid, active, and a floating window (our spatial canvas)
            if (window && g_pCompositor->isWindowActive(window)) {
                if (window->m_isFloating) {
                    g_pCompositor->changeWindowZOrder(window, true);
                }
                break; // Found the active window, stop looking!
            }
        }
    });

    g_pointerFocusHook = g_pSeatManager->m_events.pointerFocusChange.registerListener([](auto...) {
        for (auto& window : g_pCompositor->m_windows) {
            if (window && g_pCompositor->isWindowActive(window)) {
                if (window->m_isFloating) {
                    g_pCompositor->changeWindowZOrder(window, true);
                }
                break;
            }
        }
    });

    HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprscrolling:fullscreen_on_one_column", Hyprlang::INT{0});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprscrolling:column_width", Hyprlang::FLOAT{0.5F});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprscrolling:focus_fit_method", Hyprlang::INT{0});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprscrolling:follow_focus", Hyprlang::INT{1});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprscrolling:follow_debounce_ms", Hyprlang::INT{0});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprscrolling:explicit_column_widths", Hyprlang::STRING{"0.333, 0.5, 0.667, 1.0"});
    // The new 2026 way to register a layout using a factory lambda
    HyprlandAPI::addTiledAlgo(PHANDLE, "spatial", &typeid(CScrollingLayout), []() {
        return makeUnique<CScrollingLayout>();
    });

    if (!success) {
        HyprlandAPI::addNotification(PHANDLE, "[hyprscrolling] Failure in initialization: failed to register dispatchers", CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[hs] Dispatchers failed");
    }

    return {"hyprscrolling", "A plugin to add a scrolling layout to hyprland", "Vaxry", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    HyprlandAPI::removeAlgo(PHANDLE, "spatial");
    g_pScrollingLayout.reset();
}
