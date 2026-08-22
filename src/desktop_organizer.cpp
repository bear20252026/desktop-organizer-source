/**
 * @file desktop_organizer.cpp
 * @brief 真正的桌面文件自动整理实现
 *
 * 核心原则：真整理，不是假覆盖。
 * 扫描桌面 → 分类 → 创建文件夹 → 移动文件 → 记录日志（可撤销）
 */

#include "desktop_organizer.h"
#include <shlobj.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <codecvt>

namespace snowdesktop {

// ── 分类规则表 ─────────────────────────────────────────────────
// 扩展名 → (中文名, 英文文件夹名)

struct CategoryRule
{
    const wchar_t* ext;
    const wchar_t* categoryName;
    const wchar_t* folderName;
};

static const CategoryRule kCategoryRules[] = {
    // 文档
    {L".pdf",   L"\u6587\u6863", L"Documents"},     // 文档
    {L".doc",   L"\u6587\u6863", L"Documents"},
    {L".docx",  L"\u6587\u6863", L"Documents"},
    {L".txt",   L"\u6587\u6863", L"Documents"},
    {L".rtf",   L"\u6587\u6863", L"Documents"},
    {L".odt",   L"\u6587\u6863", L"Documents"},
    {L".xls",   L"\u6587\u6863", L"Documents"},
    {L".xlsx",  L"\u6587\u6863", L"Documents"},
    {L".ppt",   L"\u6587\u6863", L"Documents"},
    {L".pptx",  L"\u6587\u6863", L"Documents"},
    {L".csv",   L"\u6587\u6863", L"Documents"},
    {L".md",    L"\u6587\u6863", L"Documents"},
    {L".pages", L"\u6587\u6863", L"Documents"},
    {L".numbers",L"\u6587\u6863",L"Documents"},
    {L".keynote",L"\u6587\u6863",L"Documents"},
    // 图片
    {L".jpg",   L"\u56fe\u7247", L"Images"},         // 图片
    {L".jpeg",  L"\u56fe\u7247", L"Images"},
    {L".png",   L"\u56fe\u7247", L"Images"},
    {L".gif",   L"\u56fe\u7247", L"Images"},
    {L".bmp",   L"\u56fe\u7247", L"Images"},
    {L".webp",  L"\u56fe\u7247", L"Images"},
    {L".svg",   L"\u56fe\u7247", L"Images"},
    {L".ico",   L"\u56fe\u7247", L"Images"},
    {L".tiff",  L"\u56fe\u7247", L"Images"},
    {L".tif",   L"\u56fe\u7247", L"Images"},
    {L".avif",  L"\u56fe\u7247", L"Images"},
    {L".heic",  L"\u56fe\u7247", L"Images"},
    {L".heif",  L"\u56fe\u7247", L"Images"},
    // 视频
    {L".mp4",   L"\u89c6\u9891", L"Videos"},         // 视频
    {L".avi",   L"\u89c6\u9891", L"Videos"},
    {L".mkv",   L"\u89c6\u9891", L"Videos"},
    {L".mov",   L"\u89c6\u9891", L"Videos"},
    {L".wmv",   L"\u89c6\u9891", L"Videos"},
    {L".flv",   L"\u89c6\u9891", L"Videos"},
    {L".webm",  L"\u89c6\u9891", L"Videos"},
    {L".m4v",   L"\u89c6\u9891", L"Videos"},
    {L".mpg",   L"\u89c6\u9891", L"Videos"},
    {L".mpeg",  L"\u89c6\u9891", L"Videos"},
    // 音频
    {L".mp3",   L"\u97f3\u9891", L"Audio"},           // 音频
    {L".wav",   L"\u97f3\u9891", L"Audio"},
    {L".flac",  L"\u97f3\u9891", L"Audio"},
    {L".aac",   L"\u97f3\u9891", L"Audio"},
    {L".ogg",   L"\u97f3\u9891", L"Audio"},
    {L".m4a",   L"\u97f3\u9891", L"Audio"},
    {L".wma",   L"\u97f3\u9891", L"Audio"},
    {L".opus",  L"\u97f3\u9891", L"Audio"},
    {L".aiff",  L"\u97f3\u9891", L"Audio"},
    // 压缩包
    {L".zip",   L"\u538b\u7f29\u5305", L"Archives"},  // 压缩包
    {L".rar",   L"\u538b\u7f29\u5305", L"Archives"},
    {L".7z",    L"\u538b\u7f29\u5305", L"Archives"},
    {L".tar",   L"\u538b\u7f29\u5305", L"Archives"},
    {L".gz",    L"\u538b\u7f29\u5305", L"Archives"},
    {L".bz2",   L"\u538b\u7f29\u5305", L"Archives"},
    {L".xz",    L"\u538b\u7f29\u5305", L"Archives"},
    {L".dmg",   L"\u538b\u7f29\u5305", L"Archives"},
    {L".iso",   L"\u538b\u7f29\u5305", L"Archives"},
    // 代码
    {L".cpp",   L"\u4ee3\u7801", L"Code"},            // 代码
    {L".h",     L"\u4ee3\u7801", L"Code"},
    {L".c",     L"\u4ee3\u7801", L"Code"},
    {L".py",    L"\u4ee3\u7801", L"Code"},
    {L".js",    L"\u4ee3\u7801", L"Code"},
    {L".ts",    L"\u4ee3\u7801", L"Code"},
    {L".java",  L"\u4ee3\u7801", L"Code"},
    {L".cs",    L"\u4ee3\u7801", L"Code"},
    {L".go",    L"\u4ee3\u7801", L"Code"},
    {L".rs",    L"\u4ee3\u7801", L"Code"},
    {L".html",  L"\u4ee3\u7801", L"Code"},
    {L".css",   L"\u4ee3\u7801", L"Code"},
    {L".json",  L"\u4ee3\u7801", L"Code"},
    {L".xml",   L"\u4ee3\u7801", L"Code"},
    {L".yaml",  L"\u4ee3\u7801", L"Code"},
    {L".yml",   L"\u4ee3\u7801", L"Code"},
    {L".sql",   L"\u4ee3\u7801", L"Code"},
    {L".sh",    L"\u4ee3\u7801", L"Code"},
    {L".bat",   L"\u4ee3\u7801", L"Code"},
    {L".ps1",   L"\u4ee3\u7801", L"Code"},
    // 安装包
    {L".exe",   L"\u5b89\u88c5\u5305", L"Installers"}, // 安装包
    {L".msi",   L"\u5b89\u88c5\u5305", L"Installers"},
    {L".appx",  L"\u5b89\u88c5\u5305", L"Installers"},
    {L".msix",  L"\u5b89\u88c5\u5305", L"Installers"},
};

// ── DesktopOrganizer 实现 ───────────────────────────────────────

DesktopOrganizer::DesktopOrganizer() = default;

std::wstring DesktopOrganizer::GetDesktopPath()
{
    wchar_t path[MAX_PATH]{};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY,
            nullptr, SHGFP_TYPE_CURRENT, path)))
        return path;
    // 回退：环境变量
    wchar_t* env = _wgetenv(L"USERPROFILE");
    if (env)
        return std::wstring(env) + L"\\Desktop";
    return L"";
}

