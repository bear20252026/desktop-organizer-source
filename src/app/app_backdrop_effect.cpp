#include "app.h"

// ── 毛玻璃效果管道（Backdrop Effect Pipeline）───────────────────────────────
//
// 层间连接遵循 docs/architecture-pipeline.md 的管道约束：本文件是 L4 特效
// 管线的独立实现，只通过函数签名与上层通信，禁止跨节点反向访问状态。
//
//   管道拓扑（单向流动）：
//     [1] Capture  注册原生 backdrop（DWM 采样壁纸/背景，或浮窗合成器）
//     [2] Blur     模糊半径随 PersonalizationSettings.glassBlurRadius 传递
//     [3] Tint     色调填充（玻璃底色）
//     [4] Noise    亚克力噪点纹理（复用 DrawAcrylicNoise 节点）
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

    // [1] Capture — 原生毛玻璃由下层 CompositionBackdropBrush 提供，本层只
    //     负责把面板矩形注册给对应合成器（桌面或浮窗）。
    if (p.glassEnabled && registerBackdrop)
    {
        if (renderingFloatingDock_)
            floatingDockBackdropCompositor_.AddPanel(
                snowdesktop::floating_dock_rules::
                    DesktopRectToWindowRect(
                        frame, floatingDockSourceRect_),
                radius, p.glassBlurRadius);
        else
            desktopBackdropCompositor_.AddPanel(
                frame, radius, p.glassBlurRadius);
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
