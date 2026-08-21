#include "app.h"
#include "../design_tokens.h"

// ── 毛玻璃效果管道（Backdrop Effect Pipeline）───────────────────────────────
//
// 层间连接遵循 docs/architecture-pipeline.md 的管道约束：本文件是 L4 特效
// 管线的独立实现，只通过函数签名与上层通信，禁止跨节点反向访问状态。
//
//   管道拓扑（单向流动）：
//     [1] Capture  注册原生 backdrop（DWM 采样壁纸/背景，或浮窗合成器）
//     [2] Blur     Apple HIG Liquid Glass 三级模糊（小控件8px/面板20px/大覆盖36px）
//     [3] Tint     色调填充（玻璃底色）
//     [4] Noise    亚克力噪点纹理（复用 DrawAcrylicNoise 节点）
//     [4.5] Sheen  Liquid Glass 流动高光（斜向渐变光带，8秒周期）
//     [5] Compose  描边合成（液态玻璃边缘优先，退化到普通圆角描边）
//
// DrawWidgetPanelBackground 是本管道的薄封装，行为与旧实现保持一致。

namespace
{
/** @brief 从主题设置解析当前 PersonalizationSettings（未提供时回退到设置窗）。 */
const PersonalizationSettings* ResolveEffectSettings(
    const PersonalizationSettings* effectSettings)
{
    return effectSettings;
}
} // namespace

