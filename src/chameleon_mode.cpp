/**
 * @file chameleon_mode.cpp
 * @brief Chameleon Mode 实现 — 壁纸主色调自动应用到所有 UI 元素
 *
 * 灵感来源：Desktop Fences+ Chameleon Mode
 */

#include "chameleon_mode.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <shlobj.h>

namespace snowdesktop {

std::wstring ChameleonMode::lastWallpaperPath_;
bool ChameleonMode::watching_ = false;
ChameleonPalette ChameleonMode::currentPalette_;
static std::thread watcherThread;
static std::atomic<bool> stopWatcher{false};

void ChameleonMode::Initialize()
{
    // 初次启动时从当前壁纸生成配色
    ChameleonPalette palette = GeneratePalette();
    if (palette.isValid)
    {
        currentPalette_ = palette;
        ApplyPalette(palette);
    }
}

ChameleonPalette ChameleonMode::GeneratePalette()
{
    AccentColor accent = DynamicAccent::ExtractFromCurrentWallpaper();
    if (!accent.isValid)
        return ChameleonPalette{};

    // 根据系统主题选择深色/浅色调色板
    // 默认使用深色模式（与 macOS 桌面风格一致）
    return GenerateDarkPalette(accent);
}

ChameleonPalette ChameleonMode::GenerateDarkPalette(const AccentColor& accent)
{
    ChameleonPalette palette;

    // 提亮 10%，饱和度降低 15%
    AccentColor boosted = accent;
    boosted.saturation = std::clamp(accent.saturation * 0.85f, 0.0f, 1.0f);
    boosted.brightness = std::clamp(accent.brightness + 0.10f, 0.0f, 0.85f);
    DynamicAccent::HSLtoRGB(accent.hue / 360.0f, boosted.saturation,
        boosted.brightness, palette.primary.r, palette.primary.g, palette.primary.b);
    palette.primary.a = 1.0f;

    // 悬停态：再提亮 8%
    float hoverBright = std::clamp(boosted.brightness + 0.08f, 0.0f, 0.95f);
    DynamicAccent::HSLtoRGB(accent.hue / 360.0f, boosted.saturation,
        hoverBright, palette.primaryHover.r, palette.primaryHover.g, palette.primaryHover.b);
    palette.primaryHover.a = 1.0f;

    // 表面色：极低饱和度 + 低亮度 (H: 主色, S: 8%, L: 12%)
    DynamicAccent::HSLtoRGB(accent.hue / 360.0f, 0.08f, 0.12f,
        palette.surface.r, palette.surface.g, palette.surface.b);
    palette.surface.a = 1.0f;

    // 替代表面色：S: 6%, L: 16%
    DynamicAccent::HSLtoRGB(accent.hue / 360.0f, 0.06f, 0.16f,
        palette.surfaceAlt.r, palette.surfaceAlt.g, palette.surfaceAlt.b);
    palette.surfaceAlt.a = 1.0f;

    // 装饰色：保持原饱和度，亮度 +20%
    float accentBright = std::clamp(accent.brightness + 0.20f, 0.0f, 0.85f);
    DynamicAccent::HSLtoRGB(accent.hue / 360.0f, accent.saturation,
        accentBright, palette.accent.r, palette.accent.g, palette.accent.b);
    palette.accent.a = 1.0f;

    palette.isValid = true;
    return palette;
}

ChameleonPalette ChameleonMode::GenerateLightPalette(const AccentColor& accent)
{
    ChameleonPalette palette;

    // 加深 15%，饱和度 +10%
    AccentColor deepened = accent;
    deepened.saturation = std::clamp(accent.saturation * 1.10f, 0.0f, 1.0f);
    deepened.brightness = std::clamp(accent.brightness - 0.15f, 0.15f, 0.70f);
    DynamicAccent::HSLtoRGB(accent.hue / 360.0f, deepened.saturation,
        deepened.brightness, palette.primary.r, palette.primary.g, palette.primary.b);
    palette.primary.a = 1.0f;

    // 悬停态：再加深 5%
    float hoverBright = std::clamp(deepened.brightness - 0.05f, 0.10f, 0.70f);
    DynamicAccent::HSLtoRGB(accent.hue / 360.0f, deepened.saturation,
        hoverBright, palette.primaryHover.r, palette.primaryHover.g, palette.primaryHover.b);
    palette.primaryHover.a = 1.0f;

    // 表面色：S: 5%, L: 97%
    DynamicAccent::HSLtoRGB(accent.hue / 360.0f, 0.05f, 0.97f,
        palette.surface.r, palette.surface.g, palette.surface.b);
    palette.surface.a = 1.0f;

    // 替代表面色：S: 4%, L: 95%
    DynamicAccent::HSLtoRGB(accent.hue / 360.0f, 0.04f, 0.95f,
        palette.surfaceAlt.r, palette.surfaceAlt.g, palette.surfaceAlt.b);
    palette.surfaceAlt.a = 1.0f;

    // 装饰色：亮度 -10%
    float accentBright = std::clamp(accent.brightness - 0.10f, 0.10f, 0.70f);
    DynamicAccent::HSLtoRGB(accent.hue / 360.0f, accent.saturation,
        accentBright, palette.accent.r, palette.accent.g, palette.accent.b);
    palette.accent.a = 1.0f;

    palette.isValid = true;
    return palette;
}

void ChameleonMode::ApplyPalette(const ChameleonPalette& palette)
{
    if (!palette.isValid) return;

    currentPalette_ = palette;

    // 记录当前壁纸路径（用于变化检测）
    lastWallpaperPath_ = DynamicAccent::GetWallpaperPath();

    // 配色应用由主程序通过回调机制处理
    // 这里只更新内部状态
}

void ChameleonMode::StartWallpaperWatcher()
{
    if (watching_) return;
    watching_ = true;
    stopWatcher = false;

    watcherThread = std::thread([]() {
        while (!stopWatcher.load())
        {
            std::this_thread::sleep_for(std::chrono::seconds(5));

            std::wstring currentWallpaper = DynamicAccent::GetWallpaperPath();
            if (currentWallpaper != lastWallpaperPath_ && !currentWallpaper.empty())
            {
                lastWallpaperPath_ = currentWallpaper;
                ChameleonPalette palette = GeneratePalette();
                if (palette.isValid)
                    ApplyPalette(palette);
            }
        }
    });
}

void ChameleonMode::StopWallpaperWatcher()
{
    stopWatcher = true;
    watching_ = false;
    if (watcherThread.joinable())
        watcherThread.join();
}

void ChameleonMode::OnPaletteChange(ChameleonChangeCallback callback)
{
    // 回调机制由主程序集成
    // 这里提供接口，实际调用在主程序中注册
}

} // namespace snowdesktop
