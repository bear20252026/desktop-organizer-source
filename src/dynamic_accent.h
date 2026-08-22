/**
 * @file dynamic_accent.h
 * @brief 动态强调色 — 从壁纸提取主色调并应用到主题
 *
 * 灵感来源：Seelen UI 的 Dynamic Accent Color 功能
 *   - 分析当前壁纸图片，提取主要颜色
 *   - 自动应用到主题的 primary 色
 *   - 壁纸切换时自动更新
 *
 * 实现方式：
 *   1. 读取当前壁纸路径（SystemParametersInfo SPI_GETDESKWALLPAPER）
 *   2. 加载壁纸图片（WIC 或 GDI+）
 *   3. 采样主要颜色（简化算法：中心区域平均色 + 饱和度提升）
 *   4. 应用到主题 primary 色
 *
 * 设计原则：
 *   - 提取的颜色自动适配深色/浅色模式
 *   - 颜色变化平滑过渡（不突兀）
 *   - 用户可以手动覆盖（不强制）
 */

#pragma once

#include <string>
#include <windows.h>

namespace snowdesktop {

// 提取的强调色信息
struct AccentColor
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float hue = 0.0f;         // 色相 (0-360)
    float saturation = 0.0f;  // 饱和度 (0-1)
    float brightness = 0.0f;  // 亮度 (0-1)
    bool isValid = false;     // 是否提取成功
};

/**
 * @brief 动态强调色管理器
 */
class DynamicAccent
{
public:
    /**
     * @brief 获取当前壁纸路径
     */
    static std::wstring GetWallpaperPath();

    /**
     * @brief 从壁纸图片提取主要颜色
     * @param wallpaperPath 壁纸图片路径
     * @return 提取的强调色
     *
     * 算法：
     *   1. 加载图片到内存
     *   2. 采样中心 50% 区域的像素
     *   3. 计算平均 RGB 值
     *   4. 提升饱和度 20%（使颜色更鲜明）
     *   5. 确保亮度在合适范围（避免过暗/过亮）
     */
    static AccentColor ExtractFromWallpaper(const std::wstring& wallpaperPath);

    /**
     * @brief 从当前壁纸提取强调色（自动获取壁纸路径）
     */
    static AccentColor ExtractFromCurrentWallpaper();

    /**
     * @brief 应用强调色到主题
     * @param accent 提取的强调色
     *
     * 将提取的颜色应用到主题的 primary/primaryFocus/primaryOnDark 色。
     * 自动适配深色/浅色模式：
     *   - 浅色模式：使用较深的强调色（增加对比度）
     *   - 深色模式：使用较亮的强调色（增加可见性）
     */
    static void ApplyAccentToTheme(const AccentColor& accent);

    /**
     * @brief RGB 转 HSL
     */
    static void RGBtoHSL(float r, float g, float b,
                         float& h, float& s, float& l);

    /**
     * @brief HSL 转 RGB
     */
    static void HSLtoRGB(float h, float s, float l,
                         float& r, float& g, float& b);

    /**
     * @brief 提升颜色饱和度
     */
    static AccentColor BoostSaturation(const AccentColor& color,
                                        float boost = 1.2f);

    /**
     * @brief 调整颜色亮度到合适范围
     */
    static AccentColor AdjustBrightness(const AccentColor& color,
                                         float minBrightness = 0.3f,
                                         float maxBrightness = 0.7f);
};

} // namespace snowdesktop
