/**
 * @file multi_monitor.cpp
 * @brief 多显示器支持实现 — 每显示器独立配置 + 自动 DPI 感知
 */

#include "multi_monitor.h"
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <codecvt>

namespace snowdesktop {

// ── 回调函数（EnumDisplayMonitors 使用）──────────────────────────

static BOOL CALLBACK MonitorEnumProc(HMONITOR hMon, HDC, LPRECT, LPARAM dwData)
{
    auto* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(dwData);
    if (!monitors) return FALSE;

    MONITORINFOEXW mi{};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(hMon, &mi))
        return TRUE;

    MonitorInfo info;
    info.handle = hMon;
    info.rect = mi.rcMonitor;
    info.workArea = mi.rcWork;
    info.width = mi.rcMonitor.right - mi.rcMonitor.left;
    info.height = mi.rcMonitor.bottom - mi.rcMonitor.top;
    info.isPrimary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
    info.deviceId = mi.szDevice;

    // 获取 DPI（Windows 10 1607+ Per-Monitor V2）
    UINT dpiX = 96, dpiY = 96;
    HRESULT hr = GetDpiForMonitor(hMon, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
    if (SUCCEEDED(hr))
    {
        info.dpiX = static_cast<int>(dpiX);
        info.dpiY = static_cast<int>(dpiY);
        info.scaleFactor = static_cast<float>(dpiX) / 96.0f;
    }
    else
    {
        info.dpiX = 96;
        info.dpiY = 96;
        info.scaleFactor = 1.0f;
    }

    monitors->push_back(info);
    return TRUE;
}

// ── MultiMonitorManager 实现 ────────────────────────────────────

MultiMonitorManager::MultiMonitorManager() = default;

bool MultiMonitorManager::Initialize()
{
    // 启用 Per-Monitor DPI Aware V2（Windows 10 1703+）
    // 这使得应用程序能够为每个显示器使用正确的 DPI
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32)
    {
        typedef BOOL (WINAPI *FnSetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
        auto pSetProcessDpiAwarenessContext =
            reinterpret_cast<FnSetProcessDpiAwarenessContext>(
                GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (pSetProcessDpiAwarenessContext)
            pSetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }

    RefreshMonitors();
    LoadConfig();
    initialized_ = true;
    return true;
}

void MultiMonitorManager::RefreshMonitors()
{
    monitors_.clear();
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc,
        reinterpret_cast<LPARAM>(&monitors_));
}

const MonitorInfo* MultiMonitorManager::GetPrimaryMonitor() const
{
    for (const auto& m : monitors_)
    {
        if (m.isPrimary)
            return &m;
    }
    return monitors_.empty() ? nullptr : &monitors_[0];
}

const MonitorInfo* MultiMonitorManager::GetMonitorFromPoint(POINT pt) const
{
    HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    for (const auto& m : monitors_)
    {
        if (m.handle == hMon)
            return &m;
    }
    return nullptr;
}

const MonitorInfo* MultiMonitorManager::GetMonitorFromWindow(HWND hwnd) const
{
    HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    for (const auto& m : monitors_)
    {
        if (m.handle == hMon)
            return &m;
    }
    return nullptr;
}

MonitorConfig MultiMonitorManager::GetMonitorConfig(
    const std::wstring& monitorId) const
{
    auto it = configs_.find(monitorId);
    if (it != configs_.end())
        return it->second;

    // 返回默认配置
    MonitorConfig config;
    config.monitorId = monitorId;
    config.showDock = true;
    config.showMenuBar = true;
    config.showWidgets = true;
    config.dockPosition = 2; // 下
    return config;
}

void MultiMonitorManager::SetMonitorConfig(
    const std::wstring& monitorId,
    const MonitorConfig& config)
{
    configs_[monitorId] = config;
    SaveConfig();
}

POINT MultiMonitorManager::PhysicalToLogical(
    POINT physical, const MonitorInfo& monitor) const
{
    POINT logical;
    logical.x = static_cast<int>(physical.x / monitor.scaleFactor);
    logical.y = static_cast<int>(physical.y / monitor.scaleFactor);
    return logical;
}

POINT MultiMonitorManager::LogicalToPhysical(
    POINT logical, const MonitorInfo& monitor) const
{
    POINT physical;
    physical.x = static_cast<int>(logical.x * monitor.scaleFactor);
    physical.y = static_cast<int>(logical.y * monitor.scaleFactor);
    return physical;
}

float MultiMonitorManager::GetScaleFactor(const MonitorInfo& monitor) const
{
    return monitor.scaleFactor;
}

void MultiMonitorManager::OnMonitorChange(MonitorChangeCallback callback)
{
    callbacks_.push_back(std::move(callback));
}

void MultiMonitorManager::HandleDisplayChange()
{
    RefreshMonitors();
    for (auto& cb : callbacks_)
        cb(monitors_);
}

// ── 配置保存/加载 ───────────────────────────────────────────────

std::wstring MultiMonitorManager::GetConfigPath()
{
    wchar_t appData[MAX_PATH]{};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA,
            nullptr, SHGFP_TYPE_CURRENT, appData)))
    {
        std::wstring dir = std::wstring(appData) + L"\\SnowDesktop";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir + L"\\multi_monitor.json";
    }
    return L"";
}

