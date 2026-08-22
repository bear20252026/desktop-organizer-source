/**
 * @file resource_pack.cpp
 * @brief 资源包系统实现 — ZIP 主题包导入/导出
 *
 * 使用 Windows Shell API（ShellExecute + tar）实现 ZIP 解压/压缩，
 * 不依赖第三方 ZIP 库。
 */

#include "resource_pack.h"
#include "theme_engine.h"
#include <shlobj.h>
#include <shellapi.h>
#include <fstream>
#include <sstream>
#include <iostream>

namespace snowdesktop {

// ── 路径管理 ────────────────────────────────────────────────────

std::wstring ResourcePackManager::GetThemesPath()
{
    wchar_t appData[MAX_PATH]{};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA,
            nullptr, SHGFP_TYPE_CURRENT, appData)))
    {
        std::wstring dir = std::wstring(appData) + L"\\SnowDesktop\\themes";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir;
    }
    return L"";
}

std::wstring ResourcePackManager::GetWidgetsPath()
{
    wchar_t appData[MAX_PATH]{};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA,
            nullptr, SHGFP_TYPE_CURRENT, appData)))
    {
        std::wstring dir = std::wstring(appData) + L"\\SnowDesktop\\widgets";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir;
    }
    return L"";
}

std::wstring ResourcePackManager::GetFontsPath()
{
    wchar_t appData[MAX_PATH]{};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA,
            nullptr, SHGFP_TYPE_CURRENT, appData)))
    {
        std::wstring dir = std::wstring(appData) + L"\\SnowDesktop\\fonts";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir;
    }
    return L"";
}

// ── manifest.json 解析 ──────────────────────────────────────────

ResourcePackMetadata ResourcePackManager::ParseManifest(const std::string& json)
{
    ResourcePackMetadata meta;

    auto extractString = [&json](const std::string& key) -> std::string {
        std::string search = "\"" + key + "\"";
        auto pos = json.find(search);
        if (pos == std::string::npos) return "";
        auto colon = json.find(':', pos + search.length());
        if (colon == std::string::npos) return "";
        auto start = json.find('"', colon + 1);
        if (start == std::string::npos) return "";
        start++;
        auto end = json.find('"', start);
        if (end == std::string::npos) return "";
        return json.substr(start, end - start);
    };

    meta.name = extractString("name");
    meta.author = extractString("author");
    meta.version = extractString("version");
    meta.description = extractString("description");
    meta.previewImage = extractString("preview");
    meta.themeFile = extractString("theme");
    meta.minVersion = extractString("minVersion");

    // 解析 widgets 数组（简单解析）
    auto widgetsPos = json.find("\"widgets\"");
    if (widgetsPos != std::string::npos)
    {
        auto arrStart = json.find('[', widgetsPos);
        auto arrEnd = json.find(']', arrStart);
        if (arrStart != std::string::npos && arrEnd != std::string::npos)
        {
            std::string arr = json.substr(arrStart + 1, arrEnd - arrStart - 1);
            size_t pos = 0;
            while (pos < arr.length())
            {
                auto q1 = arr.find('"', pos);
                if (q1 == std::string::npos) break;
                auto q2 = arr.find('"', q1 + 1);
                if (q2 == std::string::npos) break;
                meta.widgets.push_back(arr.substr(q1 + 1, q2 - q1 - 1));
                pos = q2 + 1;
            }
        }
    }

    return meta;
}

// ── ZIP 解压（使用 Windows tar 命令）────────────────────────────

bool ResourcePackManager::ExtractZip(const std::wstring& zipPath,
                                      const std::wstring& destDir)
{
    // Windows 10+ 自带 tar 命令，支持 ZIP 解压
    std::wstring cmd = L"tar -xf \"" + zipPath + L"\" -C \"" + destDir + L"\"";
    int result = _wsystem(cmd.c_str());
    return result == 0;
}

