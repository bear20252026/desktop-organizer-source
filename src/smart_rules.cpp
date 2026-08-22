/**
 * @file smart_rules.cpp
 * @brief Smart Desktop 规则引擎实现
 *
 * 灵感来源：FolderFresh + Desktop Fences+ Smart Desktop
 */

#include "smart_rules.h"
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <filesystem>
#include <cmath>

namespace snowdesktop {

std::wstring SmartRuleEngine::GetRulesPath()
{
    wchar_t appData[MAX_PATH]{};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA,
            nullptr, SHGFP_TYPE_CURRENT, appData)))
    {
        std::wstring dir = std::wstring(appData) + L"\\SnowDesktop";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir + L"\\smart_rules.json";
    }
    return L"";
}

bool SmartRuleEngine::LoadRules(const std::wstring& path)
{
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    // 简化 JSON 解析：逐条读取规则
    auto extractString = [&json](const std::string& key, size_t startPos = 0) -> std::string {
        std::string search = "\"" + key + "\"";
        auto pos = json.find(search, startPos);
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

    auto extractBool = [&json](const std::string& key, size_t startPos = 0) -> bool {
        std::string search = "\"" + key + "\"";
        auto pos = json.find(search, startPos);
        if (pos == std::string::npos) return false;
        auto colon = json.find(':', pos + search.length());
        if (colon == std::string::npos) return false;
        return json.find("true", colon) == colon + 1;
    };

    // 解析规则
    size_t pos = 0;
    while ((pos = json.find("\"name\":", pos)) != std::string::npos)
    {
        SmartRule rule;
        rule.id = extractString("id", pos > 20 ? pos - 20 : 0);
        if (rule.id.empty())
            rule.id = "rule_" + std::to_string(rules_.size() + 1);
        rule.name = extractString("name", pos);
        rule.enabled = extractBool("enabled", pos);
        if (!rule.name.empty())
        {
            rule.action.type = ActionType::Move;
            rule.action.targetPath = extractString("target", pos);
            rules_.push_back(rule);
        }
        pos += 7;
    }

    return true;
}

bool SmartRuleEngine::SaveRules(const std::wstring& path)
{
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << "[\n";
    for (size_t i = 0; i < rules_.size(); ++i)
    {
        const auto& r = rules_[i];
        file << "  {\n";
        file << "    \"id\": \"" << r.id << "\",\n";
        file << "    \"name\": \"" << r.name << "\",\n";
        file << "    \"enabled\": " << (r.enabled ? "true" : "false") << ",\n";
        file << "    \"priority\": " << r.priority << ",\n";
        file << "    \"action\": \"" << static_cast<int>(r.action.type) << "\",\n";
        file << "    \"target\": \"" << r.action.targetPath << "\"\n";
        file << "  }" << (i < rules_.size() - 1 ? "," : "") << "\n";
    }
    file << "]\n";
    return true;
}

void SmartRuleEngine::AddRule(const SmartRule& rule)
{
    rules_.push_back(rule);
    SaveRules(GetRulesPath());
}

void SmartRuleEngine::RemoveRule(const std::string& ruleId)
{
    rules_.erase(
        std::remove_if(rules_.begin(), rules_.end(),
            [&](const SmartRule& r) { return r.id == ruleId; }),
        rules_.end());
    SaveRules(GetRulesPath());
}

// ── 字符串匹配 ──────────────────────────────────────────────────

bool SmartRuleEngine::MatchString(const std::string& value, ConditionOp op,
                                   const std::string& pattern)
{
    switch (op)
    {
    case ConditionOp::Equals:
        return value == pattern;
    case ConditionOp::NotEquals:
        return value != pattern;
    case ConditionOp::Contains:
        return value.find(pattern) != std::string::npos;
    case ConditionOp::StartsWith:
        return value.size() >= pattern.size() &&
               value.substr(0, pattern.size()) == pattern;
    case ConditionOp::EndsWith:
        return value.size() >= pattern.size() &&
               value.substr(value.size() - pattern.size()) == pattern;
    case ConditionOp::MatchesRegex:
        try {
            std::regex re(pattern, std::regex::icase);
            return std::regex_search(value, re);
        } catch (...) {
            return false;
        }
    default:
        return false;
    }
}

bool SmartRuleEngine::MatchNumeric(double value, ConditionOp op,
                                    double threshold, double threshold2)
{
    switch (op)
    {
    case ConditionOp::GreaterThan:
        return value > threshold;
    case ConditionOp::LessThan:
        return value < threshold;
    case ConditionOp::GreaterEqual:
        return value >= threshold;
    case ConditionOp::LessEqual:
        return value <= threshold;
    case ConditionOp::Between:
        return value >= threshold && value <= threshold2;
    case ConditionOp::Equals:
        return std::abs(value - threshold) < 0.001;
    default:
        return false;
    }
}

// ── 条件评估 ────────────────────────────────────────────────────

bool SmartRuleEngine::EvaluateCondition(const RuleCondition& cond,
                                         const std::wstring& filePath,
                                         const WIN32_FILE_ATTRIBUTE_DATA& attrs)
{
    namespace fs = std::filesystem;
    fs::path p(filePath);
    bool isDir = (attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    bool isHidden = (attrs.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;

    auto toLower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    };

    // 文件名
    if (cond.field == ConditionField::FileName)
    {
        std::string name = toLower(p.filename().string());
        if (cond.op == ConditionOp::In)
        {
            for (const auto& v : cond.values)
                if (name.find(toLower(v)) != std::string::npos) return true;
            return false;
        }
        return MatchString(name, cond.op, toLower(cond.value));
    }

    // 扩展名
    if (cond.field == ConditionField::Extension)
    {
        std::string ext = toLower(p.extension().string());
        if (!ext.empty() && ext[0] == '.') ext = ext.substr(1);
        if (cond.op == ConditionOp::In)
        {
            for (const auto& v : cond.values)
                if (ext == toLower(v)) return true;
            return false;
        }
        return MatchString(ext, cond.op, toLower(cond.value));
    }

    // 完整路径
    if (cond.field == ConditionField::FullPath)
    {
        return MatchString(toLower(p.string()), cond.op, toLower(cond.value));
    }

    // 文件大小
    if (cond.field == ConditionField::Size)
    {
        LARGE_INTEGER fileSize{};
        fileSize.LowPart = attrs.nFileSizeLow;
        fileSize.HighPart = attrs.nFileSizeHigh;
        return MatchNumeric(static_cast<double>(fileSize.QuadPart),
            cond.op, cond.numericValue, cond.numericValue2);
    }

    // 修改时间（天数）
    if (cond.field == ConditionField::ModifiedTime)
    {
        FILETIME ft = attrs.ftLastWriteTime;
        ULARGE_INTEGER ul{};
        ul.LowPart = ft.dwLowDateTime;
        ul.HighPart = ft.dwHighDateTime;
        double fileTime = static_cast<double>(ul.QuadPart) / 10000000.0;
        ULARGE_INTEGER now{};
        GetSystemTimeAsFileTime(reinterpret_cast<FILETIME*>(&now));
        double nowTime = static_cast<double>(now.QuadPart) / 10000000.0;
        double daysSince = (nowTime - fileTime) / 86400.0;

        if (cond.op == ConditionOp::OlderThanDays)
            return daysSince > cond.numericValue;
        if (cond.op == ConditionOp::NewerThanDays)
            return daysSince < cond.numericValue;
        return MatchNumeric(daysSince, cond.op, cond.numericValue, cond.numericValue2);
    }

    // 是否为目录
    if (cond.field == ConditionField::IsDirectory)
        return isDir == (cond.numericValue > 0.5);

    // 是否隐藏
    if (cond.field == ConditionField::IsHidden)
        return isHidden == (cond.numericValue > 0.5);

    return false;
}

// ── 文件评估 ────────────────────────────────────────────────────

RuleMatchResult SmartRuleEngine::EvaluateFile(const std::wstring& filePath)
{
    RuleMatchResult result;
    result.filePath = filePath;

    WIN32_FILE_ATTRIBUTE_DATA attrs{};
    if (!GetFileAttributesExW(filePath.c_str(), GetFileExInfoStandard, &attrs))
        return result;

    // 跳过目录
    if (attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return result;

    // 按优先级排序
    std::vector<SmartRule*> sorted;
    for (auto& r : rules_)
        if (r.enabled) sorted.push_back(&r);
    std::sort(sorted.begin(), sorted.end(),
        [](const SmartRule* a, const SmartRule* b) {
            return a->priority > b->priority;
        });

    for (const auto* rule : sorted)
    {
        if (rule->conditions.empty())
        {
            // 无条件 = 匹配所有
            result.matched = true;
            result.matchedRuleId = rule->id;
            result.actionType = rule->action.type;
            result.targetPath = rule->action.targetPath;
            return result;
        }

        bool allMatch = (rule->logic == LogicOp::And);
        for (const auto& cond : rule->conditions)
        {
            bool match = EvaluateCondition(cond, filePath, attrs);
            if (rule->logic == LogicOp::And)
                allMatch = allMatch && match;
            else if (rule->logic == LogicOp::Or)
            {
                if (match)
                {
                    allMatch = true;
                    break;
                }
            }
        }

        if (allMatch)
        {
            result.matched = true;
            result.matchedRuleId = rule->id;
            result.actionType = rule->action.type;
            result.targetPath = rule->action.targetPath;
            return result;
        }
    }

    return result;
}

// ── 目录执行 ────────────────────────────────────────────────────

RuleExecResult SmartRuleEngine::ExecuteRules(const std::wstring& directory)
{
    RuleExecResult result;
    namespace fs = std::filesystem;

    if (!fs::exists(directory) || !fs::is_directory(directory))
        return result;

    for (const auto& entry : fs::directory_iterator(directory))
    {
        if (!entry.is_regular_file()) continue;

        result.totalFiles++;
        RuleMatchResult match = EvaluateFile(entry.path().wstring());

        if (match.matched)
        {
            result.matchedFiles++;
            result.details.push_back(match);

            // 执行动作
            if (match.actionType == ActionType::Move && !match.targetPath.empty())
            {
                fs::path dest = fs::path(directory) / match.targetPath;
                fs::create_directories(dest);
                fs::path destFile = dest / entry.path().filename();
                std::error_code ec;
                fs::rename(entry.path(), destFile, ec);
                if (ec)
                {
                    fs::copy_file(entry.path(), destFile,
                        fs::copy_options::overwrite_existing, ec);
                    if (!ec) fs::remove(entry.path(), ec);
                }
                if (!ec) result.executedActions++;
            }
            else if (match.actionType == ActionType::Delete)
            {
                // 移到回收站（简化：直接删除，后续可改为 SHFileOperation）
                std::error_code ec;
                fs::remove(entry.path(), ec);
                if (!ec) result.executedActions++;
            }
            else
            {
                result.skippedFiles++;
            }
        }
        else
        {
            result.skippedFiles++;
        }
    }

    return result;
}

RuleExecResult SmartRuleEngine::PreviewRules(const std::wstring& directory)
{
    RuleExecResult result;
    namespace fs = std::filesystem;

    if (!fs::exists(directory) || !fs::is_directory(directory))
        return result;

    for (const auto& entry : fs::directory_iterator(directory))
    {
        if (!entry.is_regular_file()) continue;

        result.totalFiles++;
        RuleMatchResult match = EvaluateFile(entry.path().wstring());

        if (match.matched)
        {
            result.matchedFiles++;
            result.details.push_back(match);
        }
        else
        {
            result.skippedFiles++;
        }
    }

    return result;
}

// ── 默认规则 ────────────────────────────────────────────────────

void SmartRuleEngine::CreateDefaultRules()
{
    // 文档归档：30天以上的文档移到 Archive/Documents
    SmartRule docRule;
    docRule.id = "archive_old_documents";
    docRule.name = "Archive Old Documents (>30 days)";
    docRule.enabled = true;
    docRule.priority = 10;
    docRule.logic = LogicOp::And;
    docRule.conditions = {
        { ConditionField::Extension, ConditionOp::In, "", {"pdf","doc","docx","txt","rtf","odt","xls","xlsx","ppt","pptx"} },
        { ConditionField::ModifiedTime, ConditionOp::OlderThanDays, "", {}, 30.0 }
    };
    docRule.action.type = ActionType::Move;
    docRule.action.targetPath = "Archive/Documents";
    rules_.push_back(docRule);

    // 安装包清理：7天以上的 exe/msi 移到 Archive/Installers
    SmartRule installerRule;
    installerRule.id = "archive_old_installers";
    installerRule.name = "Archive Old Installers (>7 days)";
    installerRule.enabled = true;
    installerRule.priority = 5;
    installerRule.logic = LogicOp::And;
    installerRule.conditions = {
        { ConditionField::Extension, ConditionOp::In, "", {"exe","msi","appx","msix"} },
        { ConditionField::ModifiedTime, ConditionOp::OlderThanDays, "", {}, 7.0 }
    };
    installerRule.action.type = ActionType::Move;
    installerRule.action.targetPath = "Archive/Installers";
    rules_.push_back(installerRule);

    // 大文件警告：大于 500MB 的文件标为注意
    SmartRule largeRule;
    largeRule.id = "large_file_warning";
    largeRule.name = "Large File Tag (>500MB)";
    largeRule.enabled = true;
    largeRule.priority = 1;
    largeRule.logic = LogicOp::And;
    largeRule.conditions = {
        { ConditionField::Size, ConditionOp::GreaterThan, "", {}, 524288000.0 }
    };
    largeRule.action.type = ActionType::Tag;
    largeRule.action.tagName = "Large";
    largeRule.action.tagColor = 3; // 黄色
    rules_.push_back(largeRule);

    SaveRules(GetRulesPath());
}

} // namespace snowdesktop
