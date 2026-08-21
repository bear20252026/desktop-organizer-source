#include "app.h"

// Reusable Direct2D drawing primitives.

void DesktopApp::DrawD2DRoundedRectangle(ID2D1RenderTarget* ctx, RECT rect, float radius,
    D2D1_COLOR_F fill, D2D1_COLOR_F stroke, float strokeWidth)
{
    if (!ctx || IsRectEmptyRect(rect)) return;
    if (ctx != brushCacheContext_ || brushCache_.size() >= 512)
    {
        brushCache_.clear();
        brushCacheContext_ = ctx;
    }

    D2D1_ROUNDED_RECT rounded = D2D1::RoundedRect(ToD2DRect(rect), radius, radius);
    if (fill.a > 0.0f)
    {
        std::uint64_t k = D2DColorBrushKey(fill);
        auto it = brushCache_.find(k);
        if (it == brushCache_.end())
        {
            ComPtr<ID2D1SolidColorBrush> b;
            if (SUCCEEDED(ctx->CreateSolidColorBrush(fill, &b)) && b)
                it = brushCache_.emplace(k, std::move(b)).first;
            else
                return;
        }
        if (it != brushCache_.end() && it->second)
            ctx->FillRoundedRectangle(rounded, it->second.Get());
    }
    if (stroke.a > 0.0f)
    {
        std::uint64_t k = D2DColorBrushKey(stroke);
        auto it = brushCache_.find(k);
        if (it == brushCache_.end())
        {
            ComPtr<ID2D1SolidColorBrush> b;
            if (SUCCEEDED(ctx->CreateSolidColorBrush(stroke, &b)) && b)
                it = brushCache_.emplace(k, std::move(b)).first;
            else
                return;
        }
        if (it != brushCache_.end() && it->second)
            ctx->DrawRoundedRectangle(rounded, it->second.Get(), strokeWidth, nullptr);
    }
}

void DesktopApp::DrawWidgetPanelBackground(ID2D1DeviceContext* ctx, RECT frame, float radius,
    D2D1_COLOR_F fill, D2D1_COLOR_F border, bool selected, float strokeWidth,
    const PersonalizationSettings* effectSettings, bool registerBackdrop)
{
    // Thin wrapper: the glass/acrylic/border work lives in the L4 backdrop
    // effect pipeline (app_backdrop_effect.cpp) so the effect stays one
    // independently testable stage instead of being inlined here.
    DrawBackdropEffectPanel(ctx, frame, radius, fill, border, selected,
        strokeWidth, effectSettings, registerBackdrop);
}

void DesktopApp::DrawAcrylicNoise(ID2D1DeviceContext* ctx, RECT frame,
    float radius, bool lightTheme, POINT screenOrigin)
{
    if (!ctx || IsRectEmptyRect(frame))
        return;

    constexpr UINT kNoiseSize = 64;
    const std::uintptr_t contextKey =
        reinterpret_cast<std::uintptr_t>(ctx) & ~std::uintptr_t{1};
    const std::uintptr_t cacheKey = contextKey |
        static_cast<std::uintptr_t>(lightTheme);
    auto found = acrylicNoiseBrushCache_.find(cacheKey);
    if (found == acrylicNoiseBrushCache_.end())
    {
        if (acrylicNoiseBrushCache_.size() >= 8)
            acrylicNoiseBrushCache_.clear();

        std::array<std::uint32_t, kNoiseSize * kNoiseSize> pixels{};
        std::uint32_t state = 0x534E4F57u; // "SNOW", fixed seed.
        for (std::uint32_t& pixel : pixels)
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            // System acrylic uses a very subtle texture. Keep alpha between
            // roughly 0.8% and 3.1%, with polarity selected by content theme.
            const std::uint8_t alpha = static_cast<std::uint8_t>(
                2u + ((state >> 24) & 0x06u));
            const std::uint8_t channel = lightTheme ? 0u : alpha;
            pixel = (static_cast<std::uint32_t>(alpha) << 24) |
                (static_cast<std::uint32_t>(channel) << 16) |
                (static_cast<std::uint32_t>(channel) << 8) |
                static_cast<std::uint32_t>(channel);
        }

        D2D1_BITMAP_PROPERTIES1 bitmapProperties =
            D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_NONE,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                    D2D1_ALPHA_MODE_PREMULTIPLIED),
                96.0f, 96.0f);
        ComPtr<ID2D1Bitmap1> bitmap;
        const D2D1_SIZE_U bitmapSize =
            D2D1::SizeU(kNoiseSize, kNoiseSize);
        if (FAILED(ctx->CreateBitmap(bitmapSize, pixels.data(),
                kNoiseSize * sizeof(std::uint32_t), &bitmapProperties,
                &bitmap)) || !bitmap)
            return;

        D2D1_BITMAP_BRUSH_PROPERTIES1 brushProperties =
            D2D1::BitmapBrushProperties1(
                D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP,
                D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
        ComPtr<ID2D1BitmapBrush1> brush;
        if (FAILED(ctx->CreateBitmapBrush(bitmap.Get(), &brushProperties,
                nullptr, &brush)) || !brush)
            return;
        found = acrylicNoiseBrushCache_.emplace(cacheKey,
            std::move(brush)).first;
    }

    if (found->second)
    {
        // Keep the tile aligned to physical screen pixels. Redrawing or moving
        // a panel therefore samples the same noise instead of making the
        // texture appear to shimmer.
        found->second->SetTransform(D2D1::Matrix3x2F::Translation(
            -static_cast<float>(screenOrigin.x),
            -static_cast<float>(screenOrigin.y)));
        ctx->FillRoundedRectangle(D2D1::RoundedRect(
            ToD2DRect(frame), radius, radius), found->second.Get());
    }
}

void DesktopApp::DrawD2DFilledRectangle(ID2D1RenderTarget* ctx, RECT rect,
    D2D1_COLOR_F fill, D2D1_COLOR_F stroke)
{
    if (!ctx || IsRectEmptyRect(rect)) return;
    if (ctx != brushCacheContext_ || brushCache_.size() >= 512)
    {
        brushCache_.clear();
        brushCacheContext_ = ctx;
    }

    if (fill.a > 0.0f)
    {
        std::uint64_t k = D2DColorBrushKey(fill);
        auto it = brushCache_.find(k);
        if (it == brushCache_.end())
        {
            ComPtr<ID2D1SolidColorBrush> b;
            if (SUCCEEDED(ctx->CreateSolidColorBrush(fill, &b)) && b)
                it = brushCache_.emplace(k, std::move(b)).first;
        }
        if (it != brushCache_.end() && it->second)
            ctx->FillRectangle(ToD2DRect(rect), it->second.Get());
    }
    if (stroke.a > 0.0f)
    {
        std::uint64_t k = D2DColorBrushKey(stroke);
        auto it = brushCache_.find(k);
        if (it == brushCache_.end())
        {
            ComPtr<ID2D1SolidColorBrush> b;
            if (SUCCEEDED(ctx->CreateSolidColorBrush(stroke, &b)) && b)
                it = brushCache_.emplace(k, std::move(b)).first;
        }
        if (it != brushCache_.end() && it->second)
            ctx->DrawRectangle(ToD2DRect(rect), it->second.Get(), 1.0f, nullptr);
    }
}