void DesktopApp::DrawBackdropEffectPanel(
    ID2D1DeviceContext* ctx, RECT frame, float radius,
    D2D1_COLOR_F fill, D2D1_COLOR_F border, bool selected, float strokeWidth,
    const PersonalizationSettings* effectSettings, bool registerBackdrop)
{
    if (!ctx || IsRectEmptyRect(frame))
        return;
    if (ctx != brushCacheContext_ || brushCache_.size() >= 512)
    {
        brushCache_.clear();
        brushCacheContext_ = ctx;
    }

    const PersonalizationSettings* resolved =
        ResolveEffectSettings(effectSettings);
    PersonalizationSettings p = resolved
        ? *resolved
        : (settingsWindow_
            ? settingsWindow_->GetPersonalization()
            : PersonalizationSettings::DarkPreset());
    radius = std::max(0.0f, radius);

    auto getBrush = [&](const D2D1_COLOR_F& c) -> ID2D1SolidColorBrush* {
        const auto key = D2DColorBrushKey(c);
        auto it = brushCache_.find(key);
        if (it == brushCache_.end())
        {
            ComPtr<ID2D1SolidColorBrush> b;
            if (FAILED(ctx->CreateSolidColorBrush(c, &b)) || !b) return nullptr;
            it = brushCache_.emplace(key, std::move(b)).first;
        }
        return it->second.Get();
    };

    D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(ToD2DRect(frame), radius, radius);

    // Apple HIG 同心圆角：内层元素的圆角 = 外层圆角 - 内边距
    // 保证圆角曲线自然衔接，不会出现尖角或不连续。
    const float insetRadius = std::max(0.0f, radius - strokeWidth);

    // [1] Capture — 原生毛玻璃由下层 CompositionBackdropBrush 提供，本层只
    //     负责把面板矩形注册给对应合成器（桌面或浮窗）。
    // [2] Blur — Apple HIG Liquid Glass 三级模糊：
    //     小控件 (按钮/芯片) = 8px, 标准面板 = 20px, 大覆盖层 = 36px。
    //     按面板尺寸自动选择合适的模糊级别，而非使用单一 glassBlurRadius。
    if (p.glassEnabled && registerBackdrop)
    {
        using namespace snowdesktop::design_tokens;
        const auto& glass = GetGlass();
        float effectiveBlur;
        if (false)  // Clear 模式预留：后续通过设置项或参数启用
        {
            // Apple HIG Liquid Glass Clear 模式：始终使用 blurSmall (8px)
            // 用于媒体背景上的控件（视频播放器、照片浏览器），更高透明度
            effectiveBlur = glass.blurSmall;
        }
        else
        {
            // Apple HIG Liquid Glass Regular 模式：按面板面积自动选择三级模糊
            const int panelPixels = (frame.right - frame.left) * (frame.bottom - frame.top);
            effectiveBlur = (panelPixels < 40000)
                ? glass.blurSmall   // 8px — 小控件（按钮、芯片）
                : (panelPixels < 200000)
                    ? glass.blurMedium  // 20px — 标准面板
                    : glass.blurLarge;  // 36px — 大覆盖层
            if (p.glassBlurRadius > 0.0f)
                effectiveBlur *= (p.glassBlurRadius / 20.0f);
        }

        if (renderingFloatingDock_)
            floatingDockBackdropCompositor_.AddPanel(
                snowdesktop::floating_dock_rules::
                    DesktopRectToWindowRect(
                        frame, floatingDockSourceRect_),
                radius, effectiveBlur);
        else
            desktopBackdropCompositor_.AddPanel(
                frame, radius, effectiveBlur);
    }

    // [3] Tint — 色调填充（玻璃底色）。
    if (fill.a > 0.0f)
    {
        if (auto* fillBrush = getBrush(fill))
            ctx->FillRoundedRectangle(rr, fillBrush);
    }

    // [4] Noise — 亚克力噪点纹理（玻璃开启且亚克力启用时叠加）。
    if (p.glassEnabled && p.acrylicEnabled)
    {
        POINT screenOrigin{};
        HWND renderWindow = hwnd_;
        if (renderWindow)
            ClientToScreen(renderWindow, &screenOrigin);
        DrawAcrylicNoise(ctx, frame, radius, p.contentTheme == 1,
            screenOrigin);
    }

    // [4.5] Liquid Glass — 流动高光（Dockable 参考：斜向渐变光带随时间漂移，
    //        形成"活玻璃"质感）。仅在玻璃开启时绘制，叠加在噪点之上、
    //        描边之下，使用系统启动时间作为相位，不需要额外计时器。
    if (p.glassEnabled)
    {
        const float w = static_cast<float>(frame.right - frame.left);
        const float h = static_cast<float>(frame.bottom - frame.top);
        if (w > 4.0f && h > 4.0f)
        {
            // 对角线方向：从左上到右下，宽度为面板对角线的 40%
            const float diag = std::sqrt(w * w + h * h);
            const float bandWidth = diag * 0.40f;
            // 相位漂移：用系统 tick 计算，0..2π 循环，约 8 秒一周
            const ULONGLONG tick = GetTickCount64();
            const float phase =
                static_cast<float>(tick % 8000u) / 8000.0f;
            const float offset = phase * (diag + bandWidth) - bandWidth * 0.5f;
            // 对角线方向向量（单位化）
            const float dx = w / diag;
            const float dy = h / diag;
            // 光带中心在面板对角线上的投影位置
            const float cx = frame.left + dx * offset;
            const float cy = frame.top + dy * offset;
            // 垂直于对角线的方向
            const float nx = -dy;
            const float ny = dx;
            D2D1_POINT_2F startPt = {
                cx + nx * bandWidth, cy + ny * bandWidth};
            D2D1_POINT_2F endPt = {
                cx - nx * bandWidth, cy - ny * bandWidth};
            const D2D1_GRADIENT_STOP sheenStops[] = {
                { 0.0f, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.0f) },
                { 0.40f, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.04f) },
                { 0.50f, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.07f) },
                { 0.60f, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.04f) },
                { 1.0f, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.0f) },
            };
            ComPtr<ID2D1GradientStopCollection> sheenCollection;
            if (SUCCEEDED(ctx->CreateGradientStopCollection(
                    sheenStops,
                    static_cast<UINT32>(std::size(sheenStops)),
                    D2D1_GAMMA_2_2,
                    D2D1_EXTEND_MODE_CLAMP,
                    &sheenCollection)) &&
                sheenCollection)
            {
                ComPtr<ID2D1LinearGradientBrush> sheenBrush;
                if (SUCCEEDED(ctx->CreateLinearGradientBrush(
                        D2D1::LinearGradientBrushProperties(
                            startPt, endPt),
                        D2D1::BrushProperties(),
                        sheenCollection.Get(),
                        &sheenBrush)) &&
                    sheenBrush)
                {
                    ctx->FillRoundedRectangle(rr, sheenBrush.Get());
                }
            }
        }
    }

    // [4.6] Liquid Glass — 边缘折射（Clear 模式光学效果）
    //        模拟光线穿过弯曲玻璃时在边缘产生的折射：边缘亮、中心暗。
    //        使用径向渐变从面板中心向四周扩散，边缘 15% 宽度内产生折射高光。
    if (p.glassEnabled)
    {
        using namespace snowdesktop::design_tokens;
        const auto& glass = GetGlass();
        const float w = static_cast<float>(frame.right - frame.left);
        const float h = static_cast<float>(frame.bottom - frame.top);
        if (w > 8.0f && h > 8.0f)
        {
            const D2D1_POINT_2F center = D2D1::Point2F(
                frame.left + w * 0.5f, frame.top + h * 0.5f);
            const float outerRadius = std::max(w, h) * 0.55f;
            const float edgeIntensity = glass.lightIntensityMedium * 0.6f;  // 折射强度低于主光源
            const D2D1_GRADIENT_STOP refractionStops[] = {
                { 0.0f, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.0f) },           // 中心：无折射
                { 0.75f, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.0f) },           // 75% 半径：无折射
                { 0.85f, D2D1::ColorF(1.0f, 1.0f, 1.0f, edgeIntensity) },  // 85% 半径：折射开始
                { 0.95f, D2D1::ColorF(1.0f, 1.0f, 1.0f, edgeIntensity * 0.7f) }, // 95%：折射衰减
                { 1.0f, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.0f) },            // 边缘：无
            };
            ComPtr<ID2D1GradientStopCollection> refractionCollection;
            if (SUCCEEDED(ctx->CreateGradientStopCollection(
                    refractionStops,
                    static_cast<UINT32>(std::size(refractionStops)),
                    D2D1_GAMMA_2_2,
                    D2D1_EXTEND_MODE_CLAMP,
                    &refractionCollection)) &&
                refractionCollection)
            {
                ComPtr<ID2D1RadialGradientBrush> refractionBrush;
                if (SUCCEEDED(ctx->CreateRadialGradientBrush(
                        D2D1::RadialGradientBrushProperties(
                            center,
                            D2D1::Point2F(0.0f, 0.0f),
                            outerRadius, outerRadius),
                        D2D1::BrushProperties(),
                        refractionCollection.Get(),
                        &refractionBrush)) &&
                    refractionBrush)
                {
                    ctx->FillRoundedRectangle(rr, refractionBrush.Get());
                }
            }
        }
    }

    // [5] Compose — 描边合成（选中项用强调色；玻璃边缘优先，退化到普通描边）。
    D2D1_COLOR_F stroke = selected
        ? D2D1::ColorF(0.39f, 0.66f, 1.0f, 0.90f)
        : border;
    if (stroke.a > 0.0f)
    {
        const bool glassDrawn = p.glassEnabled && !selected &&
            DrawGlassBorder(ctx, frame, radius, stroke, strokeWidth);
        if (!glassDrawn)
        {
            if (auto* strokeBrush = getBrush(stroke))
                ctx->DrawRoundedRectangle(rr, strokeBrush, strokeWidth, nullptr);
        }
    }
}