std::wstring DesktopOrganizer::GetOrganizeLogPath()
{
    wchar_t appData[MAX_PATH]{};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA,
            nullptr, SHGFP_TYPE_CURRENT, appData)))
    {
        std::wstring dir = std::wstring(appData) + L"\\SnowDesktop";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir + L"\\organize_log.json";
    }
    return L"";
}

bool DesktopOrganizer::ShouldOrganizeFile(const std::filesystem::path& filePath)
{
    // 排除目录
    if (std::filesystem::is_directory(filePath))
        return false;

    // 排除隐藏文件
    DWORD attrs = GetFileAttributesW(filePath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES)
        return false;
    if (attrs & FILE_ATTRIBUTE_HIDDEN)
        return false;
    if (attrs & FILE_ATTRIBUTE_SYSTEM)
        return false;

    // 排除 desktop.ini
    std::wstring filename = filePath.filename().wstring();
    if (_wcsicmp(filename.c_str(), L"desktop.ini") == 0)
        return false;

    // 排除快捷方式（.lnk）—— 快捷方式本身就是整理后的产物
    std::wstring ext = filePath.extension().wstring();
    if (_wcsicmp(ext.c_str(), L".lnk") == 0)
        return false;

    return true;
}

std::wstring DesktopOrganizer::GetCategoryName(const std::wstring& extension)
{
    std::wstring ext = extension;
    // 转小写
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

    for (const auto& rule : kCategoryRules)
    {
        if (ext == rule.ext)
            return rule.categoryName;
    }
    return L"\u5176\u4ed6";  // 其他
}

std::wstring DesktopOrganizer::GetCategoryFolderName(const std::wstring& extension)
{
    std::wstring ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

    for (const auto& rule : kCategoryRules)
    {
        if (ext == rule.ext)
            return rule.folderName;
    }
    return L"Other";
}

