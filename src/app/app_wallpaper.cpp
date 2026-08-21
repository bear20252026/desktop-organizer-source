#include "app.h"
#include <shellapi.h>

// ── 动态壁纸模块（macOS Sonoma 风格时间驱动壁纸轮换）──────────────────────
//
// 参考来源：WinDynamicDesktop（★4904，MPL-2.0）— 按日出/日落/白天/夜间四段
// 轮换壁纸图片，模拟 macOS Mojave Dynamic Desktop 行为。
//
// 架构位置：L5 渲染层（桌面背景），不反向访问其他模块内部状态。
// 集成点：通过 SetDynamicWallpaperTheme 设置主题目录路径，
//         UpdateDynamicWallpaper 按时间选择图片并调用系统 API 切换壁纸。
//
// 主题格式（WinDynamicDesktop 兼容）：
// {
//   "imageFilename": "sonoma_*.jpg",
//   "sunriseImageList": [3, 4],
//   "dayImageList": [1, 5, 6, 7, 8, 9],
//   "sunsetImageList": [10, 11],
//   "nightImageList": [2, 12]
// }
// 图片编号按 imageFilename 中 * 位置展开，每段图片均分对应时段。

namespace
{
constexpr UINT kWallpaperTimerId = 300;
constexpr DWORD kWallpaperCheckIntervalMs = 15 * 60 * 1000; // 15 分钟

enum class DaySegment { Night, Sunrise, Day, Sunset };

DaySegment ResolveDaySegment(int hour, int minute)
{
    const int totalMin = hour * 60 + minute;
    if (totalMin >= 330 && totalMin < 420)   return DaySegment::Sunrise;  // 05:30-07:00
    if (totalMin >= 420 && totalMin < 1110)  return DaySegment::Day;      // 07:00-18:30
    if (totalMin >= 1110 && totalMin < 1200) return DaySegment::Sunset;   // 18:30-20:00
    return DaySegment::Night;                                              // 20:00-05:30
}

// 简易 JSON 数组解析（无外部依赖，处理 [1, 2, 3] 格式）
std::vector<int> ParseJsonIntArray(const std::string& json, const std::string& key)
{
    std::vector<int> result;
    auto pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos)
        return result;
    auto start = json.find('[', pos);
    auto end = json.find(']', start);
    if (start == std::string::npos || end == std::string::npos)
        return result;
    std::string arr = json.substr(start + 1, end - start - 1);
    std::string num;
    for (char c : arr)
    {
        if (c >= '0' && c <= '9')
            num += c;
        else if (!num.empty())
        {
            result.push_back(std::stoi(num));
            num.clear();
        }
    }
    if (!num.empty())
        result.push_back(std::stoi(num));
    return result;
}

// 简易 JSON 字符串解析
std::string ParseJsonString(const std::string& json, const std::string& key)
{
    auto pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos)
        return "";
    auto colon = json.find(':', pos);
    if (colon == std::string::npos)
        return "";
    auto start = json.find('"', colon + 1);
    if (start == std::string::npos)
        return "";
    auto end = json.find('"', start + 1);
    if (end == std::string::npos)
        return "";
    return json.substr(start + 1, end - start - 1);
}

// 将通配符模式中的 * 替换为数字，生成完整文件名
std::wstring ExpandImageFilename(const std::wstring& pattern, int number)
{
    std::wstring numStr = std::to_wstring(number);
    std::wstring result = pattern;
    auto pos = result.find(L'*');
    if (pos != std::wstring::npos)
        result.replace(pos, 1, numStr);
    return result;
}

// 从文件读取内容
std::string ReadFileContent(const std::wstring& path)
{
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ,
        FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return "";
    DWORD size = GetFileSize(hFile, nullptr);
    if (size == INVALID_FILE_SIZE || size == 0)
    {
        CloseHandle(hFile);
        return "";
    }
    std::string content(size, '\0');
    DWORD read = 0;
    ReadFile(hFile, &content[0], size, &read, nullptr);
    CloseHandle(hFile);
    content.resize(read);
    return content;
}

} // namespace

