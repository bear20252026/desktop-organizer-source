/**
 * @file workspace_profiles.h
 * @brief 工作区配置文件 — 不同场景独立布局
 *
 * 灵感来源：Desktop Fences+ 的 Workspace Profiles 功能
 *   - 用户可创建多个工作区配置文件（如"工作"、"游戏"、"开发"）
 *   - 每个配置文件保存独立的 Widget 布局、围栏位置、Dock 设置
 *   - 支持手动切换和自动切换（基于前台应用）
 *
 * 设计原则：
 *   - 配置文件是纯 JSON，可版本控制和分享
 *   - 切换是即时的（不需要重启应用）
 *   - 自动切换基于前台窗口匹配规则
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <windows.h>

namespace snowdesktop {

// Widget 位置配置
struct WidgetPlacement
{
    std::string widgetId;    // Widget 唯一 ID
    int x = 0;               // 屏幕 X 坐标
    int y = 0;               // 屏幕 Y 坐标
    int width = 200;         // 宽度
    int height = 200;        // 高度
    int zIndex = 0;          // 层级
    bool visible = true;     // 是否可见
};

// 围栏位置配置
struct FencePlacement
{
    std::string fenceId;     // 围栏 ID
    int x = 100;
    int y = 100;
    int width = 300;
    int height = 400;
    bool collapsed = false;
};

// Dock 配置
struct DockConfig
{
    int position = 2;        // 0=上, 1=左, 2=下, 3=右
    float iconSize = 48.0f;
    float magnification = 1.5f;
    bool autoHide = false;
    bool showMenuBar = true;
};

// 自动切换规则
struct AutoSwitchRule
{
    std::wstring matchPattern;  // 窗口标题/进程名匹配模式
    bool isRegex = false;       // 是否正则表达式
    std::string profileId;      // 匹配时切换到的配置文件 ID
};

// 工作区配置文件
struct WorkspaceProfile
{
    std::string id;                // 配置文件唯一 ID
    std::string name;              // 显示名称（如"工作"、"游戏"）
    std::string icon;              // 图标标识
    bool isActive = false;         // 是否为当前活跃配置

    std::vector<WidgetPlacement> widgets;     // Widget 布局
    std::vector<FencePlacement> fences;       // 围栏布局
    DockConfig dock;                          // Dock 配置
    std::string wallpaperPath;                // 壁纸路径（可选）
    std::vector<AutoSwitchRule> autoRules;    // 自动切换规则
};

/**
 * @brief 工作区配置文件管理器
 */
class WorkspaceProfileManager
{
public:
    WorkspaceProfileManager();
    ~WorkspaceProfileManager() = default;

    /**
     * @brief 创建新配置文件
     * @param name 配置文件名称
     * @return 新配置文件
     */
    WorkspaceProfile CreateProfile(const std::string& name);

    /**
     * @brief 删除配置文件
     */
    void DeleteProfile(const std::string& profileId);

    /**
     * @brief 切换到指定配置文件
     *
     * 保存当前布局 → 加载目标配置文件的布局 → 应用
     */
    bool SwitchToProfile(const std::string& profileId);

    /**
     * @brief 保存当前布局到当前配置文件
     */
    void SaveCurrentLayout();

    /**
     * @brief 获取所有配置文件
     */
    const std::vector<WorkspaceProfile>& GetProfiles() const { return profiles_; }

    /**
     * @brief 获取当前活跃配置文件
     */
    const WorkspaceProfile* GetActiveProfile() const;

    /**
     * @brief 添加自动切换规则
     */
    void AddAutoSwitchRule(const std::string& profileId,
                           const AutoSwitchRule& rule);

    /**
     * @brief 处理前台窗口变化（自动切换检查）
     */
    void HandleForegroundWindowChange(HWND hwnd);

    /**
     * @brief 保存配置到文件
     */
    bool SaveConfig();

    /**
     * @brief 加载配置
     */
    bool LoadConfig();

    /**
     * @brief 获取配置文件路径
     */
    static std::wstring GetConfigPath();

    /**
     * @brief 创建默认配置文件（"工作"、"休闲"、"开发"）
     */
    void CreateDefaultProfiles();

private:
    std::vector<WorkspaceProfile> profiles_;
    std::string activeProfileId_;
    std::unordered_map<std::string, std::vector<WidgetPlacement>> savedLayouts_;
};

} // namespace snowdesktop
