#pragma once

// ── 苹果设计令牌系统（Apple HIG Design Tokens）────────────────────────
//
// 参考来源：Apple Human Interface Guidelines + DESIGN.md（awesome-design-md）
//
// 本文件将苹果设计语言的核心规范转化为 C++ 常量，供整个项目统一使用。
// 遵循苹果设计原则：
//   - 单一光源模型（左上 35°，低/中/高三档亮度）
//   - 同心圆角系统（Fixed/Capsule/Concentric）
//   - 统一间距基数（8px）
//   - Liquid Glass 三层材质（Regular/Clear/Frosted）
//   - 可访问性自适应（减少透明度/增强对比/减少动画）
//
// 架构位置：L3 基础层（设计常量），不依赖任何模块，纯头文件。

#include <d2d1.h>
#include <windows.h>

namespace snowdesktop::design_tokens
{

// ── 颜色令牌（Color Tokens）────────────────────────────────────
// 苹果设计系统语义颜色，深色/浅色模式各一套

struct ColorTokens
{
    // 品牌与强调色
    D2D1_COLOR_F primary;        // #0066cc Action Blue — 所有交互元素
    D2D1_COLOR_F primaryFocus;   // #0071e3 Focus Blue — 键盘焦点环
    D2D1_COLOR_F primaryOnDark;  // #2997ff Sky Link Blue — 暗色表面链接

    // 表面色
    D2D1_COLOR_F canvas;         // #ffffff 纯白画布
    D2D1_COLOR_F canvasParchment;// #f5f5f7 羊皮纸白（交替浅色区块）
    D2D1_COLOR_F surfacePearl;   // #fafafc 珍珠按钮底色
    D2D1_COLOR_F surfaceTile1;   // #272729 近黑区块 1
    D2D1_COLOR_F surfaceTile2;   // #2a2a2c 近黑区块 2（微亮）
    D2D1_COLOR_F surfaceTile3;   // #252527 近黑区块 3（微暗）
    D2D1_COLOR_F surfaceBlack;   // #000000 纯黑（视频/覆盖层）

    // 文本色
    D2D1_COLOR_F ink;            // #1d1d1f 近黑墨水色（标题/正文）
    D2D1_COLOR_F bodyOnDark;     // #ffffff 暗色表面文本
    D2D1_COLOR_F bodyMuted;      // #cccccc 暗色表面次要文本
    D2D1_COLOR_F inkMuted80;     // #333333 珍珠按钮文本
    D2D1_COLOR_F inkMuted48;     // #7a7a7a 禁用文本/法律细字

    // 分隔线与边框
    D2D1_COLOR_F dividerSoft;    // #f0f0f0 柔分隔线
    D2D1_COLOR_F hairline;       // #e0e0e0 细发丝边框

