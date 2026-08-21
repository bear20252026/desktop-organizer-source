-- stacks.lua - macOS 风格文件堆叠组件
name = l10n.tr("lua_widget.stacks.name")
useCustomStyle = true
followPersonalizationDefault = true
showTitle = true
bottomBarHover = true

-- Apple HIG 风格默认配色
bg = 0x1A2230
border = 0xFFFFFF
alpha = 0.88
gradientEndA = 0.22

-- 分组方式：0=按类型, 1=按日期, 2=按扩展名
groupBy = 0
-- 各堆叠展开状态
expandedStacks = {}

settings = {
    fields = {
        { key = "groupBy", label = l10n.tr("lua_widget.stacks.group_by"), type = "int", default = 0, min = 0, max = 2 },
        { key = "showCount", label = l10n.tr("lua_widget.stacks.show_count"), type = "bool", default = true },
    }
}

-- ── 文件类型分组映射 ─────────────────────────────────────────────

local imageExts = { jpg=1, jpeg=1, png=1, gif=1, bmp=1, webp=1, svg=1, ico=1, tiff=1, tif=1, avif=1, heic=1, heif=1 }
local docExts = { pdf=1, doc=1, docx=1, txt=1, rtf=1, odt=1, xls=1, xlsx=1, ppt=1, pptx=1, csv=1, md=1, pages=1, numbers=1, keynote=1 }
local videoExts = { mp4=1, avi=1, mkv=1, mov=1, wmv=1, flv=1, webm=1, m4v=1, mpg=1, mpeg=1 }
local audioExts = { mp3=1, wav=1, flac=1, aac=1, ogg=1, m4a=1, wma=1, opus=1, aiff=1 }
local archiveExts = { zip=1, rar=1, ["7z"]=1, tar=1, gz=1, bz2=1, xz=1, lz=1, dmg=1, iso=1 }

local function getFileExt(name)
    local ext = name:match("%.([^%.]+)$")
    return ext and string.lower(ext) or ""
end

local function categorizeByType(item)
    if item.isShortcut or item.isLink then
        return l10n.tr("lua_widget.stacks.shortcuts")
    end
    local ext = getFileExt(item.name or "")
    if imageExts[ext] then return l10n.tr("lua_widget.stacks.images") end
    if docExts[ext] then return l10n.tr("lua_widget.stacks.documents") end
    if videoExts[ext] then return l10n.tr("lua_widget.stacks.videos") end
    if audioExts[ext] then return l10n.tr("lua_widget.stacks.audio") end
    if archiveExts[ext] then return l10n.tr("lua_widget.stacks.archives") end
    return l10n.tr("lua_widget.stacks.other")
end

local function categorizeByDate(item)
    local t = sys.getTime()
    local today = os.time({ year=t.year, month=t.month, day=t.day, hour=0, min=0, sec=0 })
    local yesterday = today - 86400
    local weekAgo = today - 7 * 86400
    local monthAgo = today - 30 * 86400
    local itemTime = item.modifiedTime or item.createdTime or 0
    if itemTime >= today then return l10n.tr("lua_widget.stacks.today") end
    if itemTime >= yesterday then return l10n.tr("lua_widget.stacks.yesterday") end
    if itemTime >= weekAgo then return l10n.tr("lua_widget.stacks.this_week") end
    if itemTime >= monthAgo then return l10n.tr("lua_widget.stacks.this_month") end
    return l10n.tr("lua_widget.stacks.older")
end

local function categorizeByExt(item)
    local ext = getFileExt(item.name or "")
    if ext == "" then return l10n.tr("lua_widget.stacks.other") end
    return string.upper(ext)
end

local function categorize(item, mode)
    if mode == 1 then return categorizeByDate(item) end
    if mode == 2 then return categorizeByExt(item) end
    return categorizeByType(item)
end

-- ── 图标映射 ─────────────────────────────────────────────────────

