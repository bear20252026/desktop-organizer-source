import json, glob, os

translations = {
    "zh-CN": {
        "app.settings.desktop_organize": "桌面整理",
        "app.settings.desktop_organize_desc": "扫描桌面文件，按类型自动分类并移动到对应文件夹。真正移动文件，不是覆盖假界面。",
        "app.settings.desktop_organize_preview": "预览整理效果",
        "app.settings.organize_preview": "整理预览",
        "app.settings.desktop_organize_execute": "开始整理",
        "app.settings.desktop_organize_cancel": "取消",
        "app.settings.desktop_organize_undo": "撤销上次整理",
        "app.settings.organize_result": "整理完成：已移动 {0} 个文件到 {1} 个分类文件夹",
        "app.settings.mica_enabled": "Mica 毛玻璃材质",
        "app.settings.hide_desktop_icons": "隐藏桌面图标（不推荐）",
    },
    "zh-TW": {
        "app.settings.desktop_organize": "桌面整理",
        "app.settings.desktop_organize_desc": "掃描桌面檔案，按類型自動分類並移動到對應資料夾。真正移動檔案，不是覆蓋假介面。",
        "app.settings.desktop_organize_preview": "預覽整理效果",
        "app.settings.organize_preview": "整理預覽",
        "app.settings.desktop_organize_execute": "開始整理",
        "app.settings.desktop_organize_cancel": "取消",
        "app.settings.desktop_organize_undo": "撤銷上次整理",
        "app.settings.organize_result": "整理完成：已移動 {0} 個檔案到 {1} 個分類資料夾",
        "app.settings.mica_enabled": "Mica 毛玻璃材質",
        "app.settings.hide_desktop_icons": "隱藏桌面圖示（不推薦）",
    },
    "en-US": {
        "app.settings.desktop_organize": "Desktop Organize",
        "app.settings.desktop_organize_desc": "Scan desktop files, auto-classify by type and move to category folders. Actually moves files, not a fake overlay.",
        "app.settings.desktop_organize_preview": "Preview Organization",
        "app.settings.organize_preview": "Organization Preview",
        "app.settings.desktop_organize_execute": "Start Organizing",
        "app.settings.desktop_organize_cancel": "Cancel",
        "app.settings.desktop_organize_undo": "Undo Last Organization",
        "app.settings.organize_result": "Organization complete: moved {0} files to {1} category folders",
        "app.settings.mica_enabled": "Mica frosted glass material",
        "app.settings.hide_desktop_icons": "Hide desktop icons (not recommended)",
    },
    "ja-JP": {
        "app.settings.desktop_organize": "デスクトップ整理",
        "app.settings.desktop_organize_desc": "デスクトップファイルをスキャンし、タイプ別に自動分類してフォルダに移動。実際にファイルを移動します。",
        "app.settings.desktop_organize_preview": "整理プレビュー",
        "app.settings.organize_preview": "整理プレビュー",
        "app.settings.desktop_organize_execute": "整理開始",
        "app.settings.desktop_organize_cancel": "キャンセル",
        "app.settings.desktop_organize_undo": "前回の整理を元に戻す",
        "app.settings.organize_result": "整理完了：{0}個のファイルを{1}個のカテゴリフォルダに移動",
        "app.settings.mica_enabled": "Mica すりガラス素材",
        "app.settings.hide_desktop_icons": "デスクトップアイコンを非表示（非推奨）",
    },
    "ko-KR": {
        "app.settings.desktop_organize": "데스크톱 정리",
        "app.settings.desktop_organize_desc": "데스크톱 파일을 스캔하고 유형별로 자동 분류하여 폴더로 이동. 실제로 파일을 이동합니다.",
        "app.settings.desktop_organize_preview": "정리 미리보기",
        "app.settings.organize_preview": "정리 미리보기",
        "app.settings.desktop_organize_execute": "정리 시작",
        "app.settings.desktop_organize_cancel": "취소",
        "app.settings.desktop_organize_undo": "이전 정리 실행 취소",
        "app.settings.organize_result": "정리 완료: {0}개 파일을 {1}개 카테고리 폴더로 이동",
        "app.settings.mica_enabled": "Mica 프로스트 글래스",
        "app.settings.hide_desktop_icons": "바탕화면 아이콘 숨기기 (권장하지 않음)",
    },
    "de-DE": {
        "app.settings.desktop_organize": "Desktop organisieren",
        "app.settings.desktop_organize_desc": "Desktop-Dateien scannen, automatisch nach Typ klassifizieren und in Ordner verschieben.",
        "app.settings.desktop_organize_preview": "Vorschau der Organisation",
        "app.settings.organize_preview": "Organisationsvorschau",
        "app.settings.desktop_organize_execute": "Organisation starten",
        "app.settings.desktop_organize_cancel": "Abbrechen",
        "app.settings.desktop_organize_undo": "Letzte Organisation ruckgangig machen",
        "app.settings.organize_result": "Organisation abgeschlossen: {0} Dateien in {1} Kategorieordner verschoben",
        "app.settings.mica_enabled": "Mica matter Glas-Effekt",
        "app.settings.hide_desktop_icons": "Desktop-Symbole verbergen (nicht empfohlen)",
    },
    "fr-FR": {
        "app.settings.desktop_organize": "Organiser le bureau",
        "app.settings.desktop_organize_desc": "Scanner les fichiers du bureau, classer par type et deplacer dans des dossiers. Deplace reellement les fichiers.",
        "app.settings.desktop_organize_preview": "Apercu de l'organisation",
        "app.settings.organize_preview": "Apercu de l'organisation",
        "app.settings.desktop_organize_execute": "Demarrer l'organisation",
        "app.settings.desktop_organize_cancel": "Annuler",
        "app.settings.desktop_organize_undo": "Annuler la derniere organisation",
        "app.settings.organize_result": "Organisation terminee : {0} fichiers deplaces dans {1} dossiers de categorie",
        "app.settings.mica_enabled": "Effet verre depoli Mica",
        "app.settings.hide_desktop_icons": "Masquer les icones du bureau (non recommande)",
    },
    "es-ES": {
        "app.settings.desktop_organize": "Organizar escritorio",
        "app.settings.desktop_organize_desc": "Escanear archivos del escritorio, clasificar por tipo y mover a carpetas. Mueve realmente los archivos.",
        "app.settings.desktop_organize_preview": "Vista previa de organizacion",
        "app.settings.organize_preview": "Vista previa de organizacion",
        "app.settings.desktop_organize_execute": "Iniciar organizacion",
        "app.settings.desktop_organize_cancel": "Cancelar",
        "app.settings.desktop_organize_undo": "Deshacer ultima organizacion",
        "app.settings.organize_result": "Organizacion completada: {0} archivos movidos a {1} carpetas de categoria",
        "app.settings.mica_enabled": "Efecto cristal esmerilado Mica",
        "app.settings.hide_desktop_icons": "Ocultar iconos del escritorio (no recomendado)",
    },
    "es-419": {
        "app.settings.desktop_organize": "Organizar escritorio",
        "app.settings.desktop_organize_desc": "Escanear archivos del escritorio, clasificar por tipo y mover a carpetas. Mueve realmente los archivos.",
        "app.settings.desktop_organize_preview": "Vista previa de organizacion",
        "app.settings.organize_preview": "Vista previa de organizacion",
        "app.settings.desktop_organize_execute": "Iniciar organizacion",
        "app.settings.desktop_organize_cancel": "Cancelar",
        "app.settings.desktop_organize_undo": "Deshacer ultima organizacion",
        "app.settings.organize_result": "Organizacion completada: {0} archivos movidos a {1} carpetas de categoria",
        "app.settings.mica_enabled": "Efecto cristal esmerilado Mica",
        "app.settings.hide_desktop_icons": "Ocultar iconos del escritorio (no recomendado)",
    },
    "pt-BR": {
        "app.settings.desktop_organize": "Organizar area de trabalho",
        "app.settings.desktop_organize_desc": "Escanear arquivos, classificar por tipo e mover para pastas. Move realmente os arquivos.",
        "app.settings.desktop_organize_preview": "Pre-visualizar organizacao",
        "app.settings.organize_preview": "Pre-visualizacao da organizacao",
        "app.settings.desktop_organize_execute": "Iniciar organizacao",
        "app.settings.desktop_organize_cancel": "Cancelar",
        "app.settings.desktop_organize_undo": "Desfazer ultima organizacao",
        "app.settings.organize_result": "Organizacao concluida: {0} arquivos movidos para {1} pastas de categoria",
        "app.settings.mica_enabled": "Efeito vidro fosco Mica",
        "app.settings.hide_desktop_icons": "Ocultar icones da area de trabalho (nao recomendado)",
    },
}

for path in glob.glob("lang/*.json"):
    locale = os.path.splitext(os.path.basename(path))[0]
    if locale not in translations:
        continue
    with open(path, "r", encoding="utf-8") as f:
        d = json.load(f)
    for k, v in translations[locale].items():
        d[k] = v
    with open(path, "w", encoding="utf-8") as f:
        json.dump(d, f, indent=2, ensure_ascii=False)
        f.write("\n")
    print(f"Fixed: {path}")