    // 状态色
    D2D1_COLOR_F statusSuccess;  // #03a10e 绿色成功
    D2D1_COLOR_F statusError;    // #e30000 红色错误
    D2D1_COLOR_F statusWarning;  // #f56300 橙色警告
};

inline const ColorTokens kLightTheme = {
    /* primary */        {0.0f, 0.40f, 0.80f, 1.0f},  // #0066cc
    /* primaryFocus */   {0.0f, 0.44f, 0.89f, 1.0f},  // #0071e3
    /* primaryOnDark */  {0.16f, 0.60f, 1.0f, 1.0f},  // #2997ff
    /* canvas */         {1.0f, 1.0f, 1.0f, 1.0f},    // #ffffff
    /* canvasParchment */{0.96f, 0.96f, 0.97f, 1.0f},  // #f5f5f7
    /* surfacePearl */   {0.98f, 0.98f, 0.99f, 1.0f},  // #fafafc
    /* surfaceTile1 */   {0.15f, 0.15f, 0.16f, 1.0f},  // #272729
    /* surfaceTile2 */   {0.16f, 0.16f, 0.17f, 1.0f},  // #2a2a2c
    /* surfaceTile3 */   {0.14f, 0.14f, 0.15f, 1.0f},  // #252527
    /* surfaceBlack */   {0.0f, 0.0f, 0.0f, 1.0f},     // #000000
    /* ink */            {0.11f, 0.11f, 0.12f, 1.0f},   // #1d1d1f
    /* bodyOnDark */     {1.0f, 1.0f, 1.0f, 1.0f},     // #ffffff
    /* bodyMuted */      {0.80f, 0.80f, 0.80f, 1.0f},  // #cccccc
    /* inkMuted80 */     {0.20f, 0.20f, 0.20f, 1.0f},  // #333333
    /* inkMuted48 */     {0.48f, 0.48f, 0.48f, 1.0f},  // #7a7a7a
    /* dividerSoft */    {0.94f, 0.94f, 0.94f, 1.0f},  // #f0f0f0
    /* hairline */       {0.88f, 0.88f, 0.88f, 1.0f},  // #e0e0e0
    /* statusSuccess */  {0.01f, 0.63f, 0.05f, 1.0f},  // #03a10e
    /* statusError */    {0.89f, 0.0f, 0.0f, 1.0f},    // #e30000
    /* statusWarning */  {0.96f, 0.39f, 0.0f, 1.0f},   // #f56300
};

inline const ColorTokens kDarkTheme = {
    /* primary */        {0.16f, 0.60f, 1.0f, 1.0f},   // #2997ff Sky Link Blue
    /* primaryFocus */   {0.18f, 0.63f, 1.0f, 1.0f},   // 稍亮
    /* primaryOnDark */  {0.16f, 0.60f, 1.0f, 1.0f},   // #2997ff
    /* canvas */         {0.11f, 0.11f, 0.12f, 1.0f},   // #1d1d1f 近黑
    /* canvasParchment */{0.15f, 0.15f, 0.16f, 1.0f},   // #272729
    /* surfacePearl */   {0.16f, 0.16f, 0.17f, 1.0f},   // #2a2a2c
    /* surfaceTile1 */   {0.09f, 0.09f, 0.10f, 1.0f},   // #17171a
    /* surfaceTile2 */   {0.11f, 0.11f, 0.12f, 1.0f},   // #1c1c1e
    /* surfaceTile3 */   {0.07f, 0.07f, 0.08f, 1.0f},   // #121213
    /* surfaceBlack */   {0.0f, 0.0f, 0.0f, 1.0f},      // #000000
    /* ink */            {0.95f, 0.95f, 0.95f, 1.0f},   // #f2f2f2 浅灰白
    /* bodyOnDark */     {0.95f, 0.95f, 0.95f, 1.0f},   // #f2f2f2
    /* bodyMuted */      {0.55f, 0.55f, 0.55f, 1.0f},   // #8c8c8c
    /* inkMuted80 */     {0.80f, 0.80f, 0.80f, 1.0f},   // #cccccc
    /* inkMuted48 */     {0.48f, 0.48f, 0.48f, 1.0f},   // #7a7a7a
    /* dividerSoft */    {0.22f, 0.22f, 0.23f, 1.0f},   // #38383a
    /* hairline */       {0.30f, 0.30f, 0.31f, 1.0f},   // #4d4d50
    /* statusSuccess */  {0.18f, 0.84f, 0.29f, 1.0f},   // 绿色（暗色主题更亮）
    /* statusError */    {1.0f, 0.27f, 0.27f, 1.0f},    // 红色（暗色主题更亮）
    /* statusWarning */  {1.0f, 0.58f, 0.0f, 1.0f},     // 橙色（暗色主题更亮）
};

// ── 间距令牌（Spacing Tokens）────────────────────────────────
// 苹果设计系统 8px 基数间距

struct SpacingTokens
{
    int xxs;      // 4px
    int xs;       // 8px  基数单位
    int sm;       // 12px
    int md;       // 17px 苹果标准正文行距
    int lg;       // 24px
    int xl;       // 32px
    int xxl;      // 48px
    int section;  // 80px 段落间距
};

inline const SpacingTokens kSpacing = {
    4, 8, 12, 17, 24, 32, 48, 80
};

// ── 圆角令牌（Corner Radius Tokens）───────────────────────────
// 苹果同心圆角系统：Fixed（固定值）/ Capsule（半高）/ Concentric（父级减内边距）

struct RadiusTokens
{
    float none;    // 0px
    float xs;      // 5px  小控件
    float sm;      // 8px  按钮/控件
    float md;      // 11px 卡片/面板
    float lg;      // 18px 大卡片/弹窗
    float pill;    // 9999px 胶囊形状
};

inline const RadiusTokens kRadius = {
    0.0f, 5.0f, 8.0f, 11.0f, 18.0f, 9999.0f
};

// ── 阴影令牌（Elevation Tokens）───────────────────────────────
// 苹果设计：极少使用阴影，仅在产品图像需要"悬浮"时使用一个签名阴影

struct ElevationTokens
{
    // 苹果签名阴影：产品图像在表面上的唯一投影
    // rgba(0, 0, 0, 0.22) 3px 5px 30px
    struct Shadow {
        float offsetX;
        float offsetY;
        float blur;
        float spread;
        D2D1_COLOR_F color;
    };
    Shadow productImage;  // 产品图像悬浮阴影
    Shadow subtle;        // 微妙提升（UI 元素）
};

inline const ElevationTokens kElevation = {
    /* productImage */ {3.0f, 5.0f, 30.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.22f}},
    /* subtle */       {0.0f, 2.0f, 10.0f, 0.0f, {0.0f, 0.0f, 0.0f, 0.10f}},
};

