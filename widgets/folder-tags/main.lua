-- folder_tags.lua - macOS 风格文件夹颜色标签组件
name = l10n.tr("lua_widget.folder_tags.name")
useCustomStyle = true
followPersonalizationDefault = true
showTitle = true
bottomBarHover = true

-- Apple HIG 风格默认配色
bg = 0x1A2230
border = 0x38383A
alpha = 0.88
gradientEndA = 0.22

-- macOS 风格 7 种标签颜色（Apple HIG 色值）
local tagColors = {
    { id = "red",    color = 0xFF3B30, label = function() return l10n.tr("lua_widget.folder_tags.red") end },
    { id = "orange", color = 0xFF9500, label = function() return l10n.tr("lua_widget.folder_tags.orange") end },
    { id = "yellow", color = 0xFFCC00, label = function() return l10n.tr("lua_widget.folder_tags.yellow") end },
    { id = "green",  color = 0x34C759, label = function() return l10n.tr("lua_widget.folder_tags.green") end },
    { id = "blue",   color = 0x007AFF, label = function() return l10n.tr("lua_widget.folder_tags.blue") end },
    { id = "purple", color = 0xAF52DE, label = function() return l10n.tr("lua_widget.folder_tags.purple") end },
    { id = "gray",   color = 0x8E8E93, label = function() return l10n.tr("lua_widget.folder_tags.gray") end },
}

-- macOS 风格符号标签（SF Symbols 对应）
local tagSymbols = {
    { id = "work",      symbol = "💼", label = function() return l10n.tr("lua_widget.folder_tags.work") end },
    { id = "personal",  symbol = "👤", label = function() return l10n.tr("lua_widget.folder_tags.personal") end },
    { id = "urgent",    symbol = "⚡", label = function() return l10n.tr("lua_widget.folder_tags.urgent") end },
    { id = "important", symbol = "⭐", label = function() return l10n.tr("lua_widget.folder_tags.important") end },
    { id = "archive",   symbol = "📦", label = function() return l10n.tr("lua_widget.folder_tags.archive") end },
    { id = "project",   symbol = "📁", label = function() return l10n.tr("lua_widget.folder_tags.project") end },
}

settings = {
    fields = {
        { key = "autoApply", label = "Auto-apply on new folders", type = "bool", default = false },
    }
}

-- 桌面文件夹路径缓存
local desktopFolders = {}
local selectedFolderIdx = 0

-- ── 配置加载 ─────────────────────────────────────────────────────

local function loadConfig()
    local theme = widget.theme()
    if theme then
        if theme.contentTheme == 1 then
            bg = 0xF5F5F7
            border = 0xE0E0E0
            alpha = 0.92
        else
            bg = 0x1A2230
            border = 0x38383A
            alpha = 0.88
        end
    end
end

-- ── 文件夹扫描 ───────────────────────────────────────────────────

local function scanDesktopFolders()
    desktopFolders = {}
    local allItems = desktop.items()
    if not allItems then return end
    for _, item in ipairs(allItems) do
        if item.isFolder then
            local savedTag = storage.get("tag_" .. (item.name or ""))
            local savedSymbol = storage.get("symbol_" .. (item.name or ""))
            table.insert(desktopFolders, {
                name = item.name or "unknown",
                path = item.path or "",
                tagColor = savedTag or "",
                tagSymbol = savedSymbol or "",
            })
        end
    end
end

-- ── 渲染 ─────────────────────────────────────────────────────────

