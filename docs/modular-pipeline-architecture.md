# 模块化管道架构规范（Modular Pipeline Architecture Spec）

> 本文档定义桌面整理项目（Desktop Organizer）的模块化设计原则。
> 所有新功能必须遵循本规范，确保整体是**积木搭建结构**，而非过度耦合。

---

## 1. 核心原则

### 1.1 单向数据流
```
输入层(L1) → 分发层(L2) → 领域层(L3) → 特效层(L4) → 渲染层(L5) → 呈现层(L6) → 反馈层(L7)
```
- 数据只从高编号层流向低编号层
- 反馈通过"事件回投"（重新进入 L2 分发），不反向直调
- **违反案例**：渲染代码直接修改领域模型 → 错误；应通过事件/命令

### 1.2 接口即管道
- 层与层之间只通过显式函数签名通信
- 禁止跨层访问成员变量（`this->xxx_` 直接读写）
- 每个函数的输入/输出是明确的数据结构，不是隐式状态

### 1.3 功能即节点
- 每个功能模块是独立节点，有自己的 `.h`/`.cpp`
- 节点之间不互相 `#include`（依赖方向：下游依赖上游，不反向）
- 节点可独立编译、测试、替换

---

## 2. 模块清单与职责

| 模块 | 文件 | 层级 | 职责 | 输入 | 输出 |
|------|------|------|------|------|------|
| **设计令牌** | `design_tokens.h` | L3 基础 | 颜色/间距/圆角/阴影/运动/材质/排版/可访问性令牌 | 系统设置（注册表） | 令牌常量 + 适配函数 |
| **毛玻璃管道** | `app_backdrop_effect.cpp` | L4 特效 | Capture→Blur→Tint→Noise→Sheen→Refract→Compose 六节点；`DrawLiquidGlassSurface()` 统一入口 | 帧矩形 + 主题设置 | D2D 绘制命令 |
| **Liquid Glass 统一入口** | `app_backdrop_effect.cpp` | L4 特效 | 便捷封装：一行调用获得完整 Liquid Glass 材质（Dock/Widget/设置页面通用） | 帧矩形 + 圆角 + 是否选中 | 完整六节点管道 |
| **单光源** | `app_glass_render.cpp` | L5 渲染 | 玻璃边框渐变（35° 左上光源 + 三档亮度） | 颜色 + 光强令牌 | 径向渐变绘制 |
| **剪贴板历史** | `widgets/clipboard-history/` | L3 基础 | macOS Tahoe 风格剪贴板历史（8小时保留、快速粘贴/复制/删除） | 用户交互 + 剪贴板数据 | 历史列表 UI |
| **Stacks 堆叠** | `widgets/stacks/` | L3 基础 | macOS 风格文件自动分组（类型/日期/扩展名） | 桌面文件列表 | 分组堆叠 UI |
| **文件夹标签** | `widgets/folder-tags/` | L3 基础 | macOS 风格文件夹颜色/符号标签 | 用户选择 | 标签应用 |
| **智能文件夹** | `widgets/smart-folder/` | L3 基础 | macOS 风格保存搜索查询自动更新 | 搜索关键词 | 实时结果 |
| **圆角系统** | `design_tokens.h` | L3 基础 | Fixed/Capsule/Concentric 三种圆角计算 | 元素尺寸/层级 | 圆角半径值 |
| **动态排版** | `design_tokens.h` | L3 基础 | SF Pro→Inter→Segoe UI Variable 回退 + 字号缩放 | 系统 TextScaleFactor | 字体名 + 缩放字号 |
| **深色主题** | `design_tokens.h` | L3 基础 | kLightTheme/kDarkTheme 切换 + 系统跟随 | 注册表 AppsUseLightTheme | 色彩令牌 |
| **可访问性** | `design_tokens.h` | L3 基础 | 减少透明度/增强对比/减少动画检测 | SPI/注册表 | 布尔开关 + 适配值 |
| **弹簧动画** | `design_tokens.h` | L3 基础 | 物理弹簧计算（mass/stiffness/damping） | 时间 t | 归一化位置 [0,1] |
| **焦点环** | `design_tokens.h` | L3 基础 | 2px solid primary-focus + 键盘导航判断 | 焦点状态 | 环矩形 + 颜色 |
| **设置页面** | `settings_window.cpp` | L5 渲染 | ImGui 多页面设置界面 | 用户输入 | 配置持久化 |
| **Dock 渲染** | `app_dock_render.cpp` | L5 渲染 | Dock 控件/条目/指示器绘制 | 设计令牌 + 状态 | D2D 绘制命令 |
| **Spotlight** | `app_quick_navigation_render.cpp` | L5 渲染 | 搜索面板渲染 | 搜索结果 + 主题 | D2D 绘制命令 |
| **菜单栏** | `app_menu_bar.cpp` | L5 渲染 | 系统状态栏（时钟/电池/WiFi/音量） | 系统 API | GDI 绘制命令 |