// ── 运动令牌（Motion Tokens）──────────────────────────────────
// 苹果设计系统动画参数

struct MotionTokens
{
    float standardDurationMs;   // 300ms 标准过渡
    float springMass;           // 1.0 弹簧质量
    float springStiffness;      // 100.0 弹簧刚度
    float springDamping;        // 15.0 弹簧阻尼
};

inline const MotionTokens kMotion = {
    300.0f, 1.0f, 100.0f, 15.0f
};

// ── Liquid Glass 材质令牌 ─────────────────────────────────────
// 苹果 Liquid Glass 三层材质系统

enum class GlassVariant
{
    Regular,   // 默认：模糊+亮度调整，保持文本可读性
    Clear,     // 高透明：用于媒体背景上的控件
    Frosted,   // 磨砂：减少透明度可访问性模式
};

struct GlassTokens
{
    // 模糊半径（按元素角色分级）
    float blurSmall;     // 8px  小控件（按钮、芯片）
    float blurMedium;    // 20px 标准面板
    float blurLarge;     // 36px 英雄/覆盖层

    // 饱和度增强
    float saturation;    // 1.4  Liquid Glass 饱和度倍数

    // 光源模型（单一锁定：左上 35°）
    float lightAngleDeg; // 35.0 光源角度
    float lightIntensityLow;   // 0.08 低档（图标）
    float lightIntensityMedium;// 0.15 中档（按钮/菜单）
    float lightIntensityHigh;  // 0.25 高档（活跃/聚焦状态）
};

inline const GlassTokens kGlass = {
    /* blurSmall */     8.0f,
    /* blurMedium */    20.0f,
    /* blurLarge */     36.0f,
    /* saturation */    1.4f,
    /* lightAngleDeg */ 35.0f,
    /* lightIntensityLow */    0.08f,
    /* lightIntensityMedium */ 0.15f,
    /* lightIntensityHigh */   0.25f,
};

// ── 排版令牌（Typography Tokens）─────────────────────────────
// 苹果 SF Pro 排版系统：负字间距是标志性特征

struct TypographyToken
{
    float fontSize;
    float fontWeight;     // 300/400/600/700（苹果不用 500）
    float lineHeight;     // 行高倍数
    float letterSpacing;  // 字间距（负值=收紧，苹果特征）
};

struct TypographyTokens
{
    TypographyToken heroDisplay;   // 56px/600/1.07/-0.28px 英雄标题
    TypographyToken displayLg;     // 40px/600/1.10/0 区块标题
    TypographyToken displayMd;     // 34px/600/1.47/-0.374px 段落标题
    TypographyToken lead;          // 28px/400/1.14/0.196px 产品副文本
    TypographyToken leadAiry;      // 24px/300/1.5/0 轻盈段落
    TypographyToken tagline;       // 21px/600/1.19/0.231px 副标题
    TypographyToken bodyStrong;    // 17px/600/1.24/-0.374px 强调正文
    TypographyToken body;          // 17px/400/1.47/-0.374px 默认正文（苹果用 17px 不是 16px）
    TypographyToken caption;       // 14px/400/1.43/-0.224px 说明文字
    TypographyToken captionStrong; // 14px/600/1.29/-0.224px 强调说明
    TypographyToken buttonLarge;   // 18px/300/1.0/0 大按钮（苹果用 300 轻字重）
    TypographyToken buttonUtility; // 14px/400/1.29/-0.224px 工具按钮
    TypographyToken finePrint;     // 12px/400/1.0/-0.12px 细字
    TypographyToken navLink;       // 12px/400/1.0/-0.12px 导航链接
};

