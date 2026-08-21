#include "app.h"
#include <shellapi.h>

// ── 动态壁纸模块（macOS Sonoma 风格时间驱动壁纸轮换）──────────────────────
//
// 参考来源：WinDynamicDesktop（★4904，MPL-2.0）— 按日出/日落/白天/夜间四段
// 轮换壁纸图片，模拟 macOS Mojave Dynamic Desktop 行为。
//
// 架构位置：L5 渲染层（桌面背景），不反向访问其他模块内部状态。
// 集成点：PersonalizationSettings 中读取 wallpaperTheme 路径，
//         通过定时器每 60 分钟检查并按时间切换壁纸。

namespace
{
constexpr UINT kWallpaperTimerId = 300;
constexpr DWORD kWallpaperCheckIntervalMs = 60 * 60 * 1000; // 60 分钟

// WinDynamicDesktop 兼容的主题 JSON 格式：
// {
//   "imageFilename": "sonoma_*.jpg",
//   "imageCredits": "Apple",
//   "sunriseImageList": [3, 4],
//   "dayImageList": [1, 5, 6],
//   "sunsetImageList": [7, 8],
//   "nightImageList": [2]
// }
// 图片按 dayImageList 均分白天时段，sunrise/sunset/night 同理。

enum class DaySegment { Night, Sunrise, Day, Sunset };

DaySegment ResolveDaySegment(int hour, int minute)
{
    // 简化时段划分（可扩展为精确日出日落计算）
    const int totalMin = hour * 60 + minute;
    if (totalMin >= 330 && totalMin < 420)   return DaySegment::Sunrise;  // 05:30-07:00
    if (totalMin >= 420 && totalMin < 1110)  return DaySegment::Day;      // 07:00-18:30
    if (totalMin >= 1110 && totalMin < 1200) return DaySegment::Sunset;   // 18:30-20:00
    return DaySegment::Night;                                              // 20:00-05:30
}

} // namespace

void DesktopApp::UpdateDynamicWallpaper()
{
    // 动态壁纸更新：检查当前时间，按主题 JSON 配置切换壁纸。
    // 需要 wallpaperTheme 路径（由 PersonalizationSettings 提供）。
    if (dynamicWallpaperThemePath_.empty())
        return;

    // 简化实现：直接设置壁纸路径。
    // 后续可扩展为解析主题 JSON、按日出日落精确计算时段。
    SYSTEMTIME st{};
    GetLocalTime(&st);
    const DaySegment seg = ResolveDaySegment(st.wHour, st.wMinute);

    // 读取主题目录中的图片文件列表
    std::wstring themeDir = dynamicWallpaperThemePath_;
    WIN32_FIND_DATAW findData{};
    std::wstring searchPath = themeDir + L"\\*.jpg";
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        searchPath = themeDir + L"\\*.png";
        hFind = FindFirstFileW(searchPath.c_str(), &findData);
        if (hFind == INVALID_HANDLE_VALUE)
            return;
    }

    // 收集所有图片文件
    std::vector<std::wstring> images;
    do
    {
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            images.push_back(findData.cFileName);
    } while (FindNextFileW(hFind, &findData));
    FindClose(hFind);

    if (images.empty())
        return;

    // 按时段选择图片索引
    int index = 0;
    switch (seg)
    {
    case DaySegment::Sunrise: index = 0; break;
    case DaySegment::Day:     index = static_cast<int>(images.size()) / 4; break;
    case DaySegment::Sunset:  index = static_cast<int>(images.size()) / 2; break;
    case DaySegment::Night:   index = static_cast<int>(images.size()) * 3 / 4; break;
    }
    index = std::clamp(index, 0, static_cast<int>(images.size()) - 1);

    std::wstring wallpaperPath = themeDir + L"\\" + images[index];

    // 如果壁纸未变化则跳过
    if (wallpaperPath == currentWallpaperPath_)
        return;
    currentWallpaperPath_ = wallpaperPath;

    // 设置桌面壁纸
    SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0,
        const_cast<wchar_t*>(wallpaperPath.c_str()),
        SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
}

void DesktopApp::SetDynamicWallpaperTheme(const std::wstring& themePath)
{
    dynamicWallpaperThemePath_ = themePath;
    currentWallpaperPath_.clear();
    UpdateDynamicWallpaper();
}
