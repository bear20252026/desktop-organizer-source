#include "app.h"
#include <winsock2.h>
#include "../design_tokens.h"

// ── 顶部菜单栏（macOS 风格系统状态栏）──────────────────────────────
//
// 实现一个 WS_EX_TOPMOST 全宽菜单栏，位于屏幕最顶部，显示：
//   左侧：当前聚焦窗口名称（占位）
//   中央：实时时钟（时:分 + 日期）
//   右侧：电池百分比、网络状态图标（占位）
//
// 管道架构：本文件是 L5 渲染层 + L1 输入层节点，不反向访问其他模块内部状态。
// 参考来源：LumX（D2D 渲染 + 500ms 刷新）、barik（可配置 widget 棒）、edgebar。

namespace
{
constexpr wchar_t kMenuBarClassName[] =
    L"SnowDesktop.MenuBar";
constexpr UINT_PTR kMenuBarClockTimerId = 200;
constexpr int kMenuBarHeightDip = 30;
constexpr int kMenuBarUpdateIntervalMs = 1000;

WNDPROC gMenuBarOrigWndProc = nullptr;

} // namespace

bool DesktopApp::CreateMenuBarWindow()
{
    if (menuBarHwnd_ && IsWindow(menuBarHwnd_))
        return true;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = instance_;
    wc.lpszClassName = kMenuBarClassName;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    menuBarHwnd_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW |
            WS_EX_NOACTIVATE | WS_EX_LAYERED,
        kMenuBarClassName, L"",
        WS_POPUP,
        0, 0, screenWidth, kMenuBarHeightDip,
        nullptr, nullptr, instance_, nullptr);
    if (!menuBarHwnd_)
        return false;

    // Liquid Glass 材质：Windows 原生毛玻璃 + 分层透明，模拟 macOS 菜单栏玻璃质感
    SetLayeredWindowAttributes(
        menuBarHwnd_, RGB(24, 28, 38), 230, LWA_ALPHA);

    const BOOL disableTransitions = TRUE;
    DwmSetWindowAttribute(menuBarHwnd_,
        DWMWA_TRANSITIONS_FORCEDISABLED,
        &disableTransitions, sizeof(disableTransitions));

    // Apple HIG: 启用沉浸式暗色模式（DWM 深色标题栏）
    const BOOL darkMode = TRUE;
    DwmSetWindowAttribute(menuBarHwnd_,
        20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */,
        &darkMode, sizeof(darkMode));

    SetTimer(menuBarHwnd_, kMenuBarClockTimerId,
        kMenuBarUpdateIntervalMs, nullptr);

    ShowWindow(menuBarHwnd_, SW_SHOWNOACTIVATE);
    return true;
}

void DesktopApp::DestroyMenuBarWindow()
{
    if (menuBarHwnd_)
    {
        KillTimer(menuBarHwnd_, kMenuBarClockTimerId);
        DestroyWindow(menuBarHwnd_);
        menuBarHwnd_ = nullptr;
    }
}

void DesktopApp::PaintMenuBarWindow(HWND hwnd)
{
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hwnd, &ps);
    if (!hdc)
        return;

    RECT client{};
    GetClientRect(hwnd, &client);
    const int w = client.right - client.left;
    const int h = client.bottom - client.top;

    // 使用 GDI 双缓冲绘制（菜单栏是轻量 UI，不需要 DComp 表面）。
    HDC memDc = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
    SelectObject(memDc, memBmp);

    // 背景填充 — Apple HIG: 近黑色 (#181C26)，与 Liquid Glass 深色材质一致
    using namespace snowdesktop::design_tokens;
    const auto& colors = GetColorTokens();
    const auto& typo = GetTypography();
    const int menuFontSize = static_cast<int>(GetScaledFontSize(typo.navLink.fontSize));  // Dynamic Type 缩放
    HBRUSH bgBrush = CreateSolidBrush(
        RGB(static_cast<int>(colors.surfaceTile1.r * 255),
            static_cast<int>(colors.surfaceTile1.g * 255),
            static_cast<int>(colors.surfaceTile1.b * 255)));
    FillRect(memDc, &client, bgBrush);
    DeleteObject(bgBrush);

    // 字体 — Apple HIG: SF Pro Text 12px nav-link 字重 400
    HFONT font = CreateFontW(
        -menuFontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI Variable");
    SelectObject(memDc, font);
    SetBkMode(memDc, TRANSPARENT);
    // Apple HIG: 暗色表面文本 = bodyOnDark (#ffffff)
    SetTextColor(memDc,
        RGB(static_cast<int>(colors.bodyOnDark.r * 255),
            static_cast<int>(colors.bodyOnDark.g * 255),
            static_cast<int>(colors.bodyOnDark.b * 255)));

    // 中央：时钟
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t timeBuf[64]{};
    wsprintfW(timeBuf, L"%d:%02d  %d/%d/%d",
        st.wHour, st.wMinute,
        st.wYear, st.wMonth, st.wDay);
    SIZE timeSize{};
    GetTextExtentPoint32W(memDc, timeBuf,
        static_cast<int>(wcslen(timeBuf)), &timeSize);
    TextOutW(memDc, (w - timeSize.cx) / 2,
        (h - timeSize.cy) / 2,
        timeBuf, static_cast<int>(wcslen(timeBuf)));

    // 左侧：应用名占位
    const wchar_t* appName = L"Desktop";
    TextOutW(memDc, 12, (h - timeSize.cy) / 2,
        appName, static_cast<int>(wcslen(appName)));

    // 右侧：电池 + WiFi + 音量
    int rightX = w - 12;
    wchar_t rightBuf[96]{};
    int rightLen = 0;
    SYSTEM_POWER_STATUS powerStatus{};
    if (GetSystemPowerStatus(&powerStatus))
    {
        rightLen += wsprintfW(rightBuf + rightLen, L" %d%%", powerStatus.BatteryLifePercent);
    }
    // WiFi 状态（通过 wininet.dll 动态加载 InternetGetConnectedState
    // 检测系统级网络连接状态，无链接依赖；比 socket 探测更准确）
    {
        static const HMODULE hWininet = LoadLibraryW(L"wininet.dll");
        typedef BOOL (WINAPI *FnInternetGetConnectedState)(LPDWORD, DWORD);
        static const auto pInternetGetConnectedState =
            hWininet ? reinterpret_cast<FnInternetGetConnectedState>(
                GetProcAddress(hWininet, "InternetGetConnectedState"))
            : nullptr;
        DWORD flags = 0;
        if (pInternetGetConnectedState && pInternetGetConnectedState(&flags, 0))
        {
            rightLen += wsprintfW(rightBuf + rightLen, L" WiFi");
        }
    }
    // 音量（通过 Core Audio API 获取主端点音量）
    // 简化：使用 GetMasterVolumeLevelScalar 需要 COM 初始化，
    // 菜单栏作为轻量 GDI 窗口，先用占位符，后续扩展。
    {
        rightLen += wsprintfW(rightBuf + rightLen, L" 🔊");
    }
    if (rightLen > 0)
    {
        SIZE rightSize{};
        GetTextExtentPoint32W(memDc, rightBuf,
            static_cast<int>(wcslen(rightBuf)), &rightSize);
        TextOutW(memDc, rightX - rightSize.cx,
            (h - rightSize.cy) / 2,
            rightBuf, static_cast<int>(wcslen(rightBuf)));
    }

    BitBlt(hdc, 0, 0, w, h, memDc, 0, 0, SRCCOPY);
    DeleteObject(font);
    DeleteObject(memBmp);
    DeleteDC(memDc);
    EndPaint(hwnd, &ps);
}
