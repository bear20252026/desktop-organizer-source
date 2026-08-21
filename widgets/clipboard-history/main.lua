-- clipboard_history.lua - macOS Tahoe 风格剪贴板历史组件
name = l10n.tr("lua_widget.clipboard_history.name")
useCustomStyle = true
followPersonalizationDefault = true
showTitle = true
bottomBarHover = true

-- Apple HIG 风格默认配色
bg = 0x1A2230
border = 0x38383A
alpha = 0.88
gradientEndA = 0.22

-- macOS Tahoe: 8 小时历史保留（可配置：30分钟/8小时/7天）
-- 默认 8 小时 = 28800 秒
local kMaxHistoryAge = 28800
local kMaxItems = 50

settings = {
    fields = {
        { key = "maxAgeHours", label = "Retention (hours)", type = "float", default = 8.0, min = 0.5, max = 168.0 },
        { key = "maxItems", label = "Max items", type = "int", default = 50, min = 10, max = 200 },
    }
}

-- 剪贴板历史表：{text, timestamp, source}
local history = {}
local lastClipboard = ""
local selectedIdx = 0

-- ── 配置加载 ─────────────────────────────────────────────────────

local function loadConfig()
    local theme = widget.theme()
    if theme then
        if theme.contentTheme == 1 then
            bg = 0xF5F5F7; border = 0xE0E0E0; alpha = 0.92
        else
            bg = 0x1A2230; border = 0x38383A; alpha = 0.88
        end
    end
    kMaxHistoryAge = (tonumber(storage.get("maxAgeHours")) or 8.0) * 3600
    kMaxItems = tonumber(storage.get("maxItems")) or 50
end

-- ── 时间格式化 ──────────────────────────────────────────────────

local function formatAgo(seconds)
    if seconds < 60 then
        return math.floor(seconds) .. " " .. l10n.tr("lua_widget.clipboard_history.ago_seconds")
    elseif seconds < 3600 then
        return math.floor(seconds / 60) .. " " .. l10n.tr("lua_widget.clipboard_history.ago_minutes")
    else
        return math.floor(seconds / 3600) .. " " .. l10n.tr("lua_widget.clipboard_history.ago_hours")
    end
end

-- ── 历史管理 ─────────────────────────────────────────────────────

local function loadHistory()
    history = {}
    local raw = storage.get("history") or ""
    local now = os.time()
    for line in raw:gmatch("[^\n]+") do
        local ts, text = line:match("^(%d+)|(.*)$")
        if ts and text then
            local t = tonumber(ts)
            if t and (now - t) < kMaxHistoryAge then
                table.insert(history, { text = text, timestamp = t })
            end
        end
    end
    -- 按时间倒序
    table.sort(history, function(a, b) return a.timestamp > b.timestamp end)
end

local function saveHistory()
    local parts = {}
    for _, item in ipairs(history) do
        table.insert(parts, item.timestamp .. "|" .. item.text)
    end
    storage.set("history", table.concat(parts, "\n"))
end

local function addToHistory(text)
    if not text or text == "" or text == lastClipboard then return end
    -- 去重
    for i, item in ipairs(history) do
        if item.text == text then
            table.remove(history, i)
            break
        end
    end
    table.insert(history, 1, { text = text, timestamp = os.time() })
    lastClipboard = text
    -- 限制条目数
    while #history > kMaxItems do
        table.remove(history)
    end
    saveHistory()
end

local function deleteItem(idx)
    if idx >= 1 and idx <= #history then
        table.remove(history, idx)
        saveHistory()
    end
end

local function clearAll()
    history = {}
    lastClipboard = ""
    storage.remove("history")
end

-- ── 渲染 ─────────────────────────────────────────────────────────

