/**
 * @file theme_engine.cpp
 * @brief CSS-like 主题引擎实现
 *
 * 实现主题的 JSON 解析、加载、保存、切换。
 * 不依赖外部 JSON 库，使用手写解析器（轻量级）。
 */

#include "theme_engine.h"
#include "personalization.h"
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>

namespace snowdesktop {

// ── ThemeColor 实现 ─────────────────────────────────────────────

ThemeColor ThemeColor::FromHex(const std::string& hex)
{
    ThemeColor c{};
    std::string h = hex;
    // 去掉 # 前缀
    if (!h.empty() && h[0] == '#')
        h = h.substr(1);

    auto hexToFloat = [](const std::string& s) -> float {
        if (s.empty()) return 0.0f;
        try {
            return static_cast<float>(std::stoi(s, nullptr, 16)) / 255.0f;
        } catch (...) {
            return 0.0f;
        }
    };

    if (h.length() >= 6)
    {
        c.r = hexToFloat(h.substr(0, 2));
        c.g = hexToFloat(h.substr(2, 2));
        c.b = hexToFloat(h.substr(4, 2));
    }
    if (h.length() >= 8)
        c.a = hexToFloat(h.substr(6, 2));
    else
        c.a = 1.0f;

    return c;
}

std::string ThemeColor::ToHex() const
{
    auto toHex = [](float f) -> std::string {
        int v = static_cast<int>(std::clamp(f, 0.0f, 1.0f) * 255.0f);
        char buf[4]{};
        snprintf(buf, sizeof(buf), "%02x", v);
        return buf;
    };
    return "#" + toHex(r) + toHex(g) + toHex(b);
}

// ── ThemeEngine 实现 ────────────────────────────────────────────

ThemeEngine::ThemeEngine()
{
    currentTheme_ = GetDefaultTheme();
}

std::wstring ThemeEngine::GetThemesPath()
{
    wchar_t appData[MAX_PATH]{};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA,
            nullptr, SHGFP_TYPE_CURRENT, appData)))
    {
        std::wstring dir = std::wstring(appData) + L"\\SnowDesktop\\themes";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir;
    }
    return L"";
}

ThemeDefinition ThemeEngine::GetDefaultTheme()
{
    ThemeDefinition theme;
    theme.metadata.name = "Apple HIG Light";
    theme.metadata.author = "Desktop Organizer";
    theme.metadata.version = "1.0";

    // Apple HIG 浅色主题颜色
    theme.colors.primary = ThemeColor::FromHex("#0066cc");
    theme.colors.primaryFocus = ThemeColor::FromHex("#0071e3");
    theme.colors.primaryOnDark = ThemeColor::FromHex("#2997ff");
    theme.colors.canvas = ThemeColor::FromHex("#ffffff");
    theme.colors.canvasParchment = ThemeColor::FromHex("#f5f5f7");
    theme.colors.surfacePearl = ThemeColor::FromHex("#fafafc");
    theme.colors.surfaceTile1 = ThemeColor::FromHex("#272729");
    theme.colors.surfaceTile2 = ThemeColor::FromHex("#2a2a2c");
    theme.colors.surfaceTile3 = ThemeColor::FromHex("#252527");
    theme.colors.surfaceBlack = ThemeColor::FromHex("#000000");
    theme.colors.ink = ThemeColor::FromHex("#1d1d1f");
    theme.colors.bodyOnDark = ThemeColor::FromHex("#ffffff");
    theme.colors.bodyMuted = ThemeColor::FromHex("#cccccc");
    theme.colors.dividerSoft = ThemeColor::FromHex("#f0f0f0");
    theme.colors.hairline = ThemeColor::FromHex("#e0e0e0");

    return theme;
}

