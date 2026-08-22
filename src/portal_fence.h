/**
 * @file portal_fence.h
 * @brief Portal Fence — 文件夹映射到桌面围栏
 *
 * 核心功能：
 *   1. 将真实文件夹映射到桌面围栏（半透明面板）
 *   2. 围栏内显示文件夹内容（文件图标+名称）
 *   3. 支持拖拽文件到围栏（移动到对应文件夹）
 *   4. 支持从围栏拖拽文件到桌面（复制/移动）
 *   5. 围栏可拖拽、调整大小、折叠/展开
 *   6. 围栏位置/大小持久化
 *   7. 文件夹变化时自动刷新
 *
 * 与 Desktop Fences+ 对标：
 *   - Portal Fence：映射真实文件夹，显示文件夹内容
 *   - 围栏可拖拽移动、调整大小
 *   - 折叠/展开切换
 *   - 文件拖拽到围栏 = 移动到对应文件夹
 *   - 双击文件 = 打开文件
 *
 * 安全机制：
 *   - 不修改文件夹内容（只显示和移动文件）
 *   - 拖拽前确认（可选）
 *   - 围栏配置持久化到 JSON
 */

#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <d2d1.h>
#include <d2d1_1.h>

namespace snowdesktop {

// 围栏内文件条目
struct FenceFileEntry
{
    std::wstring name;           // 文件名
    std::wstring fullPath;       // 完整路径
    std::wstring extension;      // 扩展名
    bool isDirectory = false;    // 是否为目录
    uint64_t fileSize = 0;       // 文件大小
    FILETIME lastModified = {};  // 最后修改时间
};

// 围栏配置
struct PortalFenceConfig
{
    std::wstring id;             // 围栏唯一 ID
    std::wstring name;           // 围栏显示名称
    std::wstring folderPath;     // 映射的文件夹路径
    int x = 100;                 // 屏幕 X 坐标
    int y = 100;                 // 屏幕 Y 坐标
    int width = 300;             // 围栏宽度
    int height = 400;            // 围栏高度
    bool collapsed = false;      // 是否折叠
    bool showHiddenFiles = false; // 是否显示隐藏文件
    int sortBy = 0;              // 排序方式（0=名称, 1=日期, 2=大小, 3=类型）
    float bgAlpha = 0.85f;       // 背景透明度
    float bgR = 0.11f;           // 背景红色
    float bgG = 0.11f;           // 背景绿色
    float bgB = 0.13f;           // 背景蓝色
};

// 围栏状态
enum class FenceState
{
    Normal,      // 正常状态
    Hovered,     // 鼠标悬停
    Dragging,    // 拖拽中
    Resizing,    // 调整大小中
    Dropping,    // 文件拖放中
};

/**
 * @brief Portal Fence 管理器
 *
 * 管理所有围栏的创建、渲染、交互、持久化。
 */
class PortalFenceManager
{
public:
    PortalFenceManager();
    ~PortalFenceManager() = default;

    /**
     * @brief 创建新围栏
     * @param folderPath 映射的文件夹路径
     * @param name 围栏显示名称（空则使用文件夹名）
     * @return 围栏配置
     */
    PortalFenceConfig CreateFence(const std::wstring& folderPath,
                                   const std::wstring& name = L"");

    /**
     * @brief 删除围栏
     * @param fenceId 围栏 ID
     */
    void DeleteFence(const std::wstring& fenceId);

    /**
     * @brief 获取所有围栏配置
     */
    const std::vector<PortalFenceConfig>& GetFences() const { return fences_; }

    /**
     * @brief 获取围栏内的文件列表
     * @param fenceId 围栏 ID
     * @return 文件列表
     */
    std::vector<FenceFileEntry> GetFenceFiles(const std::wstring& fenceId);

    /**
     * @brief 刷新围栏文件列表
     */
    void RefreshFence(const std::wstring& fenceId);

    /**
     * @brief 刷新所有围栏
     */
    void RefreshAll();

    /**
     * @brief 绘制围栏
     * @param ctx D2D1 设备上下文
     * @param fenceId 围栏 ID
     */
    void DrawFence(ID2D1DeviceContext* ctx, const std::wstring& fenceId);

    /**
     * @brief 绘制所有围栏
     */
    void DrawAllFences(ID2D1DeviceContext* ctx);

    /**
     * @brief 处理鼠标点击
     * @return 是否处理了点击
     */
    bool HandleClick(int x, int y);

    /**
     * @brief 处理鼠标拖拽
     */
    bool HandleDrag(int x, int y);

    /**
     * @brief 处理文件拖放到围栏
     */
    bool HandleFileDrop(const std::wstring& filePath, int x, int y);

    /**
     * @brief 保存围栏配置到文件
     */
    bool SaveConfig();

    /**
     * @brief 加载围栏配置
     */
    bool LoadConfig();

    /**
     * @brief 获取配置文件路径
     */
    static std::wstring GetConfigPath();

private:
    /**
     * @brief 生成围栏唯一 ID
     */
    static std::wstring GenerateId();

    /**
     * @brief 获取围栏的 D2D1 矩形
     */
    D2D1_RECT_F GetFenceRect(const PortalFenceConfig& fence) const;

    /**
     * @brief 绘制围栏标题栏
     */
    void DrawTitleBar(ID2D1DeviceContext* ctx, const PortalFenceConfig& fence,
                      const D2D1_RECT_F& rect);

    /**
     * @brief 绘制围栏文件列表
     */
    void DrawFileList(ID2D1DeviceContext* ctx, const PortalFenceConfig& fence,
                      const D2D1_RECT_F& rect);

    std::vector<PortalFenceConfig> fences_;
    std::unordered_map<std::wstring, std::vector<FenceFileEntry>> fileCache_;
    std::wstring activeFenceId_;  // 当前活跃（选中）的围栏
};

} // namespace snowdesktop