local categoryIcons = {
    [l10n.tr("lua_widget.stacks.images")]    = "🖼️",
    [l10n.tr("lua_widget.stacks.documents")] = "📄",
    [l10n.tr("lua_widget.stacks.videos")]    = "🎬",
    [l10n.tr("lua_widget.stacks.audio")]     = "🎵",
    [l10n.tr("lua_widget.stacks.archives")]  = "📦",
    [l10n.tr("lua_widget.stacks.shortcuts")] = "🔗",
    [l10n.tr("lua_widget.stacks.other")]     = "📁",
    [l10n.tr("lua_widget.stacks.today")]     = "📅",
    [l10n.tr("lua_widget.stacks.yesterday")] = "📅",
    [l10n.tr("lua_widget.stacks.this_week")] = "📅",
    [l10n.tr("lua_widget.stacks.this_month")]= "📅",
    [l10n.tr("lua_widget.stacks.older")]     = "📅",
}

local function getCategoryIcon(cat)
    return categoryIcons[cat] or "📁"
end

-- ── 分组逻辑 ─────────────────────────────────────────────────────

local function groupItems(items, mode)
    local groups = {}
    local order = {}
    for _, item in ipairs(items) do
        if not item.isFolder then
            local cat = categorize(item, mode)
            if not groups[cat] then
                groups[cat] = {}
                table.insert(order, cat)
            end
            table.insert(groups[cat], item)
        end
    end
    -- 按项目数量降序排列
    table.sort(order, function(a, b)
        return #groups[a] > #groups[b]
    end)
    return groups, order
end

-- ── 配置加载 ─────────────────────────────────────────────────────

local function loadConfig()
    groupBy = tonumber(storage.get("groupBy")) or 0
    local theme = widget.theme()
    if theme then
        if theme.contentTheme == 1 then
            -- 浅色主题
            bg = 0xF5F5F7
            border = 0xE0E0E0
            alpha = 0.92
        else
            -- 深色主题（Apple HIG kDarkTheme）
            bg = 0x1A2230
            border = 0x38383A
            alpha = 0.88
        end
    end
end

-- ── 渲染 ─────────────────────────────────────────────────────────