ThemeDefinition ThemeEngine::GetDefaultDarkTheme()
{
    ThemeDefinition theme;
    theme.metadata.name = "Apple HIG Dark";
    theme.metadata.author = "Desktop Organizer";
    theme.metadata.version = "1.0";

    theme.colors.primary = ThemeColor::FromHex("#2997ff");
    theme.colors.primaryFocus = ThemeColor::FromHex("#2ea3ff");
    theme.colors.primaryOnDark = ThemeColor::FromHex("#2997ff");
    theme.colors.canvas = ThemeColor::FromHex("#1d1d1f");
    theme.colors.canvasParchment = ThemeColor::FromHex("#272729");
    theme.colors.surfacePearl = ThemeColor::FromHex("#2a2a2c");
    theme.colors.surfaceTile1 = ThemeColor::FromHex("#17171a");
    theme.colors.surfaceTile2 = ThemeColor::FromHex("#1c1c1e");
    theme.colors.surfaceTile3 = ThemeColor::FromHex("#121213");
    theme.colors.surfaceBlack = ThemeColor::FromHex("#000000");
    theme.colors.ink = ThemeColor::FromHex("#f2f2f2");
    theme.colors.bodyOnDark = ThemeColor::FromHex("#f2f2f2");
    theme.colors.bodyMuted = ThemeColor::FromHex("#8c8c8c");
    theme.colors.dividerSoft = ThemeColor::FromHex("#38383a");
    theme.colors.hairline = ThemeColor::FromHex("#4d4d50");

    return theme;
}

// ── JSON 解析（轻量级，不依赖外部库）───────────────────────────

// 辅助：从 JSON 字符串中提取指定 key 的字符串值
static std::string ExtractString(const std::string& json, const std::string& key)
{
    std::string search = "\"" + key + "\"";
    auto pos = json.find(search);
    if (pos == std::string::npos) return "";
    auto colon = json.find(':', pos + search.length());
    if (colon == std::string::npos) return "";
    auto start = json.find('"', colon + 1);
    if (start == std::string::npos) return "";
    start++;
    auto end = json.find('"', start);
    if (end == std::string::npos) return "";
    return json.substr(start, end - start);
}

// 辅助：从 JSON 字符串中提取指定 key 的数值
static float ExtractFloat(const std::string& json, const std::string& key, float defaultVal)
{
    std::string search = "\"" + key + "\"";
    auto pos = json.find(search);
    if (pos == std::string::npos) return defaultVal;
    auto colon = json.find(':', pos + search.length());
    if (colon == std::string::npos) return defaultVal;
    // 跳过空格
    auto start = colon + 1;
    while (start < json.length() && (json[start] == ' ' || json[start] == '\t'))
        start++;
    // 读取数值
    auto end = start;
    while (end < json.length() && (isdigit(json[end]) || json[end] == '.' || json[end] == '-'))
        end++;
    if (end == start) return defaultVal;
    try {
        return std::stof(json.substr(start, end - start));
    } catch (...) {
        return defaultVal;
    }
}

// 辅助：提取子对象
static std::string ExtractObject(const std::string& json, const std::string& key)
{
    std::string search = "\"" + key + "\"";
    auto pos = json.find(search);
    if (pos == std::string::npos) return "";
    auto colon = json.find(':', pos + search.length());
    if (colon == std::string::npos) return "";
    auto start = json.find('{', colon);
    if (start == std::string::npos) return "";
    int depth = 1;
    auto end = start + 1;
    while (end < json.length() && depth > 0)
    {
        if (json[end] == '{') depth++;
        else if (json[end] == '}') depth--;
        end++;
    }
    return json.substr(start, end - start);
}