std::vector<FileCategory> DesktopOrganizer::PreviewOrganize()
{
    std::wstring desktopPath = GetDesktopPath();
    if (desktopPath.empty())
        return {};

    // 收集所有分类
    std::vector<FileCategory> categories;
    std::vector<std::wstring> categoryOrder = {
        L"Documents", L"Images", L"Videos", L"Audio",
        L"Archives", L"Code", L"Installers", L"Other"
    };

    // 初始化分类
    for (const auto& folderName : categoryOrder)
    {
        FileCategory cat;
        cat.folderName = folderName;
        // 设置分类名称（中文）
        if (folderName == L"Documents")  cat.name = L"\u6587\u6863";      // 文档
        else if (folderName == L"Images") cat.name = L"\u56fe\u7247";      // 图片
        else if (folderName == L"Videos") cat.name = L"\u89c6\u9891";      // 视频
        else if (folderName == L"Audio")  cat.name = L"\u97f3\u9891";      // 音频
        else if (folderName == L"Archives") cat.name = L"\u538b\u7f29\u5305"; // 压缩包
        else if (folderName == L"Code")   cat.name = L"\u4ee3\u7801";      // 代码
        else if (folderName == L"Installers") cat.name = L"\u5b89\u88c5\u5305"; // 安装包
        else cat.name = L"\u5176\u4ed6";  // 其他
        categories.push_back(cat);
    }

    // 扫描桌面文件
    for (const auto& entry : std::filesystem::directory_iterator(desktopPath))
    {
        if (!ShouldOrganizeFile(entry.path()))
            continue;

        std::wstring ext = entry.path().extension().wstring();
        std::wstring folderName = GetCategoryFolderName(ext);

        // 找到对应分类并增加计数
        for (auto& cat : categories)
        {
            if (cat.folderName == folderName)
            {
                cat.fileCount++;
                cat.extensions.push_back(ext);
                break;
            }
        }
    }

    // 移除空分类
    categories.erase(
        std::remove_if(categories.begin(), categories.end(),
            [](const FileCategory& c) { return c.fileCount == 0; }),
        categories.end());

    return categories;
}

bool DesktopOrganizer::CreateCategoryFolder(const std::wstring& desktopPath,
                                             const std::wstring& folderName)
{
    std::wstring folderPath = desktopPath + L"\\" + folderName;
    if (GetFileAttributesW(folderPath.c_str()) != INVALID_FILE_ATTRIBUTES)
        return true;  // 已存在
    return CreateDirectoryW(folderPath.c_str(), nullptr) != FALSE;
}

std::wstring DesktopOrganizer::MoveFileWithConflictResolution(
    const std::wstring& sourcePath,
    const std::wstring& destFolder)
{
    namespace fs = std::filesystem;
    fs::path src(sourcePath);
    fs::path destDir(destFolder);

    fs::path destPath = destDir / src.filename();

    // 冲突处理：自动重命名
    if (fs::exists(destPath))
    {
        std::wstring stem = src.stem().wstring();
        std::wstring ext = src.extension().wstring();
        int counter = 1;
        while (fs::exists(destPath) && counter < 1000)
        {
            destPath = destDir / (stem + L"_" + std::to_wstring(counter) + ext);
            counter++;
        }
        if (fs::exists(destPath))
            return L"";  // 放弃
    }

    // 执行移动
    std::error_code ec;
    fs::rename(src, destPath, ec);
    if (ec)
    {
        // rename 失败（跨分区），尝试 copy + delete
        fs::copy_file(src, destPath, fs::copy_options::overwrite_existing, ec);
        if (ec) return L"";
        fs::remove(src, ec);
        if (ec) return L"";
    }

    return destPath.wstring();
}

bool DesktopOrganizer::SaveMoveLog(const std::vector<FileMoveRecord>& records)
{
    std::wstring logPath = GetOrganizeLogPath();
    if (logPath.empty()) return false;

    std::ofstream file(logPath);
    if (!file.is_open()) return false;

    file << "[\n";
    for (size_t i = 0; i < records.size(); ++i)
    {
        const auto& r = records[i];
        // 简单 JSON 格式
        file << "  {\n";
        // 使用窄字符路径（UTF-8 转换）
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        file << "    \"original\": \"" << conv.to_bytes(r.originalPath) << "\",\n";
        file << "    \"new\": \"" << conv.to_bytes(r.newPath) << "\",\n";
        file << "    \"category\": \"" << conv.to_bytes(r.category) << "\"\n";
        file << "  }" << (i < records.size() - 1 ? "," : "") << "\n";
    }
    file << "]\n";
    return true;
}

