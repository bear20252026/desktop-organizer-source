#include "app.h"
#include <winsock2.h>

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

    // 半透明深色背景，模拟 macOS 菜单栏毛玻璃质感。
    SetLayeredWindowAttributes(
        menuBarHwnd_, RGB(24, 28, 38), 230, LWA_ALPHA);

    const BOOL disableTransitions = TRUE;
    DwmSetWindowAttribute(menuBarHwnd_,
        DWMWA_TRANSITIONS_FORCEDISABLED,
        &disableTransitions, sizeof(disableTransitions));

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

    // 背景填充
    HBRUSH bgBrush = CreateSolidBrush(RGB(24, 28, 38));
    FillRect(memDc, &client, bgBrush);
    DeleteObject(bgBrush);

    // 字体
    HFONT font = CreateFontW(
        -14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI");
    SelectObject(memDc, font);
    SetBkMode(memDc, TRANSPARENT);
    SetTextColor(memDc, RGB(230, 235, 245));

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
    // WiFi 状态（简化：通过 ws2_32 探测本地回环判断网络可用性，
    // 避免依赖 iphlpapi.lib；后续可扩展为完整 WiFi SSID 检测）
    {
        // 轻量探测：尝试创建一个 TCP socket 判断网络栈是否就绪
        SOCKET probe = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (probe != INVALID_SOCKET)
        {
            rightLen += wsprintfW(rightBuf + rightLen, L" WiFi");
            closesocket(probe);
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
