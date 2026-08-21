#pragma once

// ── 窗口平铺吸附算法（macOS Sequoia 风格窗口布局）──────────────────────
//
// 参考来源：MacWindowZone（macOS FancyZones 克隆）、FancyZones（PowerToys）
//
// 设计原则：
//   - 区域以分数坐标存储（0.0~1.0），自动适配任意分辨率/DPI
//   - 每个屏幕独立布局，支持多显示器
//   - 拖拽时显示吸附预览，释放时吸附到最近区域
//   - 窗口记忆：记录窗口上次吸附的区域，下次打开时恢复
//
// 架构位置：L3/L5 层（布局算法），不依赖 DesktopApp，纯函数库。

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

namespace snowdesktop::window_tiling
{

// ── 区域定义 ───────────────────────────────────────────────────

struct FractionalRect
{
    float left = 0.0f;    // 0.0 = 屏幕左边缘
    float top = 0.0f;     // 0.0 = 屏幕上边缘
    float width = 1.0f;   // 1.0 = 屏幕宽度
    float height = 1.0f;  // 1.0 = 屏幕高度
};

/** @brief 将分数区域转换为像素区域。 */
inline RECT ToPixelRect(
    const FractionalRect& frac,
    const RECT& workArea)
{
    const float aw = static_cast<float>(workArea.right - workArea.left);
    const float ah = static_cast<float>(workArea.bottom - workArea.top);
    RECT r{};
    r.left   = workArea.left + static_cast<LONG>(frac.left * aw);
    r.top    = workArea.top  + static_cast<LONG>(frac.top * ah);
    r.right  = r.left + static_cast<LONG>(frac.width * aw);
    r.bottom = r.top + static_cast<LONG>(frac.height * ah);
    return r;
}

// ── 预定义区域模板 ─────────────────────────────────────────────

enum class ZoneLayout
{
    Halves,       // 左右半屏
    Thirds,       // 三等分
    Quarters,     // 四角四等分
    PrimaryFocus, // 左 2/3 + 右 1/3（主焦点布局）
};

/** @brief 返回指定布局下的所有吸附区域。 */
inline std::vector<FractionalRect> GetZonesForLayout(
    ZoneLayout layout)
{
    switch (layout)
    {
    case ZoneLayout::Halves:
        return {
            { 0.0f, 0.0f, 0.5f, 1.0f },  // 左半屏
            { 0.5f, 0.0f, 0.5f, 1.0f },  // 右半屏
        };
    case ZoneLayout::Thirds:
        return {
            { 0.0f, 0.0f, 1.0f / 3.0f, 1.0f },
            { 1.0f / 3.0f, 0.0f, 1.0f / 3.0f, 1.0f },
            { 2.0f / 3.0f, 0.0f, 1.0f / 3.0f, 1.0f },
        };
    case ZoneLayout::Quarters:
        return {
            { 0.0f, 0.0f, 0.5f, 0.5f },  // 左上
            { 0.5f, 0.0f, 0.5f, 0.5f },  // 右上
            { 0.0f, 0.5f, 0.5f, 0.5f },  // 左下
            { 0.5f, 0.5f, 0.5f, 0.5f },  // 右下
        };
    case ZoneLayout::PrimaryFocus:
        return {
            { 0.0f, 0.0f, 2.0f / 3.0f, 1.0f },  // 左 2/3 主焦点
            { 2.0f / 3.0f, 0.0f, 1.0f / 3.0f, 1.0f },  // 右 1/3 副面板
        };
    }
    return {};
}

// ── 吸附计算 ───────────────────────────────────────────────────

/** @brief 计算拖拽点所在的最近区域索引（无匹配返回 -1）。 */
inline int FindNearestZone(
    const std::vector<FractionalRect>& zones,
    const RECT& workArea,
    POINT cursorPos,
    int snapThresholdPx = 30)
{
    int bestIdx = -1;
    float bestDist = 1e9f;
    const float aw = static_cast<float>(workArea.right - workArea.left);
    const float ah = static_cast<float>(workArea.bottom - workArea.top);
    const float cx = static_cast<float>(cursorPos.x - workArea.left) / aw;
    const float cy = static_cast<float>(cursorPos.y - workArea.top) / ah;

    for (size_t i = 0; i < zones.size(); ++i)
    {
        const auto& z = zones[i];
        const float zx = z.left + z.width * 0.5f;
        const float zy = z.top + z.height * 0.5f;
        const float dx = cx - zx;
        const float dy = cy - zy;
        const float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < bestDist)
        {
            bestDist = dist;
            bestIdx = static_cast<int>(i);
        }
    }

    // 仅在屏幕边缘附近（snapThresholdPx 内）激活吸附
    const float threshold = static_cast<float>(snapThresholdPx) /
        std::max(aw, ah);
    if (bestDist > threshold * 2.0f)
        return -1;

    return bestIdx;
}

/** @brief 根据拖拽位置计算吸附目标区域（返回像素矩形）。 */
inline RECT ResolveSnapTarget(
    const std::vector<FractionalRect>& zones,
    const RECT& workArea,
    POINT cursorPos,
    int snapThresholdPx = 30)
{
    const int idx = FindNearestZone(
        zones, workArea, cursorPos, snapThresholdPx);
    if (idx < 0)
        return { 0, 0, 0, 0 };
    return ToPixelRect(zones[idx], workArea);
}

// ── 窗口记忆 ───────────────────────────────────────────────────

struct WindowZoneMemory
{
    FractionalRect zone{};
    int monitorId = 0;
};

inline std::unordered_map<std::string, WindowZoneMemory>&
GetWindowMemory()
{
    static std::unordered_map<std::string, WindowZoneMemory> mem;
    return mem;
}

/** @brief 记录窗口吸附位置。 */
inline void RememberWindowZone(
    const std::string& windowKey,
    const FractionalRect& zone,
    int monitorId)
{
    GetWindowMemory()[windowKey] = { zone, monitorId };
}

/** @brief 查询窗口上次吸附的区域（无记录返回 nullptr）。 */
inline const WindowZoneMemory* RecallWindowZone(
    const std::string& windowKey)
{
    const auto& mem = GetWindowMemory();
    const auto found = mem.find(windowKey);
    return found != mem.end() ? &found->second : nullptr;
}

/** @brief 生成窗口记忆键（进程名 + 窗口标题前缀）。 */
inline std::string MakeWindowKey(
    const std::wstring& processName,
    const std::wstring& windowTitle)
{
    // 取进程名 + 标题前 60 字符
    std::string key;
    key.reserve(128);
    for (wchar_t c : processName)
        key += static_cast<char>(c & 0x7F);
    key += '|';
    for (size_t i = 0; i < std::min<size_t>(60, windowTitle.size()); ++i)
        key += static_cast<char>(windowTitle[i] & 0x7F);
    return key;
}

} // namespace snowdesktop::window_tiling