inline const TypographyTokens kTypography = {
    /* heroDisplay */   {56.0f, 600.0f, 1.07f, -0.28f},
    /* displayLg */     {40.0f, 600.0f, 1.10f, 0.0f},
    /* displayMd */     {34.0f, 600.0f, 1.47f, -0.374f},
    /* lead */          {28.0f, 400.0f, 1.14f, 0.196f},
    /* leadAiry */      {24.0f, 300.0f, 1.5f, 0.0f},
    /* tagline */       {21.0f, 600.0f, 1.19f, 0.231f},
    /* bodyStrong */    {17.0f, 600.0f, 1.24f, -0.374f},
    /* body */          {17.0f, 400.0f, 1.47f, -0.374f},
    /* caption */       {14.0f, 400.0f, 1.43f, -0.224f},
    /* captionStrong */ {14.0f, 600.0f, 1.29f, -0.224f},
    /* buttonLarge */   {18.0f, 300.0f, 1.0f, 0.0f},
    /* buttonUtility */ {14.0f, 400.0f, 1.29f, -0.224f},
    /* finePrint */     {12.0f, 400.0f, 1.0f, -0.12f},
    /* navLink */       {12.0f, 400.0f, 1.0f, -0.12f},
};

// ── 组件级样式令牌（Component Tokens）────────────────────────────
// Apple HIG DESIGN.md 组件规范：每个组件是颜色/排版/圆角/间距的组合

struct ComponentStyle
{
    D2D1_COLOR_F backgroundColor;
    D2D1_COLOR_F textColor;
    float fontSize;
    float fontWeight;
    float cornerRadius;
    float padH;  // 水平内边距
    float padV;  // 垂直内边距
};

struct ComponentTokens
{
    ComponentStyle buttonPrimary;       // 主按钮：primary底色 + pill圆角
    ComponentStyle buttonSecondaryPill; // 次要胶囊：canvas底色 + primary文字
    ComponentStyle buttonDarkUtility;   // 暗色工具按钮：ink底色 + onDark文字
    ComponentStyle buttonPearlCapsule;  // 珍珠胶囊：surfacePearl底色 + ink-muted-80文字
    ComponentStyle buttonIconCircular;  // 圆形图标按钮：surface-chip-translucent底色
    ComponentStyle textLink;            // 文字链接：透明底色 + primary文字
    ComponentStyle searchInput;         // 搜索输入：canvas底色 + pill圆角 + body字号
    ComponentStyle card;                // 通用卡片：canvas底色 + lg圆角 + body-strong字号
    ComponentStyle globalNav;           // 全局导航栏：surface-black底色 + onDark文字 + nav-link字号
    ComponentStyle subNavFrosted;       // 子导航（毛玻璃）：canvas-parchment底色 + ink文字
};