ThemeDefinition ThemeEngine::ParseThemeJson(const std::string& json)
{
    ThemeDefinition theme;

    // 元数据
    theme.metadata.name = ExtractString(json, "name");
    theme.metadata.author = ExtractString(json, "author");
    theme.metadata.version = ExtractString(json, "version");
    theme.metadata.description = ExtractString(json, "description");

    // 颜色
    std::string colorsObj = ExtractObject(json, "colors");
    if (!colorsObj.empty())
    {
        theme.colors.primary = ThemeColor::FromHex(ExtractString(colorsObj, "primary"));
        theme.colors.primaryFocus = ThemeColor::FromHex(ExtractString(colorsObj, "primary-focus"));
        theme.colors.primaryOnDark = ThemeColor::FromHex(ExtractString(colorsObj, "primary-on-dark"));
        theme.colors.canvas = ThemeColor::FromHex(ExtractString(colorsObj, "canvas"));
        theme.colors.canvasParchment = ThemeColor::FromHex(ExtractString(colorsObj, "canvas-parchment"));
        theme.colors.surfacePearl = ThemeColor::FromHex(ExtractString(colorsObj, "surface-pearl"));
        theme.colors.surfaceTile1 = ThemeColor::FromHex(ExtractString(colorsObj, "surface-tile-1"));
        theme.colors.surfaceTile2 = ThemeColor::FromHex(ExtractString(colorsObj, "surface-tile-2"));
        theme.colors.surfaceTile3 = ThemeColor::FromHex(ExtractString(colorsObj, "surface-tile-3"));
        theme.colors.surfaceBlack = ThemeColor::FromHex(ExtractString(colorsObj, "surface-black"));
        theme.colors.ink = ThemeColor::FromHex(ExtractString(colorsObj, "ink"));
        theme.colors.bodyOnDark = ThemeColor::FromHex(ExtractString(colorsObj, "body-on-dark"));
        theme.colors.bodyMuted = ThemeColor::FromHex(ExtractString(colorsObj, "body-muted"));
        theme.colors.dividerSoft = ThemeColor::FromHex(ExtractString(colorsObj, "divider-soft"));
        theme.colors.hairline = ThemeColor::FromHex(ExtractString(colorsObj, "hairline"));
        theme.overrides["colors"] = true;
    }

    // 玻璃材质
    std::string glassObj = ExtractObject(json, "glass");
    if (!glassObj.empty())
    {
        theme.glass.blurSmall = ExtractFloat(glassObj, "blurSmall", 8.0f);
        theme.glass.blurMedium = ExtractFloat(glassObj, "blurMedium", 20.0f);
        theme.glass.blurLarge = ExtractFloat(glassObj, "blurLarge", 36.0f);
        theme.glass.saturation = ExtractFloat(glassObj, "saturation", 1.4f);
        theme.glass.lightAngle = ExtractFloat(glassObj, "lightAngle", 35.0f);
        theme.overrides["glass"] = true;
    }

    // 圆角
    std::string radiusObj = ExtractObject(json, "radius");
    if (!radiusObj.empty())
    {
        theme.radius.xs = ExtractFloat(radiusObj, "xs", 5.0f);
        theme.radius.sm = ExtractFloat(radiusObj, "sm", 8.0f);
        theme.radius.md = ExtractFloat(radiusObj, "md", 11.0f);
        theme.radius.lg = ExtractFloat(radiusObj, "lg", 18.0f);
        theme.overrides["radius"] = true;
    }

    // 间距
    std::string spacingObj = ExtractObject(json, "spacing");
    if (!spacingObj.empty())
    {
        theme.spacing.xs = ExtractFloat(spacingObj, "xs", 8.0f);
        theme.spacing.sm = ExtractFloat(spacingObj, "sm", 12.0f);
        theme.spacing.md = ExtractFloat(spacingObj, "md", 17.0f);
        theme.spacing.lg = ExtractFloat(spacingObj, "lg", 24.0f);
        theme.spacing.xl = ExtractFloat(spacingObj, "xl", 32.0f);
        theme.overrides["spacing"] = true;
    }

    return theme;
}

