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

// ── 便捷访问函数 ──────────────────────────────────────────────

inline const ColorTokens& GetColorTokens()
{
    return kLightTheme;
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

} // namespace snowdesktop::design_tokens
