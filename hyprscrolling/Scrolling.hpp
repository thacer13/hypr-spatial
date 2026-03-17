#pragma once

#include <vector>
#include <memory>
#include <expected>
#include <string_view>

#include <hyprland/src/layout/algorithm/TiledAlgorithm.hpp>
#include <hyprland/src/helpers/memory/Memory.hpp>
#include <hyprland/src/helpers/math/Math.hpp>

namespace Layout {
    class ITarget;
}

struct SCamera {
    float x = 0.f;
    float y = 0.f;
    float scale = 1.0f;
};

struct SScrollingWindowData {
    SScrollingWindowData(SP<Layout::ITarget> t) : target(t) {}

    SP<Layout::ITarget> target;

    float world_x = 0.f;
    float world_y = 0.f;
    float width   = 0.f;
    float height  = 0.f;
};

class CScrollingLayout : public Layout::ITiledAlgorithm {
  public:
    CScrollingLayout() = default;
    virtual ~CScrollingLayout() = default;

    virtual void newTarget(SP<Layout::ITarget> target) override;
    
    // Fixed: Hyprutils::Math::Vector2D
    virtual void movedTarget(SP<Layout::ITarget> target, std::optional<Hyprutils::Math::Vector2D> focalPoint) override;
    virtual void removeTarget(SP<Layout::ITarget> target) override;
    virtual void resizeTarget(const Hyprutils::Math::Vector2D& delta, SP<Layout::ITarget> target, Layout::eRectCorner corner) override;
    
    virtual void recalculate() override;
    virtual void swapTargets(SP<Layout::ITarget> a, SP<Layout::ITarget> b) override;
    virtual void moveTargetInDirection(SP<Layout::ITarget> t, Math::eDirection dir, bool silent) override;
    virtual std::expected<void, std::string> layoutMsg(const std::string_view& sv) override;
    
    // Fixed: Hyprutils::Math::Vector2D
    virtual std::optional<Hyprutils::Math::Vector2D> predictSizeForNewTarget() override;

    virtual SP<Layout::ITarget> getNextCandidate(SP<Layout::ITarget> old) override;

  private:
    std::vector<SP<SScrollingWindowData>> m_windowDatas;
    SCamera m_camera;

    SP<SScrollingWindowData> dataFor(SP<Layout::ITarget> target);
};
