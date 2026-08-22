/**
 * @file smart_rules.h
 * @brief Smart Desktop 规则引擎 — 按内容/日期/大小自动分类
 *
 * 灵感来源：FolderFresh 规则引擎 + Desktop Fences+ Smart Desktop
 *
 * 规则格式（JSON）：
 * {
 *   "name": "Large Videos to Archive",
 *   "enabled": true,
 *   "conditions": [
 *     { "field": "extension", "op": "in", "value": ["mp4","mkv","mov"] },
 *     { "field": "size", "op": "gt", "value": 104857600 },
 *     { "field": "modified", "op": "older_than_days", "value": 30 }
 *   ],
 *   "logic": "AND",
 *   "action": "move",
 *   "target": "Archive/Videos"
 * }
 *
 * 支持的条件字段：
 *   - name: 文件名（模糊匹配/正则）
 *   - extension: 扩展名（精确/包含/列表）
 *   - size: 文件大小（大于/小于/等于/范围）
 *   - created/modified/accessed: 时间（早于/晚于/范围/天数）
 *   - path: 完整路径（包含/前缀/后缀）
 *
 * 支持的操作：
 *   - move: 移动到目标文件夹
 *   - copy: 复制到目标文件夹
 *   - delete: 删除到回收站
 *   - rename: 重命名（模式替换）
 *   - tag: 添加标签（颜色标签）
 *   - ignore: 忽略（不处理）
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include <windows.h>

namespace snowdesktop {

// 条件操作符
enum class ConditionOp
{
    Equals,          // 精确匹配
    NotEquals,       // 不匹配
    Contains,        // 包含
    StartsWith,      // 前缀
    EndsWith,        // 后缀
    MatchesRegex,    // 正则匹配
    In,              // 在列表中
    GreaterThan,     // 大于
    LessThan,        // 小于
    GreaterEqual,    // 大于等于
    LessEqual,       // 小于等于
    Between,         // 范围（含两端）
    OlderThanDays,   // 早于 N 天前
    NewerThanDays,   // 晚于 N 天前
};

// 条件字段
enum class ConditionField
{
    FileName,        // 文件名
    Extension,       // 扩展名
    FullPath,        // 完整路径
    Size,            // 文件大小（字节）
    CreatedTime,     // 创建时间
    ModifiedTime,    // 修改时间
    AccessedTime,    // 访问时间
    IsDirectory,     // 是否为目录
    IsHidden,        // 是否隐藏
};

// 动作类型
enum class ActionType
{
    Move,            // 移动到目标文件夹
    Copy,            // 复制到目标文件夹
    Delete,          // 删除到回收站
    Rename,          // 重命名
    Tag,             // 添加颜色标签
    Ignore,          // 忽略
};

// 逻辑运算符
enum class LogicOp
{
    And,             // 所有条件都满足
    Or,              // 任一条件满足
    None,            // 无条件（匹配所有）
};

// 规则条件
struct RuleCondition
{
    ConditionField field;
    ConditionOp op;
    std::string value;           // 字符串值
    std::vector<std::string> values;  // 列表值（用于 In 操作）
    double numericValue = 0.0;   // 数值（用于大小/天数）
    double numericValue2 = 0.0;  // 范围上限（用于 Between 操作）
};

// 规则动作
struct RuleAction
{
    ActionType type;
    std::string targetPath;      // 目标路径（Move/Copy）
    std::string pattern;         // 重命名模式（Rename）
    std::string replacement;     // 替换文本（Rename）
    std::string tagName;         // 标签名（Tag）
    int tagColor = -1;           // 标签颜色（Tag，-1=自动）
};

// 分类规则
struct SmartRule
{
    std::string id;              // 规则唯一 ID
    std::string name;            // 规则名称
    bool enabled = true;         // 是否启用
    int priority = 0;            // 优先级（高优先级先执行）
    LogicOp logic = LogicOp::And; // 条件逻辑
    std::vector<RuleCondition> conditions;
    RuleAction action;
};

// 规则匹配结果
struct RuleMatchResult
{
    bool matched = false;
    std::string matchedRuleId;
    std::wstring filePath;
    ActionType actionType;
    std::string targetPath;
};

// 规则执行结果
struct RuleExecResult
{
    int totalFiles = 0;
    int matchedFiles = 0;
    int executedActions = 0;
    int skippedFiles = 0;
    std::vector<RuleMatchResult> details;
};

/**
 * @brief Smart Desktop 规则引擎
 */
class SmartRuleEngine
{
public:
    /**
     * @brief 加载规则
     */
    bool LoadRules(const std::wstring& path);

    /**
     * @brief 保存规则
     */
    bool SaveRules(const std::wstring& path);

    /**
     * @brief 添加规则
     */
    void AddRule(const SmartRule& rule);

    /**
     * @brief 删除规则
     */
    void RemoveRule(const std::string& ruleId);

    /**
     * @brief 获取所有规则
     */
    const std::vector<SmartRule>& GetRules() const { return rules_; }

    /**
     * @brief 对单个文件执行所有匹配规则
     * @return 匹配结果（第一个匹配的规则）
     */
    RuleMatchResult EvaluateFile(const std::wstring& filePath);

    /**
     * @brief 对目录中的所有文件执行规则
     */
    RuleExecResult ExecuteRules(const std::wstring& directory);

    /**
     * @brief 预览规则执行结果（不实际移动文件）
     */
    RuleExecResult PreviewRules(const std::wstring& directory);

    /**
     * @brief 创建默认规则集
     */
    void CreateDefaultRules();

    /**
     * @brief 获取规则配置路径
     */
    static std::wstring GetRulesPath();

private:
    /**
     * @brief 检查单个条件是否匹配
     */
    bool EvaluateCondition(const RuleCondition& cond,
                           const std::wstring& filePath,
                           const WIN32_FILE_ATTRIBUTE_DATA& attrs);

    /**
     * @brief 字符串条件匹配
     */
    bool MatchString(const std::string& value, ConditionOp op,
                     const std::string& pattern);

    /**
     * @brief 数值条件匹配
     */
    bool MatchNumeric(double value, ConditionOp op,
                      double threshold, double threshold2 = 0.0);

    std::vector<SmartRule> rules_;

    // 管道集成：文件变化回调
    // 主程序的桌面文件监控（Shell change notification）触发时调用，
    // 规则引擎自动评估并执行匹配规则。
    // 用法：smartRules.OnFileChange(filePath);
    bool OnFileChange(const std::wstring& filePath)
    {
        RuleMatchResult match = EvaluateFile(filePath);
        if (!match.matched) return false;

        // 执行动作
        namespace fs = std::filesystem;
        if (match.actionType == ActionType::Move && !match.targetPath.empty())
        {
            fs::path src(filePath);
            fs::path destDir = src.parent_path() / match.targetPath;
            fs::create_directories(destDir);
            fs::path destFile = destDir / src.filename();
            std::error_code ec;
            fs::rename(src, destFile, ec);
            return !ec;
        }
        return false;
    }
};

} // namespace snowdesktop
