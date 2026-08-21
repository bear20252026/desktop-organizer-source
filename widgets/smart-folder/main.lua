-- smart_folder.lua - macOS 风格智能文件夹组件
name = l10n.tr("lua_widget.smart_folder.name")
useCustomStyle = true
followPersonalizationDefault = true
showTitle = true
bottomBarHover = true

-- Apple HIG 风格默认配色
bg = 0x1A2230
border = 0x38383A
alpha = 0.88
gradientEndA = 0.22

settings = {
    fields = {
        { key = "autoRefresh", label = "Auto refresh on desktop change", type = "bool", default = true },
    }
}

-- 状态
local searchQuery = ""
local searchResults = {}
local savedQueries = {}
local selectedResultIdx = 0
local isEditing = false
local editBuffer = ""

-- 文件类型过滤
local typeFilter = "all"
local imageExts = { jpg=1, jpeg=1, png=1, gif=1, bmp=1, webp=1, svg=1, ico=1, tiff=1, avif=1, heic=1 }
local docExts = { pdf=1, doc=1, docx=1, txt=1, rtf=1, odt=1, xls=1, xlsx=1, ppt=1, pptx=1, csv=1, md=1, pages=1, numbers=1, keynote=1 }
local videoExts = { mp4=1, avi=1, mkv=1, mov=1, wmv=1, flv=1, webm=1, m4v=1, mpg=1, mpeg=1 }
local audioExts = { mp3=1, wav=1, flac=1, aac=1, ogg=1, m4a=1, wma=1, opus=1, aiff=1 }
local archiveExts = { zip=1, rar=1, ["7z"]=1, tar=1, gz=1, bz2=1, xz=1, lz=1, dmg=1, iso=1 }

local function getFileExt(name)
    local ext = name:match("%.([^%.]+)$")
    return ext and string.lower(ext) or ""
end

local function matchesTypeFilter(item)
    if typeFilter == "all" then return true end
    local ext = getFileExt(item.name or "")
    if typeFilter == "images" then return imageExts[ext] or false end
    if typeFilter == "documents" then return docExts[ext] or false end
    if typeFilter == "videos" then return videoExts[ext] or false end
    if typeFilter == "audio" then return audioExts[ext] or false end
    if typeFilter == "archives" then return archiveExts[ext] or false end
    return true
end

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

-- ── 搜索执行 ─────────────────────────────────────────────────────

local function executeSearch(query)
    searchResults = {}
    if not query or query == "" then return end
    local allItems = desktop.items()
    if not allItems then return end
    local q = string.lower(query)
    for _, item in ipairs(allItems) do
        local name = string.lower(item.name or "")
        if string.find(name, q, 1, true) and matchesTypeFilter(item) then
            table.insert(searchResults, item)
        end
    end
end

-- ── 保存查询管理 ─────────────────────────────────────────────────

local function loadSavedQueries()
    savedQueries = {}
    local raw = storage.get("savedQueries") or ""
    for line in raw:gmatch("[^\n]+") do
        local q = line:match("^%s*(.-)%s*$")
        if q and q ~= "" then
            table.insert(savedQueries, q)
        end
    end
end

local function saveSavedQueries()
    local parts = {}
    for _, q in ipairs(savedQueries) do
        table.insert(parts, q)
    end
    storage.set("savedQueries", table.concat(parts, "\n"))
end

local function addSavedQuery(query)
    if not query or query == "" then return end
    -- 避免重复
    for _, q in ipairs(savedQueries) do
        if q == query then return end
    end
    table.insert(savedQueries, query)
    saveSavedQueries()
end

local function deleteSavedQuery(idx)
    if idx < 1 or idx > #savedQueries then return end
    table.remove(savedQueries, idx)
    saveSavedQueries()
end

-- ── 渲染 ─────────────────────────────────────────────────────────

