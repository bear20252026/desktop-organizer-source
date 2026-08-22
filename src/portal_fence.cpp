/**
 * @file portal_fence.cpp
 * @brief Portal Fence 实现 — 文件夹映射到桌面围栏（数据/逻辑层）
 *
 * D2D1 渲染由集成到主程序时的调用方负责，本文件只实现围栏管理逻辑。
 */

#include "portal_fence.h"
#include <shlobj.h>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace snowdesktop {

PortalFenceManager::PortalFenceManager() = default;

std::wstring PortalFenceManager::GenerateId()
{
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    std::wstring id = L"fence_";
    id += std::to_wstring(now % 1000000);
    return id;
}

std::wstring PortalFenceManager::GetConfigPath()
{
    wchar_t appData[MAX_PATH]{};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA,
            nullptr, SHGFP_TYPE_CURRENT, appData)))
    {
        std::wstring dir = std::wstring(appData) + L"\\SnowDesktop";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir + L"\\portal_fences.json";
    }
    return L"";
}

PortalFenceConfig PortalFenceManager::CreateFence(
    const std::wstring& folderPath,
    const std::wstring& name)
{
    PortalFenceConfig fence;
    fence.id = GenerateId();
    fence.folderPath = folderPath;
    fence.name = name.empty()
        ? std::filesystem::path(folderPath).filename().wstring()
        : name;
    // 默认位置：屏幕中央偏左上
    fence.x = 100 + static_cast<int>(fences_.size()) * 30;
    fence.y = 100 + static_cast<int>(fences_.size()) * 30;
    fence.width = 300;
    fence.height = 400;

    fences_.push_back(fence);
    RefreshFence(fence.id);
    SaveConfig();
    return fence;
}

void PortalFenceManager::DeleteFence(const std::wstring& fenceId)
{
    fences_.erase(
        std::remove_if(fences_.begin(), fences_.end(),
            [&](const PortalFenceConfig& f) { return f.id == fenceId; }),
        fences_.end());
    fileCache_.erase(fenceId);
    SaveConfig();
}

std::vector<FenceFileEntry> PortalFenceManager::GetFenceFiles(
    const std::wstring& fenceId)
{
    auto it = fileCache_.find(fenceId);
    if (it != fileCache_.end())
        return it->second;

    // 缓存未命中，扫描文件夹
    RefreshFence(fenceId);
    return fileCache_[fenceId];
}

void PortalFenceManager::RefreshFence(const std::wstring& fenceId)
{
    // 找到围栏配置
    PortalFenceConfig* fence = nullptr;
    for (auto& f : fences_)
    {
        if (f.id == fenceId)
        {
            fence = &f;
            break;
        }
    }
    if (!fence) return;

    std::vector<FenceFileEntry> entries;
    namespace fs = std::filesystem;

    if (!fs::exists(fence->folderPath) || !fs::is_directory(fence->folderPath))
    {
        fileCache_[fenceId] = entries;
        return;
    }

    for (const auto& entry : fs::directory_iterator(fence->folderPath))
    {
        // 跳过隐藏文件（除非设置显示）
        if (!fence->showHiddenFiles)
        {
            DWORD attrs = GetFileAttributesW(entry.path().c_str());
            if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_HIDDEN))
                continue;
        }

        FenceFileEntry fe;
        fe.name = entry.path().filename().wstring();
        fe.fullPath = entry.path().wstring();
        fe.extension = entry.path().extension().wstring();
        fe.isDirectory = entry.is_directory();

        if (entry.is_regular_file())
        {
            std::error_code ec;
            fe.fileSize = fs::file_size(entry.path(), ec);
        }

        WIN32_FILE_ATTRIBUTE_DATA fad{};
        if (GetFileAttributesExW(entry.path().c_str(),
                GetFileExInfoStandard, &fad))
        {
            fe.lastModified = fad.ftLastWriteTime;
        }

        entries.push_back(fe);
    }

    // 排序
    std::sort(entries.begin(), entries.end(),
        [&](const FenceFileEntry& a, const FenceFileEntry& b) {
            // 目录优先
            if (a.isDirectory != b.isDirectory)
                return a.isDirectory;
            switch (fence->sortBy)
            {
            case 1: // 按日期
                return CompareFileTime(&a.lastModified, &b.lastModified) > 0;
            case 2: // 按大小
                return a.fileSize > b.fileSize;
            case 3: // 按类型
                return a.extension < b.extension;
            default: // 按名称
                return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0;
            }
        });

    fileCache_[fenceId] = entries;
}