inline const ComponentTokens kComponents = {
    /* buttonPrimary */
    {{0.0f, 0.40f, 0.80f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
     17.0f, 400.0f, 9999.0f, 22.0f, 11.0f},
    /* buttonSecondaryPill */
    {{1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.40f, 0.80f, 1.0f},
     17.0f, 400.0f, 9999.0f, 22.0f, 11.0f},
    /* buttonDarkUtility */
    {{0.11f, 0.11f, 0.12f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
     14.0f, 400.0f, 8.0f, 15.0f, 8.0f},
    /* buttonPearlCapsule */
    {{0.98f, 0.98f, 0.99f, 1.0f}, {0.20f, 0.20f, 0.20f, 1.0f},
     14.0f, 400.0f, 11.0f, 14.0f, 8.0f},
    /* buttonIconCircular */
    {{0.82f, 0.82f, 0.84f, 1.0f}, {0.11f, 0.11f, 0.12f, 1.0f},
     0.0f, 0.0f, 9999.0f, 0.0f, 0.0f},
    /* textLink */
    {{0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.40f, 0.80f, 1.0f},
     17.0f, 400.0f, 0.0f, 0.0f, 0.0f},
    /* searchInput */
    {{1.0f, 1.0f, 1.0f, 1.0f}, {0.11f, 0.11f, 0.12f, 1.0f},
     17.0f, 400.0f, 9999.0f, 20.0f, 12.0f},
    /* card */
    {{1.0f, 1.0f, 1.0f, 1.0f}, {0.11f, 0.11f, 0.12f, 1.0f},
     17.0f, 600.0f, 18.0f, 24.0f, 24.0f},
    /* globalNav */
    {{0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
     12.0f, 400.0f, 0.0f, 0.0f, 0.0f},
    /* subNavFrosted */
    {{0.96f, 0.96f, 0.97f, 1.0f}, {0.11f, 0.11f, 0.12f, 1.0f},
     21.0f, 600.0f, 0.0f, 0.0f, 0.0f},
};

/** @brief 获取当前主题的组件令牌。深色模式适配由调用方根据 GetColorTokens() 处理。 */
inline const ComponentTokens& GetComponentTokens()
{
    return kComponents;
}

// ── 可访问性令牌（Accessibility Tokens）────────────────────────
// 苹果设计：当用户开启系统无障碍设置时自动适配

struct AccessibilityTokens
{
    bool reducedTransparency;  // 减少透明度 → 玻璃变为磨砂
    bool increasedContrast;    // 增强对比 → 元素变黑白+锐边框
    bool reducedMotion;        // 减少动画 → 过渡变为线性
};

inline AccessibilityTokens gAccessibility = {
    false, false, false
};

struct ThemeState
{
    bool isDarkMode = false;
    bool initialized = false;
};

inline ThemeState gThemeState;

/** @brief 检测 Windows 系统是否处于深色模式。 */
inline bool IsSystemDarkMode()
{
    HKEY hKey{};
    DWORD appsUseLightTheme = 1; // 默认浅色
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD size = sizeof(appsUseLightTheme);
        RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr,
            nullptr, reinterpret_cast<LPBYTE>(&appsUseLightTheme), &size);
        RegCloseKey(hKey);
    }
    return appsUseLightTheme == 0;
}

/** @brief 刷新系统主题状态（深色/浅色）。 */
inline void RefreshThemeState()
{
    gThemeState.isDarkMode = IsSystemDarkMode();
    gThemeState.initialized = true;
}

// ── 便捷访问函数 ──────────────────────────────────────────────

inline const ColorTokens& GetColorTokens()
{
    if (!gThemeState.initialized)
        RefreshThemeState();
    return gThemeState.isDarkMode ? kDarkTheme : kLightTheme;
}

inline const SpacingTokens& GetSpacing()
{
    return kSpacing;
}

inline const RadiusTokens& GetRadius()
{
    return kRadius;
}

inline const ElevationTokens& GetElevation()
{
    return kElevation;
}

inline const MotionTokens& GetMotion()
{
    return kMotion;
}

inline const GlassTokens& GetGlass()
{
    return kGlass;
}

inline const TypographyTokens& GetTypography()
{
    return kTypography;
}

// ── Dynamic Type（动态排版）────────────────────────────────────
// Apple HIG：系统字号变化时，所有组件自动调整排版。
// Windows 对应：系统 DPI 缩放 + 非客户端区字号。
// 本机制读取系统字号缩放因子，返回按比例调整后的排版令牌。

struct DynamicTypeState
{
    float scaleFactor = 1.0f;  // 系统字号缩放（1.0 = 标准）
    bool initialized = false;
};

inline DynamicTypeState gDynamicType;

/** @brief 读取系统字号缩放因子（Windows 系统 DPI / 字号设置）。 */
inline float GetSystemFontScaleFactor()
{
    // 方法1: 读取注册表 HKCU\Software\Microsoft\Accessibility\TextScaleFactor
    HKEY hKey{};
    DWORD textScale = 100; // 默认 100%
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Accessibility",
            0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD size = sizeof(textScale);
        RegQueryValueExW(hKey, L"TextScaleFactor", nullptr,
            nullptr, reinterpret_cast<LPBYTE>(&textScale), &size);
        RegCloseKey(hKey);
    }
    // Windows 11: TextScaleFactor 范围 100-200（百分比）
    return static_cast<float>(textScale) / 100.0f;
}

/** @brief 刷新 Dynamic Type 状态（在应用启动时和系统设置变化时调用）。 */
inline void RefreshDynamicType()
{
    gDynamicType.scaleFactor = GetSystemFontScaleFactor();
    gDynamicType.initialized = true;
}

