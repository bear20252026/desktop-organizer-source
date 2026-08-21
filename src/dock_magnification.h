#pragma once

#include "dock_settings.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <vector>
#include <windows.h>

namespace snowdesktop::dock_magnification
{
constexpr float kFocusScale = 1.28f;
constexpr float kInfluenceRadiusInItems = 3.0f;
constexpr int kMinimumFocusSwitchHysteresisPixels = 3;
constexpr int kMaximumFocusSwitchHysteresisPixels = 8;
constexpr int kFocusExitHysteresisPixels = 5;

inline int FocusSwitchHysteresisPixels(int itemPitch)
{
    return std::clamp(
        std::max(1, itemPitch) / 16,
        kMinimumFocusSwitchHysteresisPixels,
        kMaximumFocusSwitchHysteresisPixels);
}

/**
 * @brief 判断指针是否明确越过相邻元素的切换边界。
 *
 * midpoint 两侧形成一个小型 Schmitt 区间：当前目标不同，越界方向也
 * 不同，从而避免边界上的 1px 抖动让 focus 来回翻转。
 */
inline bool HasCrossedFocusSwitchBoundary(
    int previousCenter, int nextCenter,
    int pointerAxis, int itemPitch)
{
    if (previousCenter == nextCenter)
        return true;
    const int midpoint =
        previousCenter + (nextCenter - previousCenter) / 2;
    const int hysteresis =
        FocusSwitchHysteresisPixels(itemPitch);
    return nextCenter > previousCenter
        ? pointerAxis >= midpoint + hysteresis
        : pointerAxis <= midpoint - hysteresis;
}

inline RECT ExpandFocusRetentionBounds(RECT visualBounds)
{
    InflateRect(
        &visualBounds,
        kFocusExitHysteresisPixels,
        kFocusExitHysteresisPixels);
    return visualBounds;
}

inline int GrowthForScale(float scale, int baseIconSize)
{
    return std::max(0, static_cast<int>(std::round(
        std::max(1, baseIconSize) * (std::max(1.0f, scale) - 1.0f))));
}

inline float ScaleForAxisDistance(
    float centerDistance, int itemPitch)
{
    const int pitch = std::max(1, itemPitch);
    const float distanceInItems =
        static_cast<float>(std::abs(centerDistance)) /
        static_cast<float>(pitch);
    if (distanceInItems >= kInfluenceRadiusInItems)
        return 1.0f;
    // cos⁸ fisheye: the authentic macOS Dock magnification curve (ImCoolBar
    // reference). Narrower and more localized than raised-cosine — the peak
    // concentrates tightly around the cursor, giving the signature "bubble"
    // feel of the Aqua Dock. cos⁸(x) = (cos(x))^8.
    constexpr float kPi = 3.14159265358979f;
    const float t = distanceInItems / kInfluenceRadiusInItems; // 0..1
    const float c = std::cos(kPi * 0.5f * t); // 1..0 over 0..R
    const float envelope = c * c * c * c * c * c * c * c; // cos⁸
    return 1.0f + (kFocusScale - 1.0f) * envelope;
}

inline double IntegratedGrowthInItems(
    double distanceInItems)
{
    // Analytic integral of the cos⁸ growth profile used by
    // ScaleForAxisDistance, so packed icon shifts stay consistent with the
    // cos⁸ magnification curve. growth(d) = A·cos⁸(πd/(2R)).
    //
    // ∫₀ᵗ Acos⁸(πu/2)Rdu = A·R/(128π)·[35πt + 84sin(πt)
    //   + 28sin(2πt) + (28/3)sin(3πt) + 2sin(4πt)]
    // where t = distance/R.
    constexpr double kR = static_cast<double>(
        kInfluenceRadiusInItems);
    constexpr double kPi = 3.14159265358979;
    const double distance = std::clamp(
        distanceInItems, 0.0, kR);
    const double t = distance / kR; // 0..1
    const double A = static_cast<double>(kFocusScale - 1.0f);
    const double pt = kPi * t;
    return A * kR / (128.0 * kPi) *
        (35.0 * pt
         + 84.0 * std::sin(pt)
         + 28.0 * std::sin(2.0 * pt)
         + (28.0 / 3.0) * std::sin(3.0 * pt)
         + 2.0 * std::sin(4.0 * pt));
}

inline int AxisShiftForDistance(
    int centerDistance, int itemPitch, int baseIconSize)
{
    if (centerDistance == 0)
        return 0;

    const double distanceInItems =
        static_cast<double>(
            std::abs(centerDistance)) /
        static_cast<double>(
            std::max(1, itemPitch));
    const int magnitude = static_cast<int>(
        std::lround(
            std::max(1, baseIconSize) *
            IntegratedGrowthInItems(
                distanceInItems)));
    return centerDistance < 0 ? -magnitude : magnitude;
}

inline int MaximumAxisShift(int baseIconSize)
{
    return AxisShiftForDistance(3, 1, baseIconSize);
}

inline int PackedAxisShift(
    const std::vector<float>& scales, size_t index,
    int baseIconSize, bool towardPositiveAxis)
{
    if (index >= scales.size())
        return 0;

    const auto growthAt = [&](size_t candidate) {
        return GrowthForScale(scales[candidate], baseIconSize);
    };
    const int currentGrowth = growthAt(index);
    if (towardPositiveAxis)
    {
        int previousGrowth = 0;
        for (size_t candidate = 0; candidate < index; ++candidate)
            previousGrowth += growthAt(candidate);
        return previousGrowth + currentGrowth / 2;
    }

    int followingGrowth = 0;
    for (size_t candidate = index + 1;
        candidate < scales.size(); ++candidate)
    {
        followingGrowth += growthAt(candidate);
    }
    return -(followingGrowth +
        (currentGrowth - currentGrowth / 2));
}

inline RECT MagnifyRect(
    RECT base, DockPosition position, float scale, int baseIconSize,
    int axisShift = 0)
{
    const bool vertical = position == DockPosition::Left ||
        position == DockPosition::Right;
    OffsetRect(&base, vertical ? 0 : axisShift,
        vertical ? axisShift : 0);

    const int growth = GrowthForScale(scale, baseIconSize);
    if (growth == 0)
        return base;

    const int leadingGrowth = growth / 2;
    const int trailingGrowth = growth - leadingGrowth;
    switch (position)
    {
    case DockPosition::Top:
        base.left -= leadingGrowth;
        base.right += trailingGrowth;
        base.bottom += growth;
        break;
    case DockPosition::Left:
        base.top -= leadingGrowth;
        base.bottom += trailingGrowth;
        base.right += growth;
        break;
    case DockPosition::Right:
        base.top -= leadingGrowth;
        base.bottom += trailingGrowth;
        base.left -= growth;
        break;
    case DockPosition::Bottom:
    default:
        base.left -= leadingGrowth;
        base.right += trailingGrowth;
        base.top -= growth;
        break;
    }
    return base;
}

inline RECT AnchorTooltipBounds(
    const RECT& visualBounds, DockPosition position,
    int tooltipWidth, int tooltipHeight, int gap)
{
    tooltipWidth = std::max(0, tooltipWidth);
    tooltipHeight = std::max(0, tooltipHeight);
    gap = std::max(0, gap);

    RECT tooltip{};
    switch (position)
    {
    case DockPosition::Top:
        tooltip.left =
            (visualBounds.left + visualBounds.right -
                tooltipWidth) / 2;
        tooltip.top = visualBounds.bottom + gap;
        break;
    case DockPosition::Left:
        tooltip.left = visualBounds.right + gap;
        tooltip.top =
            (visualBounds.top + visualBounds.bottom -
                tooltipHeight) / 2;
        break;
    case DockPosition::Right:
        tooltip.left =
            visualBounds.left - gap - tooltipWidth;
        tooltip.top =
            (visualBounds.top + visualBounds.bottom -
                tooltipHeight) / 2;
        break;
    case DockPosition::Bottom:
    default:
        tooltip.left =
            (visualBounds.left + visualBounds.right -
                tooltipWidth) / 2;
        tooltip.top =
            visualBounds.top - gap - tooltipHeight;
        break;
    }
    tooltip.right = tooltip.left + tooltipWidth;
    tooltip.bottom = tooltip.top + tooltipHeight;
    return tooltip;
}

inline RECT ExpandInteractionBounds(
    RECT bounds, DockPosition position, int baseIconSize)
{
    const int growth = std::max(
        1, GrowthForScale(kFocusScale, baseIconSize));
    const int axisPadding = std::max(1,
        MaximumAxisShift(baseIconSize) +
        (growth + 1) / 2);
    switch (position)
    {
    case DockPosition::Top:
        bounds.left -= axisPadding;
        bounds.right += axisPadding;
        bounds.bottom += growth;
        break;
    case DockPosition::Left:
        bounds.top -= axisPadding;
        bounds.bottom += axisPadding;
        bounds.right += growth;
        break;
    case DockPosition::Right:
        bounds.top -= axisPadding;
        bounds.bottom += axisPadding;
        bounds.left -= growth;
        break;
    case DockPosition::Bottom:
    default:
        bounds.left -= axisPadding;
        bounds.right += axisPadding;
        bounds.top -= growth;
        break;
    }
    return bounds;
}

inline RECT ExpandPerpendicularBounds(
    RECT bounds, DockPosition position, int baseIconSize)
{
    const int growth = std::max(
        1, GrowthForScale(kFocusScale, baseIconSize));
    switch (position)
    {
    case DockPosition::Top:
        bounds.bottom += growth;
        break;
    case DockPosition::Left:
        bounds.right += growth;
        break;
    case DockPosition::Right:
        bounds.left -= growth;
        break;
    case DockPosition::Bottom:
    default:
        bounds.top -= growth;
        break;
    }
    return bounds;
}

/**
 * @brief 扩展分隔区的 hover 走廊，使其覆盖图标向桌面侧放大的高度。
 *
 * 分割线本身没有可命中的元素矩形；指针从相邻图标斜向经过分割线上方时，
 * 需要继续由最近的图标接管 focus，避免放大波形短暂归零。
 */
inline RECT ExpandSeparatorHoverBounds(
    RECT bounds, DockPosition position, int baseIconSize)
{
    return ExpandPerpendicularBounds(
        bounds, position, baseIconSize);
}

inline RECT ResolveFocusInteractionBounds(
    RECT bounds, DockPosition position, int baseIconSize,
    bool magnificationActive)
{
    return magnificationActive
        ? ExpandSeparatorHoverBounds(
            bounds, position, baseIconSize)
        : bounds;
}

inline RECT FitOverflowViewportToFixedVisuals(
    RECT viewport, DockPosition position,
    const RECT& leadingVisual, const RECT& trailingVisual,
    int separatorGap)
{
    const int gap = std::max(0, separatorGap);
    const bool vertical = position == DockPosition::Left ||
        position == DockPosition::Right;
    if (vertical)
    {
        if (!IsRectEmpty(&leadingVisual))
            viewport.top = std::max(
                viewport.top, leadingVisual.bottom + gap);
        if (!IsRectEmpty(&trailingVisual))
            viewport.bottom = std::min(
                viewport.bottom, trailingVisual.top - gap);
        viewport.bottom = std::max(
            viewport.top, viewport.bottom);
    }
    else
    {
        if (!IsRectEmpty(&leadingVisual))
            viewport.left = std::max(
                viewport.left, leadingVisual.right + gap);
        if (!IsRectEmpty(&trailingVisual))
            viewport.right = std::min(
                viewport.right, trailingVisual.left - gap);
        viewport.right = std::max(
            viewport.left, viewport.right);
    }
    return viewport;
}

inline RECT MoveOverflowViewportWithScrollableVisuals(
    RECT viewport, DockPosition position,
    const RECT& firstBase, const RECT& firstVisual,
    const RECT& lastBase, const RECT& lastVisual)
{
    if (IsRectEmpty(&firstBase) ||
        IsRectEmpty(&firstVisual) ||
        IsRectEmpty(&lastBase) ||
        IsRectEmpty(&lastVisual))
    {
        return viewport;
    }

    const bool vertical = position == DockPosition::Left ||
        position == DockPosition::Right;
    if (vertical)
    {
        viewport.top += firstVisual.top - firstBase.top;
        viewport.bottom += lastVisual.bottom - lastBase.bottom;
        viewport.bottom = std::max(
            viewport.top, viewport.bottom);
    }
    else
    {
        viewport.left += firstVisual.left - firstBase.left;
        viewport.right += lastVisual.right - lastBase.right;
        viewport.right = std::max(
            viewport.left, viewport.right);
    }
    return viewport;
}

inline RECT ExtendPanelAlongDockAxis(
    RECT panel, const RECT& visualElement, DockPosition position,
    int axisMargin = 0)
{
    const int margin = std::max(0, axisMargin);
    const bool vertical = position == DockPosition::Left ||
        position == DockPosition::Right;
    if (vertical)
    {
        panel.top = std::min(panel.top, visualElement.top - margin);
        panel.bottom = std::max(panel.bottom, visualElement.bottom + margin);
    }
    else
    {
        panel.left = std::min(panel.left, visualElement.left - margin);
        panel.right = std::max(panel.right, visualElement.right + margin);
    }
    return panel;
}
}