---

## 3. 管道连接规范

### 3.1 新功能接入流程
```
1. 确定功能属于哪个层（L1-L7）
2. 在该层创建独立 .h/.cpp
3. 定义输入/输出数据结构（纯 POD，不依赖 GUI 框架）
4. 实现节点逻辑（纯函数优先，状态通过参数传递）
5. 在上游层的调度点挂载节点
6. 添加单元测试验证输入→输出
```

### 3.2 依赖方向图
```
design_tokens.h (L3 基础，无依赖)
    ↑
app_backdrop_effect.cpp (L4，依赖 L3)
    ↑
app_glass_render.cpp (L5，依赖 L3)
app_dock_render.cpp (L5，依赖 L3)
app_quick_navigation_render.cpp (L5，依赖 L3)
app_menu_bar.cpp (L5，依赖 L3)
    ↑
settings_window.cpp (L5，依赖 L3 + 配置)
    ↑
app_lifecycle.cpp (L1/L2，依赖所有 L3-L5)
```

### 3.3 禁止的模式
```cpp
// ❌ 错误：渲染代码直接修改领域模型
void DesktopApp::DrawSomeWidget(...) {
    items_[0].selected = true;  // 渲染层直接写领域层
}

// ✅ 正确：通过事件/命令回投
void DesktopApp::DrawSomeWidget(...) {
    // 只读 items_，不修改
    // 交互通过 PostMessage/事件回投到 L2 分发
}

// ❌ 错误：节点之间互相依赖
// app_backdrop_effect.cpp 直接 #include app_dock_render.h

// ✅ 正确：节点通过共享的 L3 基础层通信
// 两者都 #include design_tokens.h，通过令牌交换数据
```

---

## 4. 设计令牌使用规范

### 4.1 颜色使用
```cpp
// ❌ 硬编码颜色
D2D1::ColorF(0.39f, 0.66f, 1.0f, 1.0f)

// ✅ 使用设计令牌
using namespace snowdesktop::design_tokens;
const auto& colors = GetColorTokens();
D2D1::ColorF(colors.primary.r, colors.primary.g, colors.primary.b, 1.0f)
```

### 4.2 圆角使用
```cpp
// ❌ 硬编码圆角
DrawD2DRoundedRectangle(ctx, rect, 6.0f, ...)

// ✅ 使用设计令牌
DrawD2DRoundedRectangle(ctx, rect, kRadius.sm, ...)  // 8px
```

### 4.3 字号使用
```cpp
// ❌ 硬编码字号
CreateFontW(-14, ...)

// ✅ 使用设计令牌 + Dynamic Type
const float size = GetScaledFontSize(typo.navLink.fontSize);
CreateFontW(-static_cast<int>(size), ...)
```

### 4.4 动画使用
```cpp
// ❌ 硬编码缓动
float t = progress * progress * (3.0f - 2.0f * progress);

// ✅ 使用弹簧物理
double position = AppleSpringAnimation(timeSeconds);
```

---

## 5. 新增组件检查清单

新增任何功能前，确认以下项目：

- [ ] 功能属于哪个层？（L1-L7）
- [ ] 是否已存在类似功能？（复用优先）
- [ ] 输入/输出数据结构是否纯 POD？
- [ ] 是否只通过设计令牌访问颜色/间距/圆角？
- [ ] 是否遵循单向数据流？（不反向修改上游层）
- [ ] 是否有独立 .h/.cpp 文件？
- [ ] 是否能在不破坏其他模块的情况下独立替换？
- [ ] 是否通过 CI 构建验证？

---

## 6. 已知的技术债

| 问题 | 影响 | 计划修复 |
|------|------|---------|
| `DesktopApp` 仍是巨型类（3000+ 行） | 新功能只能往巨类里加方法 | 后续逐步拆分到独立模块 |
| 设置页面硬编码颜色 | 与设计令牌不一致 | 后续用 design_tokens 替换所有硬编码 |
| 部分渲染代码直接写领域模型 | 违反单向数据流 | 后续通过事件/命令回投 |
| 测试覆盖不足 | 回归风险 | 后续补充关键路径测试 |

---

## 7. 参考资料

- **Apple HIG**：设计令牌、Liquid Glass、排版系统、可访问性
- **DESIGN.md**（awesome-design-md）：Apple 官方设计规范
- **Vulkan Rendergraph**：DAG 渲染管线模式
- **UNIX 哲学**：模块化/组合/策略机制分离
- **Flutter 架构**：embedder→framework→render 单向分层
