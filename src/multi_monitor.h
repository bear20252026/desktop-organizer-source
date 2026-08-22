/**
 * @file multi_monitor.h
 * @brief 多显示器支持 — 每显示器独立配置 + 自动 DPI 感知
 *
 * 核心功能：
 *   1. 检测所有连接的显示器（分辨率、位置、DPI）
 *   2. 每显示器独立配置（Widget 布局、围栏位置、壁纸）
 *   3. 自动 DPI 感知（Per-Monitor DPI Aware V2）
 *   4. 显示器热插拔监听（WM_DISPLAYCHANGE）
 *   5. 坐标转换（物理像素 ↔ 逻辑像素）
 *
 * 与 macOS 对标：
 *   - macOS 每个显示器有独立的桌面空间
 *   - Widget 和文件夹可以在不同显示器上独立布局
 *   - HiDPI Retina 显示器自动缩放
 */

#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <shellscalingapi.h>

namespace snowdesktop {

// 显示器信息
struct MonitorInfo
{
    HMONITOR handle = nullptr;         // 显示器句柄
    std::wstring deviceId;             // 设备 ID
    std::wstring name;                 // 显示器名称（如 "DELL U2723QE"）
    RECT rect = {};                    // 显示器矩形（物理像素）
    RECT workArea = {};                // 工作区域（排除任务栏）
    int width = 0;                     // 宽度（物理像素）
    int height = 0;                    // 高度（物理像素）
    int dpiX = 96;                     // 水平 DPI
    int dpiY = 96;                     // 垂直 DPI
    float scaleFactor = 1.0f;          // DPI 缩放因子（1.0 = 100%）
    bool isPrimary = false;            // 是否为主显示器
    int refreshRate = 60;              // 刷新率（Hz）
    std::wstring colorProfile;         // 颜色配置文件路径
};

// 每显示器配置
struct MonitorConfig
{
    std::wstring monitorId;            // 显示器 ID
    std::wstring wallpaperPath;        // 壁纸路径（每显示器独立）
    bool showDock = true;              // 是否显示 Dock
    bool showMenuBar = true;           // 是否显示菜单栏
    bool showWidgets = true;           // 是否显示 Widget
    std::vector<std::string> widgetIds; // 此显示器上的 Widget 列表
    int dockPosition = 2;              // Dock 位置（0=上, 1=左, 2=下, 3=右）
};

// 显示器变更回调
using MonitorChangeCallback = std::function<void(
    const std::vector<MonitorInfo>& monitors)>;

/**
 * @brief 多显示器管理器
 *
 * 管理所有显示器的信息、配置、DPI 感知。
 */
class MultiMonitorManager
{
public:
    MultiMonitorManager();
    ~MultiMonitorManager() = default;

    /**
     * @brief 初始化多显示器系统
     *
     * 启用 Per-Monitor DPI Aware V2，扫描所有显示器。
     */
    bool Initialize();

    /**
     * @brief 刷新显示器列表
     *
     * 调用 EnumDisplayMonitors 重新扫描所有连接的显示器。
     */
    void RefreshMonitors();

    /**
     * @brief 获取所有显示器信息
     */
    const std::vector<MonitorInfo>& GetMonitors() const { return monitors_; }

    /**
     * @brief 获取主显示器
     */
    const MonitorInfo* GetPrimaryMonitor() const;

    /**
     * @brief 获取指定点所在的显示器
     */
    const MonitorInfo* GetMonitorFromPoint(POINT pt) const;

    /**
     * @brief 获取指定窗口所在的显示器
     */
    const MonitorInfo* GetMonitorFromWindow(HWND hwnd) const;

    /**
     * @brief 获取指定显示器的配置
     */
    MonitorConfig GetMonitorConfig(const std::wstring& monitorId) const;

    /**
     * @brief 设置指定显示器的配置
     */
    void SetMonitorConfig(const std::wstring& monitorId,
                          const MonitorConfig& config);

    /**
     * @brief 将物理像素坐标转换为逻辑像素
     *
     * 用于 DPI 感知渲染：物理像素 → 逻辑像素（96 DPI 基准）
     */
    POINT PhysicalToLogical(POINT physical, const MonitorInfo& monitor) const;

    /**
     * @brief 将逻辑像素坐标转换为物理像素
     */
    POINT LogicalToPhysical(POINT logical, const MonitorInfo& monitor) const;

    /**
     * @brief 获取指定显示器的 DPI 缩放因子
     */
    float GetScaleFactor(const MonitorInfo& monitor) const;

    /**
     * @brief 注册显示器变更回调
     */
    void OnMonitorChange(MonitorChangeCallback callback);

    /**
     * @brief 处理显示器变更消息（WM_DISPLAYCHANGE）
     */
    void HandleDisplayChange();

    /**
     * @brief 保存所有显示器配置
     */
    bool SaveConfig();

    /**
     * @brief 加载显示器配置
     */
    bool LoadConfig();

    /**
     * @brief 获取配置文件路径
     */
    static std::wstring GetConfigPath();

    /**
     * @brief 获取显示器数量
     */
    size_t GetMonitorCount() const { return monitors_.size(); }

private:
    std::vector<MonitorInfo> monitors_;
    std::unordered_map<std::wstring, MonitorConfig> configs_;
    std::vector<MonitorChangeCallback> callbacks_;
    bool initialized_ = false;
};

} // namespace snowdesktop
