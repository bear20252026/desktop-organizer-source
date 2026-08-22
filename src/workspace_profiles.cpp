/**
 * @file workspace_profiles.cpp
 * @brief 工作区配置文件实现
 *
 * 灵感来源：Desktop Fences+ 的 Workspace Profiles
 */

#include "workspace_profiles.h"
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>

namespace snowdesktop {

WorkspaceProfileManager::WorkspaceProfileManager() = default;

std::wstring WorkspaceProfileManager::GetConfigPath()
{
    wchar_t appData[MAX_PATH]{};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA,
            nullptr, SHGFP_TYPE_CURRENT, appData)))
    {
        std::wstring dir = std::wstring(appData) + L"\\SnowDesktop";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir + L"\\workspace_profiles.json";
    }
    return L"";
}

WorkspaceProfile WorkspaceProfileManager::CreateProfile(const std::string& name)
{
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();

    WorkspaceProfile profile;
    profile.id = "profile_" + std::to_string(now % 1000000);
    profile.name = name;
    profile.isActive = false;

    profiles_.push_back(profile);
    SaveConfig();
    return profile;
}

void WorkspaceProfileManager::DeleteProfile(const std::string& profileId)
{
    // 不允许删除最后一个配置文件
    if (profiles_.size() <= 1) return;

    profiles_.erase(
        std::remove_if(profiles_.begin(), profiles_.end(),
            [&](const WorkspaceProfile& p) { return p.id == profileId; }),
        profiles_.end());

    // 如果删除的是当前活跃配置，切换到第一个
    if (activeProfileId_ == profileId && !profiles_.empty())
    {
        activeProfileId_ = profiles_[0].id;
        profiles_[0].isActive = true;
    }

    SaveConfig();
}

bool WorkspaceProfileManager::SwitchToProfile(const std::string& profileId)
{
    // 保存当前布局
    SaveCurrentLayout();

    // 找到目标配置文件
    WorkspaceProfile* target = nullptr;
    for (auto& p : profiles_)
    {
        if (p.id == profileId)
        {
            target = &p;
            p.isActive = true;
        }
        else
        {
            p.isActive = false;
        }
    }
    if (!target) return false;

    activeProfileId_ = profileId;

    // 恢复目标配置文件的布局
    // 实际应用需要通知主程序重新布局 Widget 和围栏
    // 这里只更新配置，由调用方处理布局变化

    SaveConfig();
    return true;
}

void WorkspaceProfileManager::SaveCurrentLayout()
{
    // 保存当前布局到活跃配置文件
    // 实际实现需要从主程序获取当前 Widget/围栏位置
    // 这里只标记配置文件为已保存
}

const WorkspaceProfile* WorkspaceProfileManager::GetActiveProfile() const
{
    for (const auto& p : profiles_)
    {
        if (p.isActive)
            return &p;
    }
    return profiles_.empty() ? nullptr : &profiles_[0];
}

void WorkspaceProfileManager::AddAutoSwitchRule(
    const std::string& profileId,
    const AutoSwitchRule& rule)
{
    for (auto& p : profiles_)
    {
        if (p.id == profileId)
        {
            p.autoRules.push_back(rule);
            SaveConfig();
            return;
        }
    }
}

void WorkspaceProfileManager::HandleForegroundWindowChange(HWND hwnd)
{
    if (!hwnd || profiles_.size() <= 1) return;

    // 获取前台窗口的进程名
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (!processId) return;

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
        FALSE, processId);
    if (!hProcess) return;

    wchar_t processPath[MAX_PATH]{};
    DWORD size = MAX_PATH;
    BOOL success = QueryFullProcessImageNameW(hProcess, 0, processPath, &size);
    CloseHandle(hProcess);

    if (!success) return;

    std::wstring processName(processPath);
    // 提取文件名
    auto lastSlash = processName.find_last_of(L'\\');
    if (lastSlash != std::wstring::npos)
        processName = processName.substr(lastSlash + 1);

    // 检查自动切换规则
    for (const auto& profile : profiles_)
    {
        for (const auto& rule : profile.autoRules)
        {
            if (processName.find(rule.matchPattern) != std::wstring::npos)
            {
                if (!profile.isActive)
                    SwitchToProfile(profile.id);
                return;
            }
        }
    }
}