void PortalFenceManager::RefreshAll()
{
    for (const auto& fence : fences_)
        RefreshFence(fence.id);
}

// D2D1 渲染函数（DrawTitleBar/DrawFileList/DrawFence/DrawAllFences/GetFenceRect）
// 将在集成到主程序渲染管线时实现，本文件只负责围栏数据/逻辑层。

bool PortalFenceManager::HandleClick(int x, int y)
{
    for (auto it = fences_.rbegin(); it != fences_.rend(); ++it)
    {
        const auto& f = *it;
        int r = f.x + f.width;
        int b = f.y + f.height;
        if (x >= f.x && x <= r && y >= f.y && y <= b)
        {
            activeFenceId_ = f.id;

            // 点击标题栏折叠/展开按钮
            int cbLeft = r - 28, cbTop = f.y + 4;
            int cbRight = r - 4, cbBottom = f.y + 28;
            if (x >= cbLeft && x <= cbRight && y >= cbTop && y <= cbBottom)
            {
                auto& mutableFence = const_cast<PortalFenceConfig&>(*it);
                mutableFence.collapsed = !mutableFence.collapsed;
                SaveConfig();
                return true;
            }

            // 点击文件列表区域
            if (!f.collapsed && y > f.y + 36)
            {
                float rowH = 28.0f;
                int fileIdx = static_cast<int>((y - f.y - 36) / rowH);
                auto files = fileCache_.find(it->id);
                if (files != fileCache_.end() &&
                    fileIdx >= 0 && fileIdx < static_cast<int>(files->second.size()))
                {
                    const auto& entry = files->second[fileIdx];
                    // 双击打开文件（简化：单击选中，此处直接打开）
                    if (!entry.isDirectory)
                    {
                        ShellExecuteW(nullptr, L"open",
                            entry.fullPath.c_str(),
                            nullptr, nullptr, SW_SHOW);
                    }
                    else
                    {
                        // 打开文件夹
                        ShellExecuteW(nullptr, L"explore",
                            entry.fullPath.c_str(),
                            nullptr, nullptr, SW_SHOW);
                    }
                }
            }

            return true;
        }
    }
    activeFenceId_.clear();
    return false;
}

bool PortalFenceManager::HandleDrag(int x, int y)
{
    // 简化：拖拽移动围栏位置
    if (activeFenceId_.empty()) return false;

    for (auto& fence : fences_)
    {
        if (fence.id == activeFenceId_)
        {
            // 实际拖拽逻辑需要记录拖拽起始点，此处简化
            return true;
        }
    }
    return false;
}

bool PortalFenceManager::HandleFileDrop(
    const std::wstring& filePath,
    int x, int y)
{
    // 检查是否拖放到了某个围栏上
    for (auto& fence : fences_)
    {
        int r = fence.x + fence.width;
        int b = fence.y + fence.height;
        if (x >= fence.x && x <= r && y >= fence.y && y <= b)
        {
            // 移动文件到围栏对应的文件夹
            namespace fs = std::filesystem;
            fs::path src(filePath);
            fs::path destDir(fence.folderPath);
            fs::path destPath = destDir / src.filename();

            // 冲突处理
            if (fs::exists(destPath))
            {
                std::wstring stem = src.stem().wstring();
                std::wstring ext = src.extension().wstring();
                int counter = 1;
                while (fs::exists(destPath) && counter < 1000)
                {
                    destPath = destDir /
                        (stem + L"_" + std::to_wstring(counter) + ext);
                    counter++;
                }
            }

            std::error_code ec;
            fs::rename(src, destPath, ec);
            if (ec)
            {
                // 跨分区：copy + delete
                fs::copy_file(src, destPath,
                    fs::copy_options::overwrite_existing, ec);
                if (!ec) fs::remove(src, ec);
            }

            if (!ec)
            {
                RefreshFence(fence.id);
                return true;
            }
        }
    }
    return false;
}