std::vector<FileMoveRecord> DesktopOrganizer::LoadMoveLog()
{
    std::vector<FileMoveRecord> records;
    std::wstring logPath = GetOrganizeLogPath();
    if (logPath.empty()) return records;

    // 简单解析 JSON 数组
    std::ifstream file(logPath);
    if (!file.is_open()) return records;

    std::string line;
    FileMoveRecord current;
    while (std::getline(file, line))
    {
        // 查找 "original": "..."
        auto findValue = [&line](const std::string& key) -> std::string {
            auto pos = line.find(key);
            if (pos == std::string::npos) return "";
            auto start = line.find('"', pos + key.length());
            if (start == std::string::npos) return "";
            start++;
            auto end = line.find('"', start);
            if (end == std::string::npos) return "";
            return line.substr(start, end - start);
        };

        std::string orig = findValue("\"original\": \"");
        std::string newPath = findValue("\"new\": \"");
        std::string cat = findValue("\"category\": \"");

        if (!orig.empty())
        {
            std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
            current.originalPath = conv.from_bytes(orig);
        }
        if (!newPath.empty())
        {
            std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
            current.newPath = conv.from_bytes(newPath);
        }
        if (!cat.empty())
        {
            std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
            current.category = conv.from_bytes(cat);
            records.push_back(current);
            current = {};
        }
    }
    return records;
}

OrganizeResult DesktopOrganizer::Organize(OrganizeProgressCallback progressCallback)
{
    OrganizeResult result;
    std::wstring desktopPath = GetDesktopPath();
    if (desktopPath.empty())
    {
        result.errorMessage = L"Cannot find desktop path";
        return result;
    }

    // 第一步：扫描桌面文件
    std::vector<std::filesystem::path> filesToOrganize;
    for (const auto& entry : std::filesystem::directory_iterator(desktopPath))
    {
        if (ShouldOrganizeFile(entry.path()))
            filesToOrganize.push_back(entry.path());
    }

    result.totalFiles = static_cast<int>(filesToOrganize.size());
    if (filesToOrganize.empty())
        return result;

    // 第二步：创建分类文件夹
    std::vector<std::wstring> categoryFolders = {
        L"Documents", L"Images", L"Videos", L"Audio",
        L"Archives", L"Code", L"Installers", L"Other"
    };
    for (const auto& folder : categoryFolders)
    {
        if (CreateCategoryFolder(desktopPath, folder))
            result.createdFolders++;
    }

    // 第三步：移动文件
    int current = 0;
    for (const auto& filePath : filesToOrganize)
    {
        current++;
        if (progressCallback)
        {
            progressCallback(current, result.totalFiles,
                filePath.filename().wstring());
        }

        std::wstring ext = filePath.extension().wstring();
        std::wstring folderName = GetCategoryFolderName(ext);
        std::wstring categoryName = GetCategoryName(ext);
        std::wstring destFolder = desktopPath + L"\\" + folderName;

        std::wstring newPath = MoveFileWithConflictResolution(
            filePath.wstring(), destFolder);

        if (!newPath.empty())
        {
            FileMoveRecord record;
            record.originalPath = filePath.wstring();
            record.newPath = newPath;
            record.category = categoryName;
            GetSystemTimeAsFileTime(&record.moveTime);
            result.moveRecords.push_back(record);
            result.movedFiles++;
        }
        else
        {
            result.skippedFiles++;
        }
    }

    // 第四步：保存移动日志（用于撤销）
    SaveMoveLog(result.moveRecords);

    return result;
}

int DesktopOrganizer::UndoLastOrganize()
{
    std::vector<FileMoveRecord> records = LoadMoveLog();
    if (records.empty())
        return 0;

    int undone = 0;
    // 从后往前撤销（避免路径冲突）
    for (auto it = records.rbegin(); it != records.rend(); ++it)
    {
        namespace fs = std::filesystem;
        fs::path currentPath(it->newPath);
        fs::path originalPath(it->originalPath);

        if (fs::exists(currentPath))
        {
            std::error_code ec;
            fs::rename(currentPath, originalPath, ec);
            if (!ec)
                undone++;
        }
    }

    // 清除日志
    std::wstring logPath = GetOrganizeLogPath();
    if (!logPath.empty())
        DeleteFileW(logPath.c_str());

    return undone;
}

bool DesktopOrganizer::CreateDesktopShortcut(
    const std::wstring& desktopPath,
    const std::wstring& targetPath,
    const std::wstring& shortcutName)
{
    IShellLinkW* psl = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr,
        CLSCTX_INPROC_SERVER, IID_IShellLinkW,
        reinterpret_cast<void**>(&psl));
    if (FAILED(hr) || !psl)
        return false;

    psl->SetPath(targetPath.c_str());
    psl->SetDescription(shortcutName.c_str());

    IPersistFile* ppf = nullptr;
    hr = psl->QueryInterface(IID_IPersistFile,
        reinterpret_cast<void**>(&ppf));
    if (SUCCEEDED(hr) && ppf)
    {
        std::wstring shortcutPath = desktopPath + L"\\" + shortcutName + L".lnk";
        ppf->Save(shortcutPath.c_str(), TRUE);
        ppf->Release();
    }
    psl->Release();
    return SUCCEEDED(hr);
}

} // namespace snowdesktop