bool ResourcePackManager::CreateZip(const std::wstring& sourceDir,
                                     const std::wstring& zipPath)
{
    // 使用 tar 命令创建 ZIP
    std::wstring cmd = L"tar -cf \"" + zipPath + L"\" -C \"" + sourceDir + L"\" .";
    int result = _wsystem(cmd.c_str());
    return result == 0;
}

// ── 资源包导入 ──────────────────────────────────────────────────

ResourcePackResult ResourcePackManager::ImportPack(const std::wstring& packPath)
{
    ResourcePackResult result;

    // 1. 检查文件是否存在
    if (GetFileAttributesW(packPath.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        result.errorMessage = L"Resource pack file not found: " + packPath;
        return result;
    }

    // 2. 创建临时解压目录
    wchar_t tempDir[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tempDir);
    std::wstring extractDir = std::wstring(tempDir) + L"SnowDesktop_Pack_"
        + std::to_wstring(GetTickCount64());
    CreateDirectoryW(extractDir.c_str(), nullptr);

    // 3. 解压 ZIP
    if (!ExtractZip(packPath, extractDir))
    {
        result.errorMessage = L"Failed to extract resource pack";
        RemoveDirectoryW(extractDir.c_str());
        return result;
    }

    // 4. 读取 manifest.json
    std::wstring manifestPath = extractDir + L"\\manifest.json";
    std::ifstream manifestFile(manifestPath);
    if (!manifestFile.is_open())
    {
        result.errorMessage = L"Invalid resource pack: missing manifest.json";
        RemoveDirectoryW(extractDir.c_str());
        return result;
    }

    std::stringstream buffer;
    buffer << manifestFile.rdbuf();
    ResourcePackMetadata meta = ParseManifest(buffer.str());

    if (meta.name.empty())
    {
        result.errorMessage = L"Invalid manifest: missing name";
        RemoveDirectoryW(extractDir.c_str());
        return result;
    }

    // 5. 安装主题文件
    std::wstring themesPath = GetThemesPath();
    if (!meta.themeFile.empty())
    {
        std::wstring srcTheme = extractDir + L"\\" +
            std::wstring(meta.themeFile.begin(), meta.themeFile.end());
        if (GetFileAttributesW(srcTheme.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            // 转换为 .json 并复制到主题目录
            std::wstring destTheme = themesPath + L"\\" +
                std::wstring(meta.name.begin(), meta.name.end()) + L".json";
            CopyFileW(srcTheme.c_str(), destTheme.c_str(), FALSE);
            result.installedThemePath = destTheme;
        }
    }

    // 6. 安装 Widget 文件
    std::wstring widgetsPath = GetWidgetsPath();
    for (const auto& widget : meta.widgets)
    {
        std::wstring srcWidget = extractDir + L"\\" +
            std::wstring(widget.begin(), widget.end());
        if (GetFileAttributesW(srcWidget.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            // 提取文件名
            std::wstring filename = std::filesystem::path(srcWidget).filename().wstring();
            std::wstring destWidget = widgetsPath + L"\\" + filename;
            CopyFileW(srcWidget.c_str(), destWidget.c_str(), FALSE);
            result.installedWidgets.push_back(destWidget);
        }
    }

    // 7. 安装字体文件
    std::wstring fontsPath = GetFontsPath();
    for (const auto& font : meta.fonts)
    {
        std::wstring srcFont = extractDir + L"\\" +
            std::wstring(font.begin(), font.end());
        if (GetFileAttributesW(srcFont.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            std::wstring filename = std::filesystem::path(srcFont).filename().wstring();
            std::wstring destFont = fontsPath + L"\\" + filename;
            CopyFileW(srcFont.c_str(), destFont.c_str(), FALSE);
            result.installedFonts.push_back(destFont);
        }
    }

    // 8. 清理临时目录
    // 递归删除临时目录
    std::wstring rmCmd = L"rm -rf \"" + extractDir + L"\"";
    _wsystem(rmCmd.c_str());

    result.success = true;
    return result;
}

// ── 资源包导出 ──────────────────────────────────────────────────

bool ResourcePackManager::ExportPack(const std::wstring& outputPath,
                                      const ResourcePackMetadata& metadata)
{
    // 1. 创建临时打包目录
    wchar_t tempDir[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tempDir);
    std::wstring packDir = std::wstring(tempDir) + L"SnowDesktop_Export_"
        + std::to_wstring(GetTickCount64());
    CreateDirectoryW(packDir.c_str(), nullptr);

    // 2. 写入 manifest.json
    std::string manifestJson = "{\n";
    manifestJson += "  \"name\": \"" + metadata.name + "\",\n";
    manifestJson += "  \"author\": \"" + metadata.author + "\",\n";
    manifestJson += "  \"version\": \"" + metadata.version + "\",\n";
    manifestJson += "  \"description\": \"" + metadata.description + "\",\n";
    manifestJson += "  \"preview\": \"preview.png\",\n";
    manifestJson += "  \"theme\": \"theme.json\",\n";
    manifestJson += "  \"minVersion\": \"1.0.0\"\n";
    manifestJson += "}\n";

    std::ofstream manifestFile(packDir + L"\\manifest.json");
    manifestFile << manifestJson;
    manifestFile.close();

    // 3. 复制主题文件
    std::wstring themesPath = GetThemesPath();
    std::wstring themeSrc = themesPath + L"\\" +
        std::wstring(metadata.name.begin(), metadata.name.end()) + L".json";
    if (GetFileAttributesW(themeSrc.c_str()) != INVALID_FILE_ATTRIBUTES)
        CopyFileW(themeSrc.c_str(), (packDir + L"\\theme.json").c_str(), FALSE);

    // 4. 创建 ZIP
    bool success = CreateZip(packDir, outputPath);

    // 5. 清理
    std::wstring rmCmd = L"rm -rf \"" + packDir + L"\"";
    _wsystem(rmCmd.c_str());

    return success;
}

// ── 资源包管理 ──────────────────────────────────────────────────

ResourcePackMetadata ResourcePackManager::ReadMetadata(const std::wstring& packPath)
{
    // 解压到临时目录，读取 manifest.json，然后清理
    wchar_t tempDir[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tempDir);
    std::wstring extractDir = std::wstring(tempDir) + L"SnowDesktop_Meta_"
        + std::to_wstring(GetTickCount64());
    CreateDirectoryW(extractDir.c_str(), nullptr);

    ExtractZip(packPath, extractDir);

    std::wstring manifestPath = extractDir + L"\\manifest.json";
    std::ifstream file(manifestPath);
    ResourcePackMetadata meta;

    if (file.is_open())
    {
        std::stringstream buffer;
        buffer << file.rdbuf();
        meta = ParseManifest(buffer.str());
    }

    // 清理
    std::wstring rmCmd = L"rm -rf \"" + extractDir + L"\"";
    _wsystem(rmCmd.c_str());

    return meta;
}

std::vector<std::wstring> ResourcePackManager::ListInstalledPacks()
{
    std::vector<std::wstring> packs;
    std::wstring themesPath = GetThemesPath();
    if (themesPath.empty()) return packs;

    for (const auto& entry : std::filesystem::directory_iterator(themesPath))
    {
        if (entry.is_regular_file() && entry.path().extension() == L".json")
            packs.push_back(entry.path().stem().wstring());
    }
    return packs;
}

bool ResourcePackManager::UninstallPack(const std::wstring& packName)
{
    std::wstring themesPath = GetThemesPath();
    std::wstring themePath = themesPath + L"\\" + packName + L".json";

    if (GetFileAttributesW(themePath.c_str()) != INVALID_FILE_ATTRIBUTES)
        return DeleteFileW(themePath.c_str()) != FALSE;

    return false;
}

} // namespace snowdesktop