std::string ThemeEngine::SerializeThemeJson(const ThemeDefinition& theme)
{
    auto colorToJson = [](const std::string& key, const ThemeColor& c) -> std::string {
        return "    \"" + key + "\": \"" + c.ToHex() + "\"";
    };

    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"name\": \"" << theme.metadata.name << "\",\n";
    ss << "  \"author\": \"" << theme.metadata.author << "\",\n";
    ss << "  \"version\": \"" << theme.metadata.version << "\",\n";
    if (!theme.metadata.description.empty())
        ss << "  \"description\": \"" << theme.metadata.description << "\",\n";

    ss << "  \"colors\": {\n";
    ss << colorToJson("primary", theme.colors.primary) << ",\n";
    ss << colorToJson("primary-focus", theme.colors.primaryFocus) << ",\n";
    ss << colorToJson("primary-on-dark", theme.colors.primaryOnDark) << ",\n";
    ss << colorToJson("canvas", theme.colors.canvas) << ",\n";
    ss << colorToJson("canvas-parchment", theme.colors.canvasParchment) << ",\n";
    ss << colorToJson("surface-pearl", theme.colors.surfacePearl) << ",\n";
    ss << colorToJson("surface-tile-1", theme.colors.surfaceTile1) << ",\n";
    ss << colorToJson("surface-tile-2", theme.colors.surfaceTile2) << ",\n";
    ss << colorToJson("surface-tile-3", theme.colors.surfaceTile3) << ",\n";
    ss << colorToJson("surface-black", theme.colors.surfaceBlack) << ",\n";
    ss << colorToJson("ink", theme.colors.ink) << ",\n";
    ss << colorToJson("body-on-dark", theme.colors.bodyOnDark) << ",\n";
    ss << colorToJson("body-muted", theme.colors.bodyMuted) << ",\n";
    ss << colorToJson("divider-soft", theme.colors.dividerSoft) << ",\n";
    ss << colorToJson("hairline", theme.colors.hairline) << "\n";
    ss << "  },\n";

    ss << "  \"glass\": {\n";
    ss << "    \"blurSmall\": " << theme.glass.blurSmall << ",\n";
    ss << "    \"blurMedium\": " << theme.glass.blurMedium << ",\n";
    ss << "    \"blurLarge\": " << theme.glass.blurLarge << ",\n";
    ss << "    \"saturation\": " << theme.glass.saturation << ",\n";
    ss << "    \"lightAngle\": " << theme.glass.lightAngle << "\n";
    ss << "  },\n";

    ss << "  \"radius\": {\n";
    ss << "    \"xs\": " << theme.radius.xs << ",\n";
    ss << "    \"sm\": " << theme.radius.sm << ",\n";
    ss << "    \"md\": " << theme.radius.md << ",\n";
    ss << "    \"lg\": " << theme.radius.lg << "\n";
    ss << "  },\n";

    ss << "  \"spacing\": {\n";
    ss << "    \"xs\": " << theme.spacing.xs << ",\n";
    ss << "    \"sm\": " << theme.spacing.sm << ",\n";
    ss << "    \"md\": " << theme.spacing.md << ",\n";
    ss << "    \"lg\": " << theme.spacing.lg << ",\n";
    ss << "    \"xl\": " << theme.spacing.xl << "\n";
    ss << "  }\n";
    ss << "}\n";

    return ss.str();
}

bool ThemeEngine::LoadTheme(const std::wstring& path)
{
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    currentTheme_ = ParseThemeJson(json);

    // 通知所有回调
    for (auto& cb : callbacks_)
        cb(currentTheme_);

    return true;
}

void ThemeEngine::ApplyCurrentTheme()
{
    // 桥接：将 ThemeDefinition 颜色映射到 PersonalizationSettings 字段
    // 使现有渲染管线（通过 settingsWindow_->GetPersonalization()）直接使用主题值
    PersonalizationSettings p;
    p.widgetBgR = currentTheme_.colors.surfaceTile1.r;
    p.widgetBgG = currentTheme_.colors.surfaceTile1.g;
    p.widgetBgB = currentTheme_.colors.surfaceTile1.b;
    p.widgetAlpha = currentTheme_.colors.surfaceTile1.a;
    p.widgetBorderR = currentTheme_.colors.hairline.r;
    p.widgetBorderG = currentTheme_.colors.hairline.g;
    p.widgetBorderB = currentTheme_.colors.hairline.b;
    p.widgetBorderAlpha = currentTheme_.colors.hairline.a;
    p.cornerRadius = currentTheme_.radius.md;
    p.glassBlurRadius = currentTheme_.glass.blurMedium;
    p.glassEnabled = true;
    p.acrylicEnabled = true;

    // 通知所有注册的回调（主程序通过回调接收新设置并刷新渲染）
    for (auto& cb : callbacks_)
        cb(currentTheme_);
}

bool ThemeEngine::SaveCurrentTheme(const std::wstring& path)
{
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << SerializeThemeJson(currentTheme_);
    return true;
}

std::vector<std::wstring> ThemeEngine::ListAvailableThemes()
{
    std::vector<std::wstring> themes;
    std::wstring themesPath = GetThemesPath();
    if (themesPath.empty()) return themes;

    for (const auto& entry : std::filesystem::directory_iterator(themesPath))
    {
        if (entry.is_regular_file() && entry.path().extension() == L".json")
            themes.push_back(entry.path().wstring());
    }
    return themes;
}

void ThemeEngine::OnThemeChange(ThemeChangeCallback callback)
{
    callbacks_.push_back(std::move(callback));
}

} // namespace snowdesktop
