#pragma once

// ── Stacks 弹出视图布局算法（macOS Dock 文件夹堆叠）────────────────────
//
// 三种布局模式：
//   Fan   — 扇形弧线排列，图标沿弧线分布，最经典 macOS Stacks 视觉
//   Grid  — 可滚动网格，适合项目较多的文件夹
//   List  — 列表菜单，图标+文件名，适合文件浏览
//
// 每种布局返回每个 item 的 (x, y, width, height) 矩形，供渲染节点使用。
// 本文件是 L3/L5 层纯算法节点，不依赖 DesktopApp，只接收参数输出结果。

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>
#include <windows.h>

namespace snowdesktop::stacks_popup_layout
{

// ── Fan 布局 ───────────────────────────────────────────────────

struct FanLayoutParams
{
    int iconSize = 48;       // 图标尺寸（像素）
    int spacing = 12;        // 图标间距（沿弧线）
    float arcAngleDeg = 90;  // 弧线总角度（度），0°=正上方
    float arcRadius = 160;   // 弧线半径（像素）
    int maxItems = 10;       // 最多显示项数
};

inline std::vector<RECT> LayoutFan(
    std::size_t itemCount,
    POINT anchor,
    const FanLayoutParams& params = {})
{
    std::vector<RECT> rects;
    const int n = static_cast<int>(
        std::min(itemCount, static_cast<std::size_t>(params.maxItems)));
    if (n <= 0) return rects;

    const float arcRad = params.arcAngleDeg * 3.14159265f / 180.0f;
    const float startAngle = -3.14159265f * 0.5f - arcRad * 0.5f;

    for (int i = 0; i < n; ++i)
    {
        const float t = (n == 1) ? 0.5f
            : static_cast<float>(i) / static_cast<float>(n - 1);
        const float angle = startAngle + arcRad * t;
        const int cx = anchor.x + static_cast<int>(
            std::cos(angle) * params.arcRadius);
        const int cy = anchor.y - static_cast<int>(
            std::sin(angle) * params.arcRadius);
        const int half = params.iconSize / 2;
        rects.push_back({ cx - half, cy - half,
            cx + half, cy + half });
    }
    return rects;
}

// ── Grid 布局 ───────────────────────────────────────────────────

struct GridLayoutParams
{
    int iconSize = 48;
    int gap = 10;
    int maxColumns = 5;
    int maxRows = 8;
};

inline std::vector<RECT> LayoutGrid(
    std::size_t itemCount,
    POINT anchor,
    const GridLayoutParams& params = {})
{
    std::vector<RECT> rects;
    if (itemCount == 0) return rects;

    const int cols = std::clamp(
        static_cast<int>(std::min(itemCount,
            static_cast<std::size_t>(params.maxColumns))),
        1, params.maxColumns);
    const int rows = std::min(
        (static_cast<int>(itemCount) + cols - 1) / cols,
        params.maxRows);
    const int panelW = cols * params.iconSize +
        (cols - 1) * params.gap;
    const int panelH = rows * params.iconSize +
        (rows - 1) * params.gap;

    // 面板出现在锚点上方居中
    const int left = anchor.x - panelW / 2;
    const int top = anchor.y - panelH - params.gap;

    for (int i = 0; i < static_cast<int>(itemCount); ++i)
    {
        const int col = i % cols;
        const int row = i / cols;
        const int x = left + col * (params.iconSize + params.gap);
        const int y = top + row * (params.iconSize + params.gap);
        rects.push_back({ x, y,
            x + params.iconSize, y + params.iconSize });
    }
    return rects;
}

// ── List 布局 ───────────────────────────────────────────────────

struct ListLayoutParams
{
    int iconSize = 24;
    int rowHeight = 32;
    int width = 260;
    int padding = 8;
    int maxRows = 12;
};

inline std::vector<RECT> LayoutList(
    std::size_t itemCount,
    POINT anchor,
    const ListLayoutParams& params = {})
{
    std::vector<RECT> rects;
    const int n = static_cast<int>(
        std::min(itemCount, static_cast<std::size_t>(params.maxRows)));
    if (n <= 0) return rects;

    const int panelH = n * params.rowHeight + params.padding * 2;
    const int left = anchor.x - params.width / 2;
    const int top = anchor.y - panelH - 8;

    for (int i = 0; i < n; ++i)
    {
        const int y = top + params.padding + i * params.rowHeight;
        rects.push_back({
            left + params.padding,
            y,
            left + params.width - params.padding,
            y + params.rowHeight
        });
    }
    return rects;
}

} // namespace snowdesktop::stacks_popup_layout
