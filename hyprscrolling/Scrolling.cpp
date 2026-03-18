#include "Scrolling.hpp"
#include <hyprland/src/Compositor.hpp>
#include </usr/include/hyprland/src/helpers/Monitor.hpp>

using namespace Layout;

SP<SScrollingWindowData> CScrollingLayout::dataFor(SP<ITarget> target) {
    for (auto& d : m_windowDatas) {
        if (d->target == target) return d;
    }
    return nullptr;
}

void CScrollingLayout::newTarget(SP<ITarget> target) {
    auto data = makeShared<SScrollingWindowData>(target);

    // Spec: default_spawn_width = 1200, height = 800
    data->width = 1200.f;
    data->height = 800.f;

    float viewport_w = 1920.f;
    float viewport_h = 1080.f;

    if (auto ws = target->workspace()) {
        if (auto mon = ws->m_monitor.lock()) {
            viewport_w = mon->m_size.x;
            viewport_h = mon->m_size.y;
        }
    }

    // --- THE SPATIAL OVERRIDE ---
    // Tag it as floating so Wayland's Scene Graph handles the Z-index and mouse focus natively.
    target->setFloating(true);

    // --- SPEC: CENTER SPAWN POLICY ---
    // Spawn the window exactly where the camera is currently looking
    data->world_x = m_camera.x + (viewport_w / 2.f) - (data->width / 2.f);
    data->world_y = m_camera.y + (viewport_h / 2.f) - (data->height / 2.f);

    m_windowDatas.push_back(data);

    // Notice we do NOT need to auto-pan the camera here, because the window 
    // is intentionally spawning directly in the center of our current view!
    recalculate();
}

void CScrollingLayout::removeTarget(SP<ITarget> target) {
    std::erase_if(m_windowDatas, [&](const auto& d) { return d->target == target; });
    recalculate();
}

void CScrollingLayout::movedTarget(SP<ITarget> target, std::optional<Hyprutils::Math::Vector2D> focalPoint) {
    newTarget(target);
}

void CScrollingLayout::recalculate() {
    // Get the monitor's global offset (crucial for multi-monitor setups or scaled UI)
    float monitor_x = 0.f;
    float monitor_y = 0.f;

    if (!m_windowDatas.empty()) {
        if (auto ws = m_windowDatas.front()->target->workspace()) {
            if (auto mon = ws->m_monitor.lock()) {
                monitor_x = mon->m_position.x;
                monitor_y = mon->m_position.y;
            }
        }
    }

    for (auto& data : m_windowDatas) {
        // screen_position = monitor_offset + ((world_position - camera_position) * camera.scale)
        double screen_x = monitor_x + ((data->world_x - m_camera.x) * m_camera.scale);
        double screen_y = monitor_y + ((data->world_y - m_camera.y) * m_camera.scale);
        double screen_w = data->width * m_camera.scale;
        double screen_h = data->height * m_camera.scale;

        CBox box{screen_x, screen_y, screen_w, screen_h};
        data->target->setPositionGlobal(box);
    }
}

void CScrollingLayout::resizeTarget(const Hyprutils::Math::Vector2D& delta, SP<ITarget> target, Layout::eRectCorner corner) {
    auto data = dataFor(target);
    if (!data) return;
    
    data->width += delta.x;
    data->height += delta.y;
    recalculate();
}

void CScrollingLayout::swapTargets(SP<ITarget> a, SP<ITarget> b) { }

void CScrollingLayout::moveTargetInDirection(SP<ITarget> t, Math::eDirection dir, bool silent) { }

std::expected<void, std::string> CScrollingLayout::layoutMsg(const std::string_view& sv) {
    // We parse the string_view into a standard string to use Hyprland's CVarList parser
    std::string msg = std::string(sv);
    const auto ARGS = Hyprutils::String::CVarList(msg, 0, ' ');

    if (ARGS[0] == "pan_x") {
        try {
            m_camera.x += std::stof(ARGS[1]);
            recalculate();
        } catch (...) {}
    } 
    else if (ARGS[0] == "pan_y") {
        try {
            m_camera.y += std::stof(ARGS[1]);
            recalculate();
        } catch (...) {}
    }

    return {};
}

std::optional<Hyprutils::Math::Vector2D> CScrollingLayout::predictSizeForNewTarget() {
    return Hyprutils::Math::Vector2D{1200.0, 800.0};
}

SP<ITarget> CScrollingLayout::getNextCandidate(SP<ITarget> old) {
    if (!m_windowDatas.empty()) return m_windowDatas.front()->target;
    return nullptr;
}