bool MultiMonitorManager::SaveConfig()
{
    std::wstring configPath = GetConfigPath();
    if (configPath.empty()) return false;

    std::ofstream file(configPath);
    if (!file.is_open()) return false;

    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    file << "[\n";
    size_t i = 0;
    for (const auto& [id, config] : configs_)
    {
        file << "  {\n";
        file << "    \"monitorId\": \"" << conv.to_bytes(id) << "\",\n";
        file << "    \"showDock\": " << (config.showDock ? "true" : "false") << ",\n";
        file << "    \"showMenuBar\": " << (config.showMenuBar ? "true" : "false") << ",\n";
        file << "    \"showWidgets\": " << (config.showWidgets ? "true" : "false") << ",\n";
        file << "    \"dockPosition\": " << config.dockPosition << "\n";
        file << "  }" << (i < configs_.size() - 1 ? "," : "") << "\n";
        i++;
    }
    file << "]\n";
    return true;
}

bool MultiMonitorManager::LoadConfig()
{
    std::wstring configPath = GetConfigPath();
    if (configPath.empty()) return false;

    std::ifstream file(configPath);
    if (!file.is_open()) return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;

    // 简单解析
    size_t pos = 0;
    while ((pos = json.find("\"monitorId\":", pos)) != std::string::npos)
    {
        MonitorConfig config;
        auto q1 = json.find('"', pos + 12);
        auto q2 = json.find('"', q1 + 1);
        if (q1 != std::string::npos && q2 != std::string::npos)
            config.monitorId = conv.from_bytes(json.substr(q1 + 1, q2 - q1 - 1));

        config.showDock = json.find("\"showDock\": true", pos) != std::string::npos;
        config.showMenuBar = json.find("\"showMenuBar\": true", pos) != std::string::npos;
        config.showWidgets = json.find("\"showWidgets\": true", pos) != std::string::npos;

        auto dpPos = json.find("\"dockPosition\":", pos);
        if (dpPos != std::string::npos)
        {
            auto colon = json.find(':', dpPos);
            auto end = colon + 1;
            while (end < json.length() && (json[end] == ' ' || isdigit(json[end])))
                end++;
            try { config.dockPosition = std::stoi(json.substr(colon + 1, end - colon - 1)); }
            catch (...) { config.dockPosition = 2; }
        }

        if (!config.monitorId.empty())
            configs_[config.monitorId] = config;
        pos += 12;
    }

    return true;
}

} // namespace snowdesktop