/** @brief 获取缩放后的字号（考虑 Dynamic Type）。 */
inline float GetScaledFontSize(float baseFontSize)
{
    if (!gDynamicType.initialized)
        RefreshDynamicType();
    return baseFontSize * gDynamicType.scaleFactor;
}

/** @brief 获取缩放后的排版令牌（整套）。 */
inline TypographyToken ScaleTypography(const TypographyToken& base)
{
    if (!gDynamicType.initialized)
        RefreshDynamicType();
    TypographyToken scaled = base;
    scaled.fontSize = base.fontSize * gDynamicType.scaleFactor;
    // 行高倍数不变（是相对值），字间距按字号等比缩放
    scaled.letterSpacing = base.letterSpacing * gDynamicType.scaleFactor;
    return scaled;
}

/** @brief 圆角胶囊形状（Capsule）：半高圆角，用于按钮/标签等高密度 UI。
 *  Apple HIG 规则：胶囊圆角 = 元素高度 / 2。
 *  适用于按钮、标签、芯片等需要完全圆角的元素。 */
inline float GetCapsuleRadius(float elementHeight)
{
    return elementHeight * 0.5f;
}

/** @brief 同心圆角（Concentric）：内层元素圆角 = 外层圆角 - 内边距。
 *  Apple HIG 规则：内层圆角必须比外层小，保证圆角曲线自然衔接。 */
inline float GetConcentricRadius(float parentRadius, float padding)
{
    return std::max(0.0f, parentRadius - padding);
}

// ── Typography 字体家族回退链 ──────────────────────────────────
// Apple HIG: SF Pro Display/Text 是标准字体。
// 非 Apple 平台回退：SF Pro → Inter → Segoe UI Variable。
// Inter 是最接近 SF Pro 的开源替代（字重 600 + ss03 ≈ SF Pro 圆体 a）。

enum class FontFamily
{
    SFPro,           // Apple 原生 SF Pro
    Inter,           // Google Inter（最接近 SF Pro 的开源替代）
    SegoeUIVariable, // Windows 标准字体
};

/** @brief 检测系统可用的最佳字体家族。 */
inline FontFamily DetectBestFontFamily()
{
    // 尝试检测 SF Pro（macOS/iOS 原生，Windows 需手动安装）
    {
        HKEY hKey{};
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts",
                0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            DWORD size = 0;
            // SF Pro Text Regular
            if (RegQueryValueExW(hKey, L"SF Pro Text Regular (TrueType)",
                    nullptr, nullptr, nullptr, &size) == ERROR_SUCCESS)
            {
                RegCloseKey(hKey);
                return FontFamily::SFPro;
            }
            RegCloseKey(hKey);
        }
    }
    // 尝试检测 Inter
    {
        HKEY hKey{};
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts",
                0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            DWORD size = 0;
            if (RegQueryValueExW(hKey, L"Inter Regular (TrueType)",
                    nullptr, nullptr, nullptr, &size) == ERROR_SUCCESS)
            {
                RegCloseKey(hKey);
                return FontFamily::Inter;
            }
            RegCloseKey(hKey);
        }
    }
    // 回退到 Segoe UI Variable
    return FontFamily::SegoeUIVariable;
}

/** @brief 获取当前字体家族的显示名称（用于 DWrite）。 */
inline const wchar_t* GetDisplayFontName()
{
    static FontFamily detected = DetectBestFontFamily();
    switch (detected)
    {
    case FontFamily::SFPro:           return L"SF Pro Display";
    case FontFamily::Inter:           return L"Inter";
    case FontFamily::SegoeUIVariable: return L"Segoe UI Variable";
    }
    return L"Segoe UI Variable";
}

/** @brief 获取当前字体家族的正文名称（用于 DWrite）。 */
inline const wchar_t* GetBodyFontName()
{
    static FontFamily detected = DetectBestFontFamily();
    switch (detected)
    {
    case FontFamily::SFPro:           return L"SF Pro Text";
    case FontFamily::Inter:           return L"Inter";
    case FontFamily::SegoeUIVariable: return L"Segoe UI Variable";
    }
    return L"Segoe UI Variable";
}