void DesktopApp::UpdateDesktopMicaBackdrop()
{
    // Desktop-level Mica: when enabled, register one full-client-area panel on
    // the native backdrop compositor so the whole desktop gets a macOS-style
    // frosted-wallpaper layer. Panels for individual widgets/dock are still
    // added by DrawBackdropEffectPanel as usual; this one sits behind them.
    PersonalizationSettings p = settingsWindow_
        ? settingsWindow_->GetPersonalization()
        : PersonalizationSettings::DarkPreset();
    if (!p.glassEnabled || !p.micaEnabled)
        return;

    RECT client{};
    if (!GetClientRect(hwnd_, &client) || IsRectEmpty(&client))
        return;
    constexpr float kMicaBlurScale = 2.5f;
    desktopBackdropCompositor_.AddPanel(
        client, 0.0f,
        std::max(16.0f, p.glassBlurRadius * kMicaBlurScale));
}

// ── 统一 Liquid Glass 表面入口（L4 管道的便捷封装）────────────────────────
//
// 任何需要 Liquid Glass 材质的表面（Dock 面板/Widget 面板/设置页面）都可以
// 通过调用此函数获得完整材质（Capture→Blur→Tint→Noise→Sheen→Refraction→Compose），
// 无需手动拼装六个节点——这是"模块化管道连接"的统一入口。
//
// 用法示例：
//   DrawLiquidGlassSurface(ctx, myRect, cornerRadius, false);
//   // → 自动读取 PersonalizationSettings，走完整 Liquid Glass 管道
//
// @param ctx D2D1 设备上下文
// @param frame 表面矩形（屏幕坐标）
// @param cornerRadius 圆角半径（Apple HIG：md=11px 卡片，sm=8px 按钮）
// @param selected 是否选中态（选中时用强调色描边）
void DesktopApp::DrawLiquidGlassSurface(
    ID2D1DeviceContext* ctx, RECT frame, float cornerRadius, bool selected)
{
    if (!ctx || IsRectEmptyRect(frame))
        return;

    // 读取当前主题设置
    const PersonalizationSettings p = settingsWindow_
        ? settingsWindow_->GetPersonalization()
        : PersonalizationSettings::DarkPreset();

    // 从设计令牌获取当前主题的颜色
    using namespace snowdesktop::design_tokens;
    const auto& colors = GetColorTokens();

    // Apple HIG: 玻璃面板底色（半透明深灰，与 Liquid Glass 材质一致）
    const D2D1_COLOR_F fillColor = D2D1::ColorF(
        p.widgetBgR, p.widgetBgG, p.widgetBgB, p.widgetAlpha);

    // Apple HIG: 玻璃边框（极低不透明度白色，毛玻璃边框几乎不可见）
    const D2D1_COLOR_F borderColor = D2D1::ColorF(
        p.widgetBorderR, p.widgetBorderG, p.widgetBorderB, p.widgetBorderAlpha);

    // 调用完整 Liquid Glass 管道（6 节点）
    DrawBackdropEffectPanel(
        ctx, frame, cornerRadius,
        fillColor, borderColor,
        selected,
        1.0f,  // strokeWidth
        &p,    // effectSettings
        true   // registerBackdrop（注册原生 backdrop 采样）
    );
}
