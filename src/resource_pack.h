/**
 * @file resource_pack.h
 * @brief 资源包系统 — ZIP 主题包导入/导出
 *
 * 资源包是一个 ZIP 文件，包含：
 *   manifest.json  — 包元数据（名称、作者、版本、描述、预览图）
 *   theme.json     — 主题定义（颜色/玻璃/圆角/间距/组件样式）
 *   preview.png    — 预览截图（可选）
 *   widgets/       — 自定义 Widget 文件（可选）
 *   fonts/         — 自定义字体文件（可选）
 *   icons/         — 自定义图标文件（可选）
 *
 * manifest.json 格式：
 * {
 *   "name": "Ocean Blue",
 *   "author": "User",
 *   "version": "1.0.0",
 *   "description": "A deep blue theme inspired by the ocean",
 *   "preview": "preview.png",
 *   "theme": "theme.json",
 *   "minVersion": "1.0.0",
 *   "widgets": ["widgets/clock.lua"],
 *   "fonts": ["fonts/Inter-Regular.ttf"]
 * }
 *
 * 设计原则：
 *   - 用户可以双击 .snowpack 文件导入主题包
 *   - 主题包可以包含自定义 Widget 和字体
 *   - 导入后自动安装到正确的目录
 *   - 支持主题包导出（分享给其他用户）
 */

#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace snowdesktop {

// 资源包元数据
struct ResourcePackMetadata
{
    std::string name;
    std::string author;
    std::string version;
    std::string description;
    std::string previewImage;  // 包内预览图路径
    std::string themeFile;     // 包内主题文件路径
    std::string minVersion;    // 最低兼容版本
    std::vector<std::string> widgets;  // 包内 Widget 文件列表
    std::vector<std::string> fonts;    // 包内字体文件列表
    std::vector<std::string> icons;    // 包内图标文件列表
};

// 资源包导入结果
struct ResourcePackResult
{
    bool success = false;
    std::wstring errorMessage;
    std::wstring installedThemePath;   // 安装后的主题文件路径
    std::vector<std::wstring> installedWidgets;  // 安装的 Widget 文件
    std::vector<std::wstring> installedFonts;    // 安装的字体文件
};

/**
 * @brief 资源包管理器
 *
 * 管理 .snowpack 格式主题包的导入、导出、安装。
 */
class ResourcePackManager
{
public:
    /**
     * @brief 获取主题包目录路径
     */
    static std::wstring GetThemesPath();

    /**
     * @brief 获取 Widget 目录路径
     */
    static std::wstring GetWidgetsPath();

    /**
     * @brief 获取字体目录路径
     */
    static std::wstring GetFontsPath();

    /**
     * @brief 导入资源包
     * @param packPath .snowpack 文件路径
     * @return 导入结果
     *
     * 解压 ZIP → 读取 manifest.json → 安装主题/Widget/字体到对应目录
     */
    static ResourcePackResult ImportPack(const std::wstring& packPath);

    /**
     * @brief 导出当前主题为资源包
     * @param outputPath 输出 .snowpack 文件路径
     * @param metadata 包元数据
     * @return 是否导出成功
     */
    static bool ExportPack(const std::wstring& outputPath,
                           const ResourcePackMetadata& metadata);

    /**
     * @brief 读取资源包元数据（不解压完整包）
     * @param packPath .snowpack 文件路径
     * @return 包元数据
     */
    static ResourcePackMetadata ReadMetadata(const std::wstring& packPath);

    /**
     * @brief 列出已安装的资源包
     */
    static std::vector<std::wstring> ListInstalledPacks();

    /**
     * @brief 删除已安装的资源包
     * @param packName 包名称
     */
    static bool UninstallPack(const std::wstring& packName);

private:
    /**
     * @brief 解压 ZIP 文件到目标目录
     */
    static bool ExtractZip(const std::wstring& zipPath,
                           const std::wstring& destDir);

    /**
     * @brief 创建 ZIP 文件（从源目录）
     */
    static bool CreateZip(const std::wstring& sourceDir,
                          const std::wstring& zipPath);

    /**
     * @brief 解析 manifest.json
     */
    static ResourcePackMetadata ParseManifest(const std::string& json);
};

} // namespace snowdesktop