function render()
    loadConfig()
    loadHistory()

    local w = layout.width()
    local h = layout.height()
    local pad = layout.cu(12)
    local headerH = layout.cu(28)
    local rowH = layout.cu(44)
    local iconSz = layout.cu(14)

    local y = pad

    -- 标题 + 数量
    local titleText = name
    if #history > 0 then
        titleText = titleText .. " (" .. #history .. l10n.tr("lua_widget.clipboard_history.items_count") .. ")"
    end
    draw.text(pad, y, "📋  " .. titleText, layout.fontCu(16), 0xCCCCCC, w - pad * 2, true)
    y = y + headerH + layout.cu(4)

    if #history == 0 then
        local msg = l10n.tr("lua_widget.clipboard_history.empty")
        local sz = layout.fontCu(14)
        local m = draw.measureText(msg, sz, 0, true)
        draw.text((w - m.width) * 0.5, (h - m.height) * 0.5, msg, sz, 0x777777, w, true)
        return
    end

    -- 历史列表
    local contentH = h - y - pad
    local maxVisible = math.floor(contentH / rowH)
    local now = os.time()

    for i = 1, math.min(#history, maxVisible) do
        local item = history[i]
        local isSelected = (selectedIdx == i)

        -- 行背景
        if isSelected then
            draw.fillRect(pad, y, w - pad * 2, rowH - layout.cu(2), 0x2A3444, 0.6)
        end

        -- 类型图标
        local typeIcon = "📝"  -- 默认文本
        if item.text:match("^%[file%]") then typeIcon = "📄"
        elseif item.text:match("^%[image%]") then typeIcon = "🖼️"
        end

        -- 内容预览（截断到30字符）
        local preview = item.text:gsub("[\r\n]+", " "):sub(1, 30)
        if #item.text > 30 then preview = preview .. "…" end

        -- 时间
        local agoStr = formatAgo(now - item.timestamp)

        -- 绘制：图标 + 内容 + 时间
        local iconX = pad + layout.cu(4)
        local contentX = iconX + layout.cu(20)
        local timeX = w - pad - layout.cu(60)

        draw.text(iconX, y + (rowH - iconSz) * 0.5, typeIcon, iconSz, 0xFFFFFF, iconSz, true)
        draw.text(contentX, y + layout.cu(6), preview, layout.fontCu(13), 0xCCCCCC, timeX - contentX - layout.cu(4), true)
        draw.text(timeX, y + layout.cu(8), agoStr, layout.fontCu(11), 0x888888, layout.cu(56), true)

        -- 剪贴板内容预览（第二行，仅当内容较长时）
        if #item.text > 30 then
            local subPreview = item.text:sub(31, 80):gsub("[\r\n]+", " ")
            draw.text(contentX, y + layout.cu(22), subPreview, layout.fontCu(11), 0x666666, timeX - contentX - layout.cu(4), true)
        end

        y = y + rowH
    end
end

-- ── 交互 ─────────────────────────────────────────────────────────

function onClick(x, y, button, delta)
    if button ~= 1 then return end
    loadConfig()
    loadHistory()

    local pad = layout.cu(12)
    local headerH = layout.cu(28)
    local rowH = layout.cu(44)
    local startY = pad + headerH + layout.cu(4)

    for i = 1, #history do
        if y >= startY and y <= startY + rowH then
            selectedIdx = i
            -- 双击：复制到剪贴板
            if delta >= 2 then
                ui.copyToClipboard(history[i].text)
                return
            end
            return
        end
        startY = startY + rowH
    end
end

-- ── 右键菜单 ─────────────────────────────────────────────────────

function getContextMenu()
    local items = {}
    if selectedIdx > 0 and selectedIdx <= #history then
        table.insert(items, { id = 1, label = l10n.tr("lua_widget.clipboard_history.paste"), icon = utf8.char(0xE77F), iconFont = "fluent" })
        table.insert(items, { id = 2, label = l10n.tr("lua_widget.clipboard_history.copy"), icon = utf8.char(0xE8C8), iconFont = "fluent" })
        table.insert(items, { id = 3, label = l10n.tr("lua_widget.clipboard_history.delete"), icon = utf8.char(0xE74D), iconFont = "fluent" })
    end
    table.insert(items, { id = 4, label = l10n.tr("lua_widget.clipboard_history.clear_all"), icon = utf8.char(0xE74D), iconFont = "fluent" })
    return items
end

function onMenu(id)
    if id == 1 then
        -- 粘贴：复制到剪贴板并触发粘贴
        if selectedIdx > 0 and selectedIdx <= #history then
            ui.copyToClipboard(history[selectedIdx].text)
        end
    elseif id == 2 then
        -- 复制到剪贴板
        if selectedIdx > 0 and selectedIdx <= #history then
            ui.copyToClipboard(history[selectedIdx].text)
        end
    elseif id == 3 then
        -- 删除选中项
        if selectedIdx > 0 and selectedIdx <= #history then
            deleteItem(selectedIdx)
            selectedIdx = 0
        end
    elseif id == 4 then
        clearAll()
        selectedIdx = 0
    end
end

-- ── 键盘快捷键 ───────────────────────────────────────────────────

function onKeyPress(key)
    if key == "Escape" then
        selectedIdx = 0
    elseif key == "Delete" and selectedIdx > 0 then
        deleteItem(selectedIdx)
        selectedIdx = math.max(0, selectedIdx - 1)
    elseif key == "Enter" and selectedIdx > 0 then
        ui.copyToClipboard(history[selectedIdx].text)
    end
end

-- ── 定时器：监控剪贴板 ──────────────────────────────────────────

function onTimer()
    loadConfig()
    loadHistory()
    -- 清理过期条目
    local now = os.time()
    local changed = false
    for i = #history, 1, -1 do
        if (now - history[i].timestamp) >= kMaxHistoryAge then
            table.remove(history, i)
            changed = true
        end
    end
    if changed then saveHistory() end
end

-- ── 初始化 ───────────────────────────────────────────────────────

function onVisible()
    loadConfig()
    loadHistory()
end

function onOpen()
    selectedIdx = 0
    loadHistory()
end