/** @brief 获取推荐的字间距补偿值（Inter 默认比 SF Pro 宽，需收紧）。 */
inline float GetLetterSpacingCompensation()
{
    static FontFamily detected = DetectBestFontFamily();
    // Apple HIG: 负字间距是"Apple tight"的标志，display 尺寸收紧 0.1~0.374px
    // Inter 默认比 SF Pro 宽，需额外收紧 ~0.01em
    switch (detected)
    {
    case FontFamily::SFPro:           return 0.0f;    // SF Pro 原生，无需补偿
    case FontFamily::Inter:           return -0.01f;   // Inter 需收紧 ~0.01em
    case FontFamily::SegoeUIVariable: return -0.005f;  // Segoe UI 略收紧
    }
    return 0.0f;
}

/** @brief 根据元素角色获取推荐的玻璃模糊半径。 */
inline float GetBlurRadiusForRole(const char* role)
{
    const auto& g = kGlass;
    // 小控件用小模糊，大面板用大模糊
    // 苹果规则：模糊半径不是固定的，随元素角色变化
    if (!role) return g.blurMedium;
    // 后续可扩展为更精细的角色映射
    return g.blurMedium;
}

/** @brief 获取当前光源强度（基于交互状态）。 */
inline float GetLightIntensity(bool active, bool focused)
{
    if (active || focused) return kGlass.lightIntensityHigh;
    return kGlass.lightIntensityMedium;
}

// ── Focus Ring 焦点环令牌 ───────────────────────────────────────
// Apple HIG: 2px solid 焦点环，使用 primaryFocus 颜色 (#0071e3)
// 键盘导航时显示，点击时隐藏。

struct FocusRingTokens
{
    float width;          // 2.0px 焦点环宽度
    float offset;         // 2.0px 焦点环与元素外边距的偏移
    float borderRadius;   // 圆角，跟随元素圆角
};

inline const FocusRingTokens kFocusRing = {
    2.0f,  // width — Apple HIG 标准 2px
    2.0f,  // offset — 元素外 2px
    4.0f,  // borderRadius — 基础圆角
};

/** @brief 根据元素角色获取焦点环圆角（与元素圆角对齐）。 */
inline float GetFocusRingRadius(float elementCornerRadius)
{
    return elementCornerRadius + kFocusRing.offset;
}

/** @brief 创建焦点环矩形（向外扩展 offset + width）。 */
inline RECT ExpandFocusRingBounds(RECT element, float offset, float width)
{
    RECT expanded{};
    expanded.left = element.left - static_cast<LONG>(offset + width);
    expanded.top = element.top - static_cast<LONG>(offset + width);
    expanded.right = element.right + static_cast<LONG>(offset + width);
    expanded.bottom = element.bottom + static_cast<LONG>(offset + width);
    return expanded;
}

/** @brief 根据元素角色获取焦点环颜色（始终使用 primaryFocus）。 */
inline D2D1_COLOR_F GetFocusRingColor()
{
    const auto& colors = GetColorTokens();
    return colors.primaryFocus;
}

/** @brief 判断当前是否应显示焦点环（仅键盘导航时）。 */
inline bool ShouldShowFocusRing(bool isKeyboardNavigation, bool isFocused)
{
    // Apple HIG: 焦点环仅在键盘导航且元素获焦时显示
    return isKeyboardNavigation && isFocused;
}

// ── Spring 物理动画 ────────────────────────────────────────────
// Apple HIG: 弹簧动画用于自然的 UI 过渡（出现/消失/位移/缩放）。
// 参数：mass=1.0, stiffness=100.0, damping=15.0（临界阻尼附近，无过冲）。
// 本函数计算弹簧动画在给定时间 t（秒）时的位置 [0.0, 1.0]。

/** @brief 弹簧动画位置计算。
 *  @param t 时间（秒，从 0 开始）
 *  @param mass 质量（Apple HIG: 1.0）
 *  @param stiffness 刚度（Apple HIG: 100.0）
 *  @param damping 阻尼（Apple HIG: 15.0）
 *  @return 归一化位置 [0.0, 1.0]，到达 1.0 时停止
 */