bool PortalFenceManager::SaveConfig()
{
    std::wstring configPath = GetConfigPath();
    if (configPath.empty()) return false;

    std::ofstream file(configPath);
    if (!file.is_open()) return false;

    file << "[\n";
    for (size_t i = 0; i < fences_.size(); ++i)
    {
        const auto& f = fences_[i];
        auto toUtf8 = [](const std::wstring& ws) -> std::string {
            if (ws.empty()) return "";
            int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(),
                static_cast<int>(ws.size()), nullptr, 0, nullptr, nullptr);
            std::string s(len, '\0');
            WideCharToMultiByte(CP_UTF8, 0, ws.c_str(),
                static_cast<int>(ws.size()), s.data(), len, nullptr, nullptr);
            return s;
        };
        file << "  {\n";
        file << "    \"id\": \"" << toUtf8(f.id) << "\",\n";
        file << "    \"name\": \"" << toUtf8(f.name) << "\",\n";
        file << "    \"folderPath\": \"" << toUtf8(f.folderPath) << "\",\n";
        file << "    \"x\": " << f.x << ",\n";
        file << "    \"y\": " << f.y << ",\n";
        file << "    \"width\": " << f.width << ",\n";
        file << "    \"height\": " << f.height << ",\n";
        file << "    \"collapsed\": " << (f.collapsed ? "true" : "false") << ",\n";
        file << "    \"bgAlpha\": " << f.bgAlpha << ",\n";
        file << "    \"bgR\": " << f.bgR << ",\n";
        file << "    \"bgG\": " << f.bgG << ",\n";
        file << "    \"bgB\": " << f.bgB << "\n";
        file << "  }" << (i < fences_.size() - 1 ? "," : "") << "\n";
    }
    file << "]\n";
    return true;
}

bool PortalFenceManager::LoadConfig()
{
    std::wstring configPath = GetConfigPath();
    if (configPath.empty()) return false;

    std::ifstream file(configPath);
    if (!file.is_open()) return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    // 简单 JSON 解析（逐字段提取）
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

    auto extractInt = [&json](const std::string& key) -> int {
        std::string search = "\"" + key + "\"";
        auto pos = json.find(search);
        if (pos == std::string::npos) return 0;
        auto colon = json.find(':', pos + search.length());
        if (colon == std::string::npos) return 0;
        auto start = colon + 1;
        while (start < json.length() && json[start] == ' ') start++;
        auto end = start;
        while (end < json.length() && (isdigit(json[end]) || json[end] == '-'))
            end++;
        try { return std::stoi(json.substr(start, end - start)); }
        catch (...) { return 0; }
    };

    auto toWstring = [](const std::string& s) -> std::wstring {
        if (s.empty()) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(),
            static_cast<int>(s.size()), nullptr, 0);
        std::wstring ws(len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(),
            static_cast<int>(s.size()), ws.data(), len);
        return ws;
    };

    // 解析每个围栏对象
    size_t pos = 0;
    while ((pos = json.find("\"id\":", pos)) != std::string::npos)
    {
        PortalFenceConfig fence;
        fence.id = toWstring(extractString("id"));
        fence.name = toWstring(extractString("name"));
        fence.folderPath = toWstring(extractString("folderPath"));
        fence.x = extractInt("x");
        fence.y = extractInt("y");
        fence.width = extractInt("width");
        fence.height = extractInt("height");
        fence.collapsed = json.find("\"collapsed\": true") != std::string::npos;

        if (!fence.id.empty() && !fence.folderPath.empty())
        {
            fences_.push_back(fence);
            RefreshFence(fence.id);
        }
        pos += 5;
    }

    return true;
}

} // namespace snowdesktop