function render()
    loadConfig()
    local w = layout.width()
    local h = layout.height()
    local pad = layout.cu(12)
    local headerH = layout.cu(28)
    local rowH = layout.cu(32)
    local colorDotSz = layout.cu(16)
    local symbolSz = layout.cu(18)

    -- 标题
    draw.text(pad, pad, "📁  " .. name, layout.fontCu(16), 0xCCCCCC, w - pad * 2, true)
    local y = pad + headerH + layout.cu(4)

    -- 扫描桌面文件夹
    scanDesktopFolders()

    if #desktopFolders == 0 then
        local msg = "No folders on desktop"
        local sz = layout.fontCu(13)
        local m = draw.measureText(msg, sz, 0, true)
        draw.text((w - m.width) * 0.5, (h - m.height) * 0.5, msg, sz, 0x999999, w, true)
        return
    end

    -- 文件夹列表
    local contentH = h - y - pad
    local maxVisible = math.floor(contentH / rowH)
    local visibleCount = math.min(#desktopFolders, maxVisible)

    for i = 1, visibleCount do
        local folder = desktopFolders[i]
        local isSelected = (selectedFolderIdx == i)

        -- 行背景
        if isSelected then
            draw.fillRect(pad, y, w - pad * 2, rowH - layout.cu(2), 0x2A3444, 0.5)
        end

        -- 颜色标签圆点
        local colorX = pad + layout.cu(4)
        local colorY = y + (rowH - colorDotSz) * 0.5
        if folder.tagColor ~= "" then
            -- 查找颜色值
            for _, tc in ipairs(tagColors) do
                if tc.id == folder.tagColor then
                    draw.fillCircle(colorX + colorDotSz * 0.5, colorY + colorDotSz * 0.5,
                        colorDotSz * 0.4, tc.color, 1.0)
                    break
                end
            end
        else
            draw.fillCircle(colorX + colorDotSz * 0.5, colorY + colorDotSz * 0.5,
                colorDotSz * 0.4, 0x555555, 0.3)
        end

        -- 符号标签
        local symbolX = pad + layout.cu(24)
        if folder.tagSymbol ~= "" then
            for _, ts in ipairs(tagSymbols) do
                if ts.id == folder.tagSymbol then
                    draw.text(symbolX, y + (rowH - symbolSz) * 0.5,
                        ts.symbol, symbolSz, 0xFFFFFF, symbolSz, true)
                    break
                end
            end
        end

        -- 文件夹名称
        local nameX = pad + layout.cu(44)
        draw.text(nameX, y + (rowH - layout.fontCu(13)) * 0.5,
            "📁 " .. folder.name, layout.fontCu(13), 0xCCCCCC, w - nameX - pad, true)

        y = y + rowH
    end

    -- 颜色选择器（底部）
    local colorPickerY = h - pad - layout.cu(28)
    draw.text(pad, colorPickerY, "Colors:", layout.fontCu(11), 0x888888, w - pad * 2, true)
    local dotX = pad + layout.cu(50)
    for _, tc in ipairs(tagColors) do
        draw.fillCircle(dotX + colorDotSz * 0.5, colorPickerY + layout.cu(14),
            colorDotSz * 0.45, tc.color, 1.0)
        dotX = dotX + colorDotSz + layout.cu(4)
    end

    -- 符号选择器（底部第二行）
    local symbolPickerY = colorPickerY + layout.cu(28)
    draw.text(pad, symbolPickerY, "Tags:", layout.fontCu(11), 0x888888, w - pad * 2, true)
    local symX = pad + layout.cu(50)
    for _, ts in ipairs(tagSymbols) do
        draw.text(symX, symbolPickerY + layout.cu(2),
            ts.symbol, layout.fontCu(14), 0xFFFFFF, layout.cu(20), true)
        symX = symX + layout.cu(22)
    end
end

-- ── 交互 ─────────────────────────────────────────────────────────

function onClick(x, y, button, delta)
    if button ~= 1 then return end
    loadConfig()
    local pad = layout.cu(12)
    local headerH = layout.cu(28)
    local rowH = layout.cu(32)
    local colorDotSz = layout.cu(16)

    local contentY = pad + headerH + layout.cu(4)
    local contentH = layout.height() - contentY - pad - layout.cu(60)
    local maxVisible = math.floor(contentH / rowH)

    -- 点击文件夹行：选中
    for i = 1, math.min(#desktopFolders, maxVisible) do
        local rowY = contentY + (i - 1) * rowH
        if y >= rowY and y <= rowY + rowH then
            selectedFolderIdx = i
            return
        end
    end

    -- 点击颜色选择器：应用颜色到选中文件夹
    local colorPickerY = layout.height() - pad - layout.cu(28)
    if y >= colorPickerY and y <= colorPickerY + layout.cu(24) then
        local dotX = pad + layout.cu(50)
        for _, tc in ipairs(tagColors) do
            if x >= dotX and x <= dotX + colorDotSz + layout.cu(4) then
                if selectedFolderIdx > 0 and selectedFolderIdx <= #desktopFolders then
                    local folder = desktopFolders[selectedFolderIdx]
                    storage.set("tag_" .. folder.name, tc.id)
                end
                return
            end
            dotX = dotX + colorDotSz + layout.cu(4)
        end
    end

    -- 点击符号选择器：应用符号到选中文件夹
    local symbolPickerY = colorPickerY + layout.cu(28)
    if y >= symbolPickerY and y <= symbolPickerY + layout.cu(24) then
        local symX = pad + layout.cu(50)
        for _, ts in ipairs(tagSymbols) do
            if x >= symX and x <= symX + layout.cu(22) then
                if selectedFolderIdx > 0 and selectedFolderIdx <= #desktopFolders then
                    local folder = desktopFolders[selectedFolderIdx]
                    storage.set("symbol_" .. folder.name, ts.id)
                end
                return
            end
            symX = symX + layout.cu(22)
        end
    end
end

-- ── 右键菜单 ─────────────────────────────────────────────────────

function getContextMenu()
    return {
        { id = 1, label = l10n.tr("lua_widget.folder_tags.remove"), icon = utf8.char(0xE5CD), iconFont = "fluent" },
    }
end

function onMenu(id)
    if id == 1 then
        if selectedFolderIdx > 0 and selectedFolderIdx <= #desktopFolders then
            local folder = desktopFolders[selectedFolderIdx]
            storage.remove("tag_" .. folder.name)
            storage.remove("symbol_" .. folder.name)
        end
    end
end

-- ── 桌面变化回调 ─────────────────────────────────────────────────

function onDesktopChanged(reason)
    scanDesktopFolders()
    selectedFolderIdx = 0
end