inline double SpringAnimation(double t, double mass, double stiffness, double damping)
{
    if (t <= 0.0) return 0.0;

    // 阻尼比 ζ = damping / (2 * sqrt(stiffness * mass))
    const double omega0 = std::sqrt(stiffness / mass);  // 自然频率
    const double zeta = damping / (2.0 * std::sqrt(stiffness * mass));

    double position;
    if (zeta < 1.0)
    {
        // 欠阻尼（有振荡）—— 最常见情况
        const double omegaD = omega0 * std::sqrt(1.0 - zeta * zeta);
        const double decay = std::exp(-zeta * omega0 * t);
        position = 1.0 - decay * (std::cos(omegaD * t) +
            (zeta * omega0 / omegaD) * std::sin(omegaD * t));
    }
    else if (zeta == 1.0)
    {
        // 临界阻尼（无振荡，最快收敛）
        const double decay = std::exp(-omega0 * t);
        position = 1.0 - decay * (1.0 + omega0 * t);
    }
    else
    {
        // 过阻尼（慢收敛，无振荡）
        const double s1 = -omega0 * (zeta + std::sqrt(zeta * zeta - 1.0));
        const double s2 = -omega0 * (zeta - std::sqrt(zeta * zeta - 1.0));
        const double c2 = (1.0 - s1) / (s2 - s1);
        const double c1 = 1.0 - c2;
        position = 1.0 - c1 * std::exp(s1 * t) - c2 * std::exp(s2 * t);
    }

    return std::clamp(position, 0.0, 1.0);
}

/** @brief 使用 Apple HIG 默认弹簧参数计算动画位置。 */
inline double AppleSpringAnimation(double t)
{
    const auto& m = kMotion;
    return SpringAnimation(t, m.springMass, m.springStiffness, m.springDamping);
}

/** @brief 弹簧动画是否已收敛（位置接近 1.0）。 */
inline bool SpringAnimationConverged(double position, double threshold = 0.999)
{
    return position >= threshold;
}

/** @brief 弹簧动画的推荐时间步长（毫秒）。 */
inline float GetSpringTimeStepMs()
{
    // Apple HIG: 标准时长 300ms，但弹簧动画实际时间由物理决定
    return kMotion.standardDurationMs;
}
// Apple HIG：当用户开启"减少透明度""增强对比""减少动画"时，
// 系统自动切换材质、颜色和动画行为。设计令牌需感知这些设置。

/** @brief 刷新全局可访问性令牌（从 Windows 系统设置读取）。 */
inline void RefreshAccessibilityTokens()
{
    // 减少透明度 → 玻璃变为不透明磨砂
    // Windows 10/11: 注册表 HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize
    {
        HKEY hKey{};
        if (RegOpenKeyExW(HKEY_CURRENT_USER,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            DWORD value = 1; // 默认启用透明
            DWORD size = sizeof(value);
            RegQueryValueExW(hKey, L"EnableTransparency", nullptr,
                nullptr, reinterpret_cast<LPBYTE>(&value), &size);
            gAccessibility.reducedTransparency = (value == 0);
            RegCloseKey(hKey);
        }
    }

    // 增强对比 → 元素变黑白+锐边框
    BOOL highContrast = FALSE;
    HIGHCONTRAST hc{};
    hc.cbSize = sizeof(hc);
    if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0))
        highContrast = (hc.dwFlags & HCF_HIGHCONTRASTON) != 0;
    gAccessibility.increasedContrast = highContrast != FALSE;

    // 减少动画 → 过渡变为线性
    BOOL reduceMotion = FALSE;
    SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &reduceMotion, 0);
    gAccessibility.reducedMotion = reduceMotion == FALSE;
}

/** @brief 获取当前有效的模糊半径（考虑可访问性设置）。 */
inline float GetEffectiveBlurRadius(float baseRadius)
{
    // Apple HIG: 减少透明度时模糊半径增大（磨砂效果更重）
    if (gAccessibility.reducedTransparency)
        return baseRadius * 2.0f;
    return baseRadius;
}

/** @brief 获取当前有效的透明度（考虑可访问性设置）。 */
inline float GetEffectiveAlpha(float baseAlpha)
{
    // Apple HIG: 减少透明度时透明度趋近 1.0
    if (gAccessibility.reducedTransparency)
        return std::clamp(baseAlpha + 0.3f, 0.0f, 1.0f);
    // Apple HIG: 增强对比时透明度完全不透明
    if (gAccessibility.increasedContrast)
        return 1.0f;
    return baseAlpha;
}

/** @brief 获取当前有效的动画时长（考虑可访问性设置）。 */
inline float GetEffectiveAnimationDuration(float baseDurationMs)
{
    // Apple HIG: 减少动画时过渡变为线性且更快
    if (gAccessibility.reducedMotion)
        return std::min(baseDurationMs, 100.0f);
    return baseDurationMs;
}

} // namespace snowdesktop::design_tokens
