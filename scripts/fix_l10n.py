import json, glob, os

translations = {
    "zh-CN": {"app.settings.hide_desktop_icons": "隐藏桌面图标（不推荐）", "app.settings.mica_enabled": "Mica 毛玻璃材质"},
    "zh-TW": {"app.settings.hide_desktop_icons": "隱藏桌面圖示（不推薦）", "app.settings.mica_enabled": "Mica 毛玻璃材質"},
    "en-US": {"app.settings.hide_desktop_icons": "Hide desktop icons (not recommended)", "app.settings.mica_enabled": "Mica frosted glass material"},
    "ja-JP": {"app.settings.hide_desktop_icons": "デスクトップアイコンを非表示（非推奨）", "app.settings.mica_enabled": "Mica すりガラス素材"},
    "ko-KR": {"app.settings.hide_desktop_icons": "바탕화면 아이콘 숨기기 (권장하지 않음)", "app.settings.mica_enabled": "Mica 프로스트 글래스"},
    "de-DE": {"app.settings.hide_desktop_icons": "Desktop-Symbole verbergen (nicht empfohlen)", "app.settings.mica_enabled": "Mica matter Glas-Effekt"},
    "fr-FR": {"app.settings.hide_desktop_icons": "Masquer les icones du bureau (non recommande)", "app.settings.mica_enabled": "Effet verre depoli Mica"},
    "es-ES": {"app.settings.hide_desktop_icons": "Ocultar iconos del escritorio (no recomendado)", "app.settings.mica_enabled": "Efecto cristal esmerilado Mica"},
    "es-419": {"app.settings.hide_desktop_icons": "Ocultar iconos del escritorio (no recomendado)", "app.settings.mica_enabled": "Efecto cristal esmerilado Mica"},
    "pt-BR": {"app.settings.hide_desktop_icons": "Ocultar icones da area de trabalho (nao recomendado)", "app.settings.mica_enabled": "Efeito vidro fosco Mica"},
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
