/**
 * @file theme_engine.h
 * @brief CSS-like 主题引擎
 *
 * 提供类似 CSS 的主题定义格式，支持：
 *   1. 颜色变量（类似 CSS custom properties）
 *   2. 组件样式继承（按钮/面板/Dock/菜单栏）
 *   3. 深色/浅色模式切换
 *   4. JSON 主题包导入/导出
 *   5. 运行时主题切换
 *
 * 设计原则：
 *   - 用户可以用 JSON 文件定义完整主题
 *   - 主题可以覆盖任何设计令牌
 *   - 未覆盖的令牌自动回退到 Apple HIG 默认值
 *   - 主题切换即时生效，无需重启
 *
 * 示例主题文件（themes/my-theme.json）：
 * {
 *   "name": "Ocean Blue",
 *   "author": "User",
 *   "version": "1.0",
 *   "colors": {
 *     "primary": "#0077cc",
 *     "canvas": "#0a1628",
 *     "ink": "#e0e8f0"
 *   },
 *   "glass": {
 *     "blurSmall": 12,
 *     "blurMedium": 24,
 *     "blurLarge": 40
 *   },
 *   "radius": {
 *     "sm": 10,
 *     "md": 14,
 *     "lg": 20
 *   },
 *   "components": {
 *     "dock": {
 *       "backgroundColor": "rgba(10, 22, 40, 0.85)",
 *       "cornerRadius": 16
 *     },
 *     "toolbar": {
 *       "height": 32,
 *       "fontSize": 13
 *     }
 *   }
 * }
 */

#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <filesystem>

namespace snowdesktop {

// 主题颜色（RGBA，0.0-1.0 范围）
struct ThemeColor
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    static ThemeColor FromHex(const std::string& hex);
    std::string ToHex() const;
};

// 主题元数据
struct ThemeMetadata
{
    std::string name;
    std::string author;
    std::string version;
    std::string description;
    std::string previewImage;  // 预览图路径
};

// 颜色主题定义
struct ThemeColors
{
    ThemeColor primary;
    ThemeColor primaryFocus;
    ThemeColor primaryOnDark;
    ThemeColor canvas;
    ThemeColor canvasParchment;
    ThemeColor surfacePearl;
    ThemeColor surfaceTile1;
    ThemeColor surfaceTile2;
    ThemeColor surfaceTile3;
    ThemeColor surfaceBlack;
    ThemeColor ink;
    ThemeColor bodyOnDark;
    ThemeColor bodyMuted;
    ThemeColor dividerSoft;
    ThemeColor hairline;
};

// 玻璃材质主题定义
struct ThemeGlass
{
    float blurSmall = 8.0f;
    float blurMedium = 20.0f;
    float blurLarge = 36.0f;
    float saturation = 1.4f;
    float lightAngle = 35.0f;
    float lightIntensityLow = 0.08f;
    float lightIntensityMedium = 0.15f;
    float lightIntensityHigh = 0.25f;
};

// 圆角主题定义
struct ThemeRadius
{
    float xs = 5.0f;
    float sm = 8.0f;
    float md = 11.0f;
    float lg = 18.0f;
};

// 间距主题定义
struct ThemeSpacing
{
    float xxs = 4.0f;
    float xs = 8.0f;
    float sm = 12.0f;
    float md = 17.0f;
    float lg = 24.0f;
    float xl = 32.0f;
    float xxl = 48.0f;
};

// 组件样式定义
struct ComponentStyle
{
    ThemeColor backgroundColor;
    ThemeColor textColor;
    float fontSize = 0.0f;      // 0 = 使用默认
    float cornerRadius = 0.0f;  // 0 = 使用默认
    float padding = 0.0f;       // 0 = 使用默认
    float height = 0.0f;        // 0 = 使用默认
};

// 完整主题定义
struct ThemeDefinition
{
    ThemeMetadata metadata;
    ThemeColors colors;
    ThemeGlass glass;
    ThemeRadius radius;
    ThemeSpacing spacing;
    std::unordered_map<std::string, ComponentStyle> components;

    // 标记哪些字段被用户覆盖（未覆盖的回退到默认值）
    std::unordered_map<std::string, bool> overrides;
};

// 主题变更回调
using ThemeChangeCallback = std::function<void(const ThemeDefinition&)>;

/**
 * @brief 主题引擎
 *
 * 管理主题的加载、切换、保存。
 * 支持 JSON 主题文件和 ZIP 资源包。
 */
class ThemeEngine
{
public:
    ThemeEngine();
    ~ThemeEngine() = default;

    /**
     * @brief 获取主题目录路径
     */
    static std::wstring GetThemesPath();

    /**
     * @brief 加载主题文件
     * @param path JSON 主题文件路径
     * @return 是否加载成功
     */
    bool LoadTheme(const std::wstring& path);

    /**
     * @brief 应用当前主题到 PersonalizationSettings
     *
     * 将主题定义转换为 PersonalizationSettings 结构体，
     * 使现有渲染管线直接使用主题值。
     */
    void ApplyCurrentTheme();

    /**
     * @brief 保存当前主题到文件
     */
    bool SaveCurrentTheme(const std::wstring& path);

    /**
     * @brief 获取当前主题
     */
    const ThemeDefinition& GetCurrentTheme() const { return currentTheme_; }

    /**
     * @brief 获取默认主题（Apple HIG）
     */
    static ThemeDefinition GetDefaultTheme();

    /**
     * @brief 获取深色默认主题
     */
    static ThemeDefinition GetDefaultDarkTheme();

    /**
     * @brief 列出所有可用主题
     */
    std::vector<std::wstring> ListAvailableThemes();

    /**
     * @brief 注册主题变更回调
     */
    void OnThemeChange(ThemeChangeCallback callback);

    /**
     * @brief 从 JSON 字符串解析主题
     */
    static ThemeDefinition ParseThemeJson(const std::string& json);

    /**
     * @brief 将主题序列化为 JSON 字符串
     */
    static std::string SerializeThemeJson(const ThemeDefinition& theme);

private:
    ThemeDefinition currentTheme_;
    std::vector<ThemeChangeCallback> callbacks_;
};

} // namespace snowdesktop