bool WorkspaceProfileManager::SaveConfig()
{
    std::wstring configPath = GetConfigPath();
    if (configPath.empty()) return false;

    std::ofstream file(configPath);
    if (!file.is_open()) return false;

    file << "{\n";
    file << "  \"activeProfile\": \"" << activeProfileId_ << "\",\n";
    file << "  \"profiles\": [\n";

    for (size_t i = 0; i < profiles_.size(); ++i)
    {
        const auto& p = profiles_[i];
        file << "    {\n";
        file << "      \"id\": \"" << p.id << "\",\n";
        file << "      \"name\": \"" << p.name << "\",\n";
        file << "      \"icon\": \"" << p.icon << "\",\n";
        file << "      \"isActive\": " << (p.isActive ? "true" : "false") << ",\n";

        // Widget 布局
        file << "      \"widgets\": [";
        for (size_t j = 0; j < p.widgets.size(); ++j)
        {
            const auto& w = p.widgets[j];
            if (j > 0) file << ",";
            file << "{\"id\":\"" << w.widgetId
                 << "\",\"x\":" << w.x
                 << ",\"y\":" << w.y
                 << ",\"w\":" << w.width
                 << ",\"h\":" << w.height
                 << ",\"z\":" << w.zIndex
                 << ",\"vis\":" << (w.visible ? "true" : "false") << "}";
        }
        file << "],\n";

        // 围栏布局
        file << "      \"fences\": [";
        for (size_t j = 0; j < p.fences.size(); ++j)
        {
            const auto& f = p.fences[j];
            if (j > 0) file << ",";
            file << "{\"id\":\"" << f.fenceId
                 << "\",\"x\":" << f.x
                 << ",\"y\":" << f.y
                 << ",\"w\":" << f.width
                 << ",\"h\":" << f.height
                 << ",\"col\":" << (f.collapsed ? "true" : "false") << "}";
        }
        file << "],\n";

        // Dock 配置
        file << "      \"dock\": {\"pos\":" << p.dock.position
             << ",\"iconSize\":" << p.dock.iconSize
             << ",\"mag\":" << p.dock.magnification
             << ",\"autoHide\":" << (p.dock.autoHide ? "true" : "false")
             << ",\"menuBar\":" << (p.dock.showMenuBar ? "true" : "false")
             << "},\n";

        file << "      \"wallpaper\": \"\"\n";
        file << "    }" << (i < profiles_.size() - 1 ? "," : "") << "\n";
    }

    file << "  ]\n";
    file << "}\n";
    return true;
}

bool WorkspaceProfileManager::LoadConfig()
{
    std::wstring configPath = GetConfigPath();
    if (configPath.empty()) return false;

    std::ifstream file(configPath);
    if (!file.is_open()) return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    // 简单解析活跃配置文件 ID
    auto pos = json.find("\"activeProfile\":");
    if (pos != std::string::npos)
    {
        auto q1 = json.find('"', pos + 17);
        auto q2 = json.find('"', q1 + 1);
        if (q1 != std::string::npos && q2 != std::string::npos)
            activeProfileId_ = json.substr(q1 + 1, q2 - q1 - 1);
    }

    // 解析配置文件列表（简化：只读取 id 和 name）
    size_t namePos = 0;
    while ((namePos = json.find("\"name\":", namePos)) != std::string::npos)
    {
        auto idPos = json.rfind("\"id\":", namePos);
        if (idPos == std::string::npos) { namePos += 7; continue; }

        auto idQ1 = json.find('"', idPos + 5);
        auto idQ2 = json.find('"', idQ1 + 1);
        auto nameQ1 = json.find('"', namePos + 7);
        auto nameQ2 = json.find('"', nameQ1 + 1);

        if (idQ1 != std::string::npos && idQ2 != std::string::npos &&
            nameQ1 != std::string::npos && nameQ2 != std::string::npos)
        {
            WorkspaceProfile profile;
            profile.id = json.substr(idQ1 + 1, idQ2 - idQ1 - 1);
            profile.name = json.substr(nameQ1 + 1, nameQ2 - nameQ1 - 1);
            profile.isActive = (profile.id == activeProfileId_);
            profiles_.push_back(profile);
        }
        namePos = nameQ2 + 1;
    }

    // 如果没有配置文件，创建默认
    if (profiles_.empty())
        CreateDefaultProfiles();

    return true;
}

void WorkspaceProfileManager::CreateDefaultProfiles()
{
    auto work = CreateProfile("Work");
    work.icon = "💼";
    work.dock.showMenuBar = true;
    work.dock.autoHide = false;

    auto gaming = CreateProfile("Gaming");
    gaming.icon = "🎮";
    gaming.dock.autoHide = true;
    gaming.dock.showMenuBar = false;

    auto dev = CreateProfile("Development");
    dev.icon = "💻";
    dev.dock.showMenuBar = true;

    // 默认激活"工作"
    if (!profiles_.empty())
    {
        activeProfileId_ = profiles_[0].id;
        profiles_[0].isActive = true;
    }

    SaveConfig();
}

} // namespace snowdesktop