function render()
    loadConfig()
    loadSavedQueries()
    local w = layout.width()
    local h = layout.height()
    local pad = layout.cu(12)
    local headerH = layout.cu(28)
    local inputH = layout.cu(32)
    local rowH = layout.cu(28)
    local iconSz = layout.cu(14)

    local y = pad

    -- 标题
    draw.text(pad, y, "🔍  " .. name, layout.fontCu(16), 0xCCCCCC, w - pad * 2, true)
    y = y + headerH + layout.cu(4)

    -- 搜索输入框
    local inputY = y
    draw.fillRoundedRect(pad, inputY, w - pad * 2, inputH, layout.cu(8), 0x2A3444, 0.6)
    local displayText = isEditing and editBuffer or searchQuery
    if displayText == "" then
        draw.text(pad + layout.cu(12), inputY + (inputH - layout.fontCu(13)) * 0.5,
            l10n.tr("lua_widget.smart_folder.search_placeholder"), layout.fontCu(13), 0x777777, w - pad * 4, true)
    else
        draw.text(pad + layout.cu(12), inputY + (inputH - layout.fontCu(13)) * 0.5,
            displayText, layout.fontCu(13), 0xEEEEEE, w - pad * 4, true)
    end
    y = y + inputH + layout.cu(8)

    -- 类型过滤标签
    local filterTypes = {
        { id = "all", label = l10n.tr("lua_widget.smart_folder.all_types") },
        { id = "images", label = l10n.tr("lua_widget.smart_folder.images") },
        { id = "documents", label = l10n.tr("lua_widget.smart_folder.documents") },
        { id = "videos", label = l10n.tr("lua_widget.smart_folder.videos") },
        { id = "audio", label = l10n.tr("lua_widget.smart_folder.audio") },
        { id = "archives", label = l10n.tr("lua_widget.smart_folder.archives") },
    }
    local filterX = pad
    for _, ft in ipairs(filterTypes) do
        local isSelected = (typeFilter == ft.id)
        local labelW = draw.measureText(ft.label, layout.fontCu(11), 0, true).width + layout.cu(12)
        if isSelected then
            draw.fillRoundedRect(filterX, y, labelW, layout.cu(20), layout.cu(10), 0x007AFF, 0.8)
        else
            draw.fillRoundedRect(filterX, y, labelW, layout.cu(20), layout.cu(10), 0x2A3444, 0.4)
        end
        draw.text(filterX + layout.cu(6), y + layout.cu(3),
            ft.label, layout.fontCu(11), isSelected and 0xFFFFFF or 0xAAAAAA, labelW, true)
        filterX = filterX + labelW + layout.cu(4)
    end
    y = y + layout.cu(26)

    -- 搜索结果
    local resultsH = h - y - pad - layout.cu(4)
    local maxVisible = math.floor(resultsH / rowH)

    if #searchResults == 0 and searchQuery ~= "" then
        local msg = l10n.tr("lua_widget.smart_folder.no_results")
        local sz = layout.fontCu(13)
        local m = draw.measureText(msg, sz, 0, true)
        draw.text((w - m.width) * 0.5, y + (resultsH - m.height) * 0.5, msg, sz, 0x777777, w, true)
    elseif #searchResults > 0 then
        -- 结果计数
        local countText = #searchResults .. " " .. l10n.tr("lua_widget.smart_folder.results_count")
        draw.text(pad, y, countText, layout.fontCu(11), 0x888888, w - pad * 2, true)
        y = y + layout.cu(18)

        for i = 1, math.min(#searchResults, maxVisible) do
            local item = searchResults[i]
            local isSelected = (selectedResultIdx == i)

            if isSelected then
                draw.fillRect(pad, y, w - pad * 2, rowH - layout.cu(2), 0x2A3444, 0.6)
            end

            -- 文件名
            draw.text(pad + layout.cu(8), y + (rowH - layout.fontCu(12)) * 0.5,
                item.name or "unknown", layout.fontCu(12), 0xCCCCCC, w - pad * 4, true)

            y = y + rowH
        end
    end

    -- 已保存查询（底部）
    if #savedQueries > 0 then
        local savedY = h - pad - layout.cu(28)
        draw.text(pad, savedY, l10n.tr("lua_widget.smart_folder.saved_queries") .. ":", layout.fontCu(11), 0x888888, w - pad * 2, true)
        local qx = pad + layout.cu(80)
        for i = 1, math.min(#savedQueries, 4) do
            local qw = draw.measureText(savedQueries[i], layout.fontCu(10), 0, true).width + layout.cu(8)
            draw.fillRoundedRect(qx, savedY - layout.cu(2), qw, layout.cu(18), layout.cu(9), 0x2A3444, 0.5)
            draw.text(qx + layout.cu(4), savedY, savedQueries[i], layout.fontCu(10), 0xBBBBBB, qw, true)
            qx = qx + qw + layout.cu(4)
        end
    end
end

-- ── 交互 ─────────────────────────────────────────────────────────

function onClick(x, y, button, delta)
    if button ~= 1 then return end
    loadConfig()
    loadSavedQueries()
    local pad = layout.cu(12)
    local inputH = layout.cu(32)
    local inputY = pad + layout.cu(28) + layout.cu(4)
    local rowH = layout.cu(28)

    -- 点击搜索框：进入编辑模式
    if y >= inputY and y <= inputY + inputH then
        isEditing = true
        editBuffer = searchQuery
        return
    end

    -- 点击类型过滤标签
    local filterY = inputY + inputH + layout.cu(8)
    if y >= filterY and y <= filterY + layout.cu(20) then
        local filterTypes = { "all", "images", "documents", "videos", "audio", "archives" }
        local filterX = pad
        for _, ft in ipairs(filterTypes) do
            local label = ft
            if ft == "all" then label = l10n.tr("lua_widget.smart_folder.all_types")
            elseif ft == "images" then label = l10n.tr("lua_widget.smart_folder.images")
            elseif ft == "documents" then label = l10n.tr("lua_widget.smart_folder.documents")
            elseif ft == "videos" then label = l10n.tr("lua_widget.smart_folder.videos")
            elseif ft == "audio" then label = l10n.tr("lua_widget.smart_folder.audio")
            elseif ft == "archives" then label = l10n.tr("lua_widget.smart_folder.archives")
            end
            local labelW = draw.measureText(label, layout.fontCu(11), 0, true).width + layout.cu(12)
            if x >= filterX and x <= filterX + labelW then
                typeFilter = ft
                if searchQuery ~= "" then executeSearch(searchQuery) end
                return
            end
            filterX = filterX + labelW + layout.cu(4)
        end
    end

    -- 点击搜索结果：选中/打开
    local resultsY = filterY + layout.cu(44)
    for i = 1, #searchResults do
        if y >= resultsY and y <= resultsY + rowH then
            selectedResultIdx = i
            if searchResults[i].path then
                desktop.open(searchResults[i].path)
            end
            return
        end
        resultsY = resultsY + rowH
    end

    -- 点击保存查询
    if searchQuery ~= "" then
        addSavedQuery(searchQuery)
    end
end

-- ── 键盘输入 ─────────────────────────────────────────────────────

function onKeyPress(key)
    if not isEditing then return end
    if key == "Enter" then
        searchQuery = editBuffer
        isEditing = false
        executeSearch(searchQuery)
    elseif key == "Escape" then
        isEditing = false
        editBuffer = ""
    elseif key == "Backspace" then
        editBuffer = editBuffer:sub(1, -2)
    elseif #key == 1 and key:byte() >= 32 then
        editBuffer = editBuffer .. key
    end
end

-- ── 右键菜单 ─────────────────────────────────────────────────────

function getContextMenu()
    local items = {
        { id = 1, label = l10n.tr("lua_widget.smart_folder.save_query"), icon = utf8.char(0xE5C4), iconFont = "fluent" },
    }
    for i, q in ipairs(savedQueries) do
        table.insert(items, { id = 100 + i, label = q, icon = utf8.char(0xE8B7), iconFont = "fluent" })
    end
    return items
end

function onMenu(id)
    if id == 1 then
        if searchQuery ~= "" then
            addSavedQuery(searchQuery)
        end
    elseif id >= 100 then
        local idx = id - 100
        if idx >= 1 and idx <= #savedQueries then
            searchQuery = savedQueries[idx]
            executeSearch(searchQuery)
        end
    end
end

-- ── 桌面变化回调 ─────────────────────────────────────────────────

function onDesktopChanged(reason)
    if searchQuery ~= "" and storage.get("autoRefresh") ~= "0" then
        executeSearch(searchQuery)
    end
end