function render()
    loadConfig()
    local w = layout.width()
    local h = layout.height()
    local pad = layout.cu(12)
    local itemH = layout.cu(36)
    local iconSz = layout.cu(16)
    local countSz = layout.cu(12)

    -- 读取桌面文件
    local allItems = desktop.items()
    if not allItems or #allItems == 0 then
        local msg = l10n.tr("lua_widget.stacks.empty")
        local sz = layout.fontCu(14)
        local m = draw.measureText(msg, sz, 0, true)
        draw.text((w - m.width) * 0.5, (h - m.height) * 0.5, msg, sz, 0x999999, w, true)
        return
    end

    local showCount = storage.get("showCount") ~= "0"
    local groups, order = groupItems(allItems, groupBy)

    -- 渲染堆叠
    local y = pad
    local contentH = h - pad * 2
    local maxVisible = math.floor(contentH / (itemH + layout.cu(4)))

    for i = 1, math.min(#order, maxVisible) do
        local cat = order[i]
        local items = groups[cat]
        local isExpanded = expandedStacks[cat] or false
        local icon = getCategoryIcon(cat)
        local countText = showCount and (" (" .. #items .. ")") or ""

        -- 堆叠头部（可点击展开/收起）
        local headerH = itemH
        local headerY = y

        -- 背景
        if isExpanded then
            draw.fillRect(pad, headerY, w - pad * 2, headerH, 0x2A3444, 0.6)
        else
            draw.fillRect(pad, headerY, w - pad * 2, headerH, 0x222A36, 0.4)
        end

        -- 图标 + 类别名 + 数量
        draw.text(pad + layout.cu(8), headerY + (headerH - iconSz) * 0.5, icon, iconSz, 0xFFFFFF, iconSz, true)
        draw.text(pad + layout.cu(28), headerY + (headerH - layout.fontCu(14)) * 0.5,
            cat .. countText, layout.fontCu(14), 0xCCCCCC, w - pad * 4, true)

        -- 展开/收起箭头
        local arrowX = w - pad - layout.cu(16)
        local arrowY = headerY + headerH * 0.5
        local arrow = isExpanded and "▼" or "▶"
        draw.text(arrowX, arrowY - layout.cu(6), arrow, layout.fontCu(10), 0x888888, layout.cu(16), true)

        y = y + headerH + layout.cu(2)

        -- 展开时显示文件列表
        if isExpanded then
            local maxFiles = math.min(#items, 8) -- 最多显示 8 个文件
            for j = 1, maxFiles do
                local file = items[j]
                local fileName = file.name or "unknown"
                local fileIcon = "  " -- 简化：用空格占位，实际可用 draw.icon

                draw.fillRect(pad + layout.cu(4), y, w - pad * 2 - layout.cu(8), itemH - layout.cu(2),
                    0x1E2736, 0.3)
                draw.text(pad + layout.cu(12), y + (itemH - layout.fontCu(12)) * 0.5,
                    fileIcon .. fileName, layout.fontCu(12), 0xBBBBBB, w - pad * 4, true)

                y = y + itemH
            end
            if #items > maxFiles then
                local moreText = "  +" .. (#items - maxFiles) .. " more"
                draw.text(pad + layout.cu(12), y + layout.cu(4),
                    moreText, layout.fontCu(11), 0x888888, w - pad * 4, true)
                y = y + layout.cu(24)
            end
            y = y + layout.cu(4)
        end
    end

    -- 空状态提示
    if #order == 0 then
        local msg = l10n.tr("lua_widget.stacks.empty")
        local sz = layout.fontCu(14)
        local m = draw.measureText(msg, sz, 0, true)
        draw.text((w - m.width) * 0.5, (h - m.height) * 0.5, msg, sz, 0x999999, w, true)
    end
end

-- ── 交互 ─────────────────────────────────────────────────────────

function onClick(x, y, button, delta)
    if button ~= 1 then return end -- 仅左键
    local w = layout.width()
    local pad = layout.cu(12)
    local itemH = layout.cu(36)

    loadConfig()
    local allItems = desktop.items()
    if not allItems or #allItems == 0 then return end

    local groups, order = groupItems(allItems, groupBy)
    local cy = pad
    local contentH = layout.height() - pad * 2
    local maxVisible = math.floor(contentH / (itemH + layout.cu(4)))

    for i = 1, math.min(#order, maxVisible) do
        local cat = order[i]
        local items = groups[cat]
        local isExpanded = expandedStacks[cat] or false
        local headerH = itemH

        -- 检查是否点击了堆叠头部
        if y >= cy and y <= cy + headerH then
            expandedStacks[cat] = not isExpanded
            return
        end

        cy = cy + headerH + layout.cu(2)

        -- 如果展开，检查是否点击了文件
        if isExpanded then
            local maxFiles = math.min(#items, 8)
            for j = 1, maxFiles do
                if y >= cy and y <= cy + itemH then
                    -- 点击文件：打开
                    if items[j].path then
                        desktop.open(items[j].path)
                    end
                    return
                end
                cy = cy + itemH
            end
            if #items > maxFiles then
                cy = cy + layout.cu(24)
            end
            cy = cy + layout.cu(4)
        end
    end
end

-- ── 右键菜单 ─────────────────────────────────────────────────────

function getContextMenu()
    return {
        { id = 1, label = l10n.tr("lua_widget.stacks.group_type"), icon = utf8.char(0xE8B2), iconFont = "fluent" },
        { id = 2, label = l10n.tr("lua_widget.stacks.group_date"), icon = utf8.char(0xE787), iconFont = "fluent" },
        { id = 3, label = l10n.tr("lua_widget.stacks.group_ext"), icon = utf8.char(0xE8A5), iconFont = "fluent" },
    }
end

function onMenu(id)
    if id == 1 then
        storage.set("groupBy", "0")
        groupBy = 0
    elseif id == 2 then
        storage.set("groupBy", "1")
        groupBy = 1
    elseif id == 3 then
        storage.set("groupBy", "2")
        groupBy = 2
    end
    expandedStacks = {}
end

-- ── 桌面变化回调 ─────────────────────────────────────────────────

function onDesktopChanged(reason)
    expandedStacks = {}
end
