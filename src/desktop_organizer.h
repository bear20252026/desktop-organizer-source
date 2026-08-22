/**
 * @file desktop_organizer.h
 * @brief 真正的桌面文件自动整理模块
 *
 * 核心功能：
 *   1. 扫描桌面所有文件（不包括文件夹和系统文件）
 *   2. 按文件类型自动分类（文档/图片/视频/音频/压缩包/代码/快捷方式/其他）
 *   3. 在桌面创建真实分类文件夹
 *   4. 真正移动文件到对应分类文件夹
 *   5. 创建桌面快捷方式指向分类文件夹
 *
 * 与"假覆盖"方案的核心区别：
 *   - 假覆盖：隐藏桌面图标 + 显示假界面 → 关闭后桌面恢复原样
 *   - 真整理：真正移动文件到分类文件夹 → 关闭软件后桌面仍然整洁
 *
 * 安全机制：
 *   - 移动前记录原位置，支持一键撤销
 *   - 不移动系统文件、隐藏文件、正在使用的文件
 *   - 冲突时自动重命名（不覆盖）
 *   - 移动日志记录到 data/organize_log.json
 */

#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <functional>

namespace snowdesktop {

// 分类结果
struct FileCategory
{
    std::wstring name;           // 分类名称（如 "文档"、"图片"）
    std::wstring folderName;     // 分类文件夹名（如 "Documents"、"Images"）
    std::vector<std::wstring> extensions;  // 属于此分类的扩展名
    int fileCount = 0;           // 此分类的文件数量
};

// 文件移动记录（用于撤销）
struct FileMoveRecord
{
    std::wstring originalPath;   // 原始路径
    std::wstring newPath;        // 新路径
    std::wstring category;       // 分类名称
    FILETIME moveTime;           // 移动时间
};

// 整理结果
struct OrganizeResult
{
    int totalFiles = 0;          // 扫描到的文件总数
    int movedFiles = 0;          // 成功移动的文件数
    int skippedFiles = 0;        // 跳过的文件数（系统文件/隐藏文件/正在使用）
    int createdFolders = 0;      // 创建的分类文件夹数
    std::vector<FileMoveRecord> moveRecords;  // 移动记录
    std::wstring errorMessage;   // 错误信息（如果有）
};

// 整理进度回调
using OrganizeProgressCallback = std::function<void(
    int current, int total, const std::wstring& currentFile)>;

/**
 * @brief 桌面文件自动整理器
 *
 * 核心原则：真整理，不是假覆盖。
 * 扫描桌面 → 分类 → 创建文件夹 → 移动文件 → 创建快捷方式
 */
class DesktopOrganizer
{
public:
    DesktopOrganizer();
    ~DesktopOrganizer() = default;

    // 禁用拷贝
    DesktopOrganizer(const DesktopOrganizer&) = delete;
    DesktopOrganizer& operator=(const DesktopOrganizer&) = delete;

    /**
     * @brief 扫描桌面文件并返回分类预览（不移动文件）
     * @return 各分类的文件列表
     *
     * 调用此函数可以预览整理效果，不实际移动任何文件。
     */
    std::vector<FileCategory> PreviewOrganize();

    /**
     * @brief 执行桌面整理（真正移动文件）
     * @param progressCallback 进度回调（可选）
     * @return 整理结果
     *
     * 执行流程：
     *   1. 扫描桌面文件
     *   2. 按类型分类
     *   3. 创建分类文件夹（在桌面下）
     *   4. 移动文件到对应文件夹
     *   5. 记录移动日志（用于撤销）
     */
    OrganizeResult Organize(OrganizeProgressCallback progressCallback = nullptr);

    /**
     * @brief 撤销最近一次整理（把文件移回原位）
     * @return 撤销的文件数量
     */
    int UndoLastOrganize();

    /**
     * @brief 获取桌面路径
     */
    static std::wstring GetDesktopPath();

    /**
     * @brief 获取整理日志路径
     */
    static std::wstring GetOrganizeLogPath();

    /**
     * @brief 检查文件是否应该被整理（排除系统文件、隐藏文件等）
     */
    static bool ShouldOrganizeFile(const std::filesystem::path& filePath);

    /**
     * @brief 根据文件扩展名获取分类名称
     */
    static std::wstring GetCategoryName(const std::wstring& extension);

    /**
     * @brief 根据文件扩展名获取分类文件夹名
     */
    static std::wstring GetCategoryFolderName(const std::wstring& extension);

private:
    /**
     * @brief 保存移动日志到文件（用于撤销）
     */
    bool SaveMoveLog(const std::vector<FileMoveRecord>& records);

    /**
     * @brief 加载移动日志
     */
    std::vector<FileMoveRecord> LoadMoveLog();

    /**
     * @brief 创建分类文件夹（如果不存在）
     */
    bool CreateCategoryFolder(const std::wstring& desktopPath,
                              const std::wstring& folderName);

    /**
     * @brief 移动单个文件（带冲突处理）
     * @return 移动后的实际路径，失败返回空
     */
    std::wstring MoveFileWithConflictResolution(
        const std::wstring& sourcePath,
        const std::wstring& destFolder);

    /**
     * @brief 创建桌面快捷方式
     */
    bool CreateDesktopShortcut(const std::wstring& desktopPath,
                               const std::wstring& targetPath,
                               const std::wstring& shortcutName);
};

} // namespace snowdesktop