void DesktopApp::UpdateDynamicWallpaper()
{
    if (dynamicWallpaperThemePath_.empty())
        return;

    SYSTEMTIME st{};
    GetLocalTime(&st);
    const DaySegment seg = ResolveDaySegment(st.wHour, st.wMinute);
    const int totalMin = st.wHour * 60 + st.wMinute;

    // 尝试加载主题 JSON
    std::wstring themeJsonPath = dynamicWallpaperThemePath_ + L"\\theme.json";
    std::string jsonContent = ReadFileContent(themeJsonPath);

    std::wstring imagePattern;
    std::vector<int> segmentImages;

    if (!jsonContent.empty())
    {
        // WinDynamicDesktop 兼容模式：解析主题 JSON
        std::string pattern = ParseJsonString(jsonContent, "imageFilename");
        if (!pattern.empty())
        {
            // 转换为宽字符
            imagePattern.assign(pattern.begin(), pattern.end());

            // 根据当前时段选择图片列表
            std::vector<int> sunriseList = ParseJsonIntArray(jsonContent, "sunriseImageList");
            std::vector<int> dayList = ParseJsonIntArray(jsonContent, "dayImageList");
            std::vector<int> sunsetList = ParseJsonIntArray(jsonContent, "sunsetImageList");
            std::vector<int> nightList = ParseJsonIntArray(jsonContent, "nightImageList");

            switch (seg)
            {
            case DaySegment::Sunrise:
                segmentImages = sunriseList.empty() ? dayList : sunriseList;
                break;
            case DaySegment::Day:
                segmentImages = dayList;
                break;
            case DaySegment::Sunset:
                segmentImages = sunsetList.empty() ? dayList : sunsetList;
                break;
            case DaySegment::Night:
                segmentImages = nightList.empty() ? dayList : nightList;
                break;
            }

            // 按时段内均分选择图片
            if (!segmentImages.empty())
            {
                int segStart = 0, segEnd = 1440;
                switch (seg)
                {
                case DaySegment::Sunrise: segStart = 330; segEnd = 420; break;
                case DaySegment::Day:     segStart = 420; segEnd = 1110; break;
                case DaySegment::Sunset:  segStart = 1110; segEnd = 1200; break;
                case DaySegment::Night:   segStart = 1200; segEnd = 330 + 1440; break;
                }
                int segDuration = segEnd - segStart;
                int offset = (seg == DaySegment::Night && totalMin < 330)
                    ? totalMin + 1440 - segStart
                    : totalMin - segStart;
                int imgIndex = static_cast<int>(
                    (static_cast<double>(offset) / segDuration) * segmentImages.size());
                imgIndex = std::clamp(imgIndex, 0,
                    static_cast<int>(segmentImages.size()) - 1);

                int imgNum = segmentImages[imgIndex];
                std::wstring imgFile = ExpandImageFilename(imagePattern, imgNum);
                std::wstring fullPath = dynamicWallpaperThemePath_ + L"\\" + imgFile;

                if (fullPath != currentWallpaperPath_)
                {
                    // 检查文件是否存在
                    DWORD attr = GetFileAttributesW(fullPath.c_str());
                    if (attr != INVALID_FILE_ATTRIBUTES)
                    {
                        currentWallpaperPath_ = fullPath;
                        SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0,
                            const_cast<wchar_t*>(fullPath.c_str()),
                            SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
                    }
                }
                return;
            }
        }
    }

    // 回退模式：简单目录扫描
    WIN32_FIND_DATAW findData{};
    std::wstring searchPath = dynamicWallpaperThemePath_ + L"\\*.jpg";
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        searchPath = dynamicWallpaperThemePath_ + L"\\*.png";
        hFind = FindFirstFileW(searchPath.c_str(), &findData);
        if (hFind == INVALID_HANDLE_VALUE)
            return;
    }

    std::vector<std::wstring> images;
    do
    {
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            images.push_back(findData.cFileName);
    } while (FindNextFileW(hFind, &findData));
    FindClose(hFind);

    if (images.empty())
        return;

    int index = 0;
    switch (seg)
    {
    case DaySegment::Sunrise: index = 0; break;
    case DaySegment::Day:     index = static_cast<int>(images.size()) / 4; break;
    case DaySegment::Sunset:  index = static_cast<int>(images.size()) / 2; break;
    case DaySegment::Night:   index = static_cast<int>(images.size()) * 3 / 4; break;
    }
    index = std::clamp(index, 0, static_cast<int>(images.size()) - 1);

    std::wstring wallpaperPath = dynamicWallpaperThemePath_ + L"\\" + images[index];
    if (wallpaperPath == currentWallpaperPath_)
        return;
    currentWallpaperPath_ = wallpaperPath;
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
