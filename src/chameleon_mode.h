/**
 * @file chameleon_mode.h
 * @brief Chameleon Mode — 壁纸主色调自动应用到所有 UI 元素
 *
 * 灵感来源：Desktop Fences+ Chameleon Mode
 *
 * 核心功能：
 *   1. 监听壁纸变化（注册表 HKCU\Control Panel\Desktop\WallPaper）
 *   2. 从壁纸提取主色调（DynamicAccent 模块）
 *   3. 生成和谐配色方案（基于主色调派生 primary/surface/accent）
 *   4. 应用到所有 UI 元素（Dock/菜单栏/围栏/Widget）
 *
 * 配色生成算法（类似 macOS 的 Dynamic Color）：
 *   - 主色调 → primary 色（按钮/强调）
 *   - 主色调降低饱和度 → surface 色（背景）
 *   - 主色调互补色 → accent 色（装饰）
 *   - 深色模式：主色调提亮 + 降低饱和度
 *   - 浅色模式：主色调加深 + 提高对比度
 */

#pragma once

#include "dynamic_accent.h"
#include "theme_engine.h"
#include <string>
#include <functional>

namespace snowdesktop {

// Chameleon 配色方案
struct ChameleonPalette
{
    ThemeColor primary;      // 主色调（按钮/强调）
    ThemeColor primaryHover; // 主色调悬停态（稍亮）
    ThemeColor surface;      // 表面色（背景）
    ThemeColor surfaceAlt;   // 替代表面色（卡片）
    ThemeColor accent;       // 装饰色（装饰/徽章）
    bool isValid = false;
};

// Chameleon 变更回调
using ChameleonChangeCallback = std::function<void(const ChameleonPalette&)>;

/**
 * @brief Chameleon Mode 管理器
 */
class ChameleonMode
{
public:
    /**
     * @brief 初始化 Chameleon Mode
     */
    static void Initialize();

    /**
     * @brief 从当前壁纸生成配色方案
     */
    static ChameleonPalette GeneratePalette();

    /**
     * @brief 生成深色模式配色方案
     *
     * 规则：
     *   - primary: 主色调提亮 10%，饱和度降低 15%
     *   - surface: 主色调极低饱和度 + 低亮度 (H: 主色, S: 8%, L: 12%)
     *   - accent: 主色调保持原饱和度，亮度 +20%
     */
    static ChameleonPalette GenerateDarkPalette(const AccentColor& accent);

    /**
     * @brief 生成浅色模式配色方案
     *
     * 规则：
     *   - primary: 主色调加深 15%，饱和度 +10%
     *   - surface: 主色调极低饱和度 + 高亮度 (H: 主色, S: 5%, L: 97%)
     *   - accent: 主色调保持原饱和度，亮度 -10%
     */
    static ChameleonPalette GenerateLightPalette(const AccentColor& accent);

    /**
     * @brief 应用配色方案到主题
     */
    static void ApplyPalette(const ChameleonPalette& palette);

    /**
     * @brief 启动壁纸变化监听
     *
     * 在后台线程中轮询注册表 Wallpaper 键值，
     * 检测到变化时自动重新生成配色并应用。
     */
    static void StartWallpaperWatcher();

    /**
     * @brief 停止壁纸变化监听
     */
    static void StopWallpaperWatcher();

    /**
     * @brief 注册配色变化回调
     */
    static void OnPaletteChange(ChameleonChangeCallback callback);

private:
    static std::wstring lastWallpaperPath_;
    static bool watching_;
    static ChameleonPalette currentPalette_;
};

} // namespace snowdesktop
