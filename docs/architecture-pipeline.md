# 管道化架构方案（Pipeline Architecture Plan）

> 目标：层与层之间、功能与功能之间用**管道（pipeline）连接**，单向数据流，
> 禁止把不同职责糅合进同一个处理路径。本文基于全球调研结论与现有源码分析，
> 定义改造后的目标架构与落地顺序。

---

## 1. 调研依据（全球最佳实践）

| 来源 | 核心思想 | 本项目借鉴 |
|------|---------|-----------|
| **Vulkan Rendergraph**（官方教程） | 渲染 = 对资源的一系列变换；pass 是节点、依赖是边（DAG），声明式注册 → 依赖分析 → 执行 | 特效层用"效果链"表达：捕获 → 模糊 → 饱和 → 着色 → 噪声 |
| **UNIX 哲学**（Raymond《TAOUP》） | 模块化（简单部件 + 干净接口）、组合（可连接）、分离（策略与机制、接口与引擎） | 每个 `.cpp` 只负责一个管道节点，接口即函数签名 |
| **Flutter 架构**（官方文档） | embedder → framework → render 单向分层，事件流单向传递 | 消息 → 分发 → 领域 → 特效 → 渲染 → 呈现，单向流动 |
| **DWMBlurGlass / 10Mica** | Mica 管线 = 壁纸采样 → 高斯模糊(140) → 亮度混合 → 着色 → 噪点 | 毛玻璃特效管道按此链式搭建 |

---

## 2. 现有架构盘点（改造前）

现状是**单巨类 + 按文件切分成员函数**：`DesktopApp`（app/app.h）承载全部功能，
每个 `app_*.cpp` 是它的一组方法。层间依赖隐式、靠成员变量共享状态。

```
Windows 消息 ──► DesktopApp::HandleMessage (switch 巨型分发)
                      │
                      ├──► 各 app_xxx.cpp 成员函数（互相可随意调用）
                      │
                      └──► OnPaint ─► RenderFrame ─► Draw* 原语（app_render_primitives）
```

问题：功能之间没有显式管道边界，任何函数都能调用任何函数；新增功能（如毛玻璃）
只能再往巨类里塞方法。

## 3. 目标架构：七层管道（改造后）

```
┌─────────────────────────────────────────────────────────────┐
│  L1 输入层 Input         WM_* / 指针 / 键盘 / OLE / 定时器      │
│      │                                                        │
│      ▼                                                        │
│  L2 分发层 Dispatch      事件路由（现有 HandleMessage 保留为    │
│      │                  薄路由，不再内联业务逻辑）               │
│      ▼                                                        │
│  L3 领域层 Domain        core/: container / dock / item / slot │
│      │                  / widget（纯模型，无渲染、无窗口）        │
│      ▼                                                        │
│  L4 特效管线 Effect      效果链：Capture→Blur→Tint→Noise→Compose│
│      │                  （毛玻璃/Mica/Acrylic 专用管道）          │
│      ▼                                                        │
│  L5 渲染层 Render        RenderFrame 场景组装：静态背景→动态覆盖  │
│      │                  → 特效挂载点                              │
│      ▼                                                        │
│  L6 呈现层 Present       DirectComposition 表面 + D2D 提交       │
│      │                                                        │
│      ▼                                                        │
│  L7 交互反馈 Feedback    UI 动画 / 快捷导航 / 弹出层回写          │
└─────────────────────────────────────────────────────────────┘
```

### 3.1 管道连接规则（硬约束）

1. **单向流动**：数据只从高编号层流向低编号层（L1→L7）；L7 的反馈通过"事件回投"（重新进入 L2 分发），不反向直调。
2. **接口即管道**：层与层之间只通过显式函数签名通信，禁止跨层访问成员变量。
3. **功能即节点**：每个功能（毛玻璃、Dock、快捷导航）实现为一个节点，输入/输出为纯数据结构；节点之间不互相调用，只通过管道串联。
4. **策略与机制分离**：`*_rules.h`（策略常量/规则函数）与 `*_render.cpp`（机制实现）分离——现有代码已部分符合，改造后保持。
5. **禁止功能糅合**：一个新功能必须落在其所属层的独立 `.cpp`，不得把逻辑塞进 `HandleMessage` 的 switch 或既有成员函数。

## 4. 毛玻璃特效管道（本轮实施的设计）

参考 DWMBlurGlass/10Mica 的链式结构，设计为**可组合效果链**：

```
EffectPipeline: 
  [1] Capture   壁纸/背景采样（WIC 解码壁纸位图）
  [2] Blur      D2D1 GaussianBlur 效果（radius 按需，mica≈140 单位）
  [3] Tint      着色叠加（亮度混合 ID2D1BlendEffect / 颜色矩阵）
  [4] Noise     亚克力噪点（复用现有 DrawAcrylicNoise 的 64×64 位图刷）
  [5] Compose   合成到目标区域（FillRoundedRectangle / FillRectangle）
```

- 每个阶段是独立函数（`DrawMicaBackdrop` / `DrawAcrylicBackdrop`），输入 `(ctx, frame, theme)` 输出"画到 ctx"。
- 与现有 `app_render_primitives.cpp` 的原语并列，但按管道节点独立成 `app_backdrop_effect.cpp`，不污染巨类。
- 触发点（管道挂载）：在 L5 RenderFrame 的"特效挂载点"调用，不直接改现有绘制函数内部。

## 5. 落地顺序（分步、可逆）

| 步骤 | 内容 | 风险 |
|------|------|------|
| ① | 新建 `app_backdrop_effect.cpp`：Capture+Blur+Tint+Noise+Compose 五节点实现 | 低（纯新增文件） |
| ② | 在 RenderFrame 增加"特效挂载点"调用（条件编译/开关），不破坏现有绘制 | 低 |
| ③ | 将 `HandleMessage` 中内联业务逻辑逐步下沉到对应层（后续轮次） | 中（回归面） |
| ④ | 效果链参数（模糊半径/着色/主题）接入设置（dock_settings 等） | 低 |

> 原则：每一步保持可编译、可回滚。本轮完成 ①②（毛玻璃效果 + 管道挂载），
> ③④ 视构建验证结果推进。

## 6. 与既有代码的兼容边界

- **不动**：core/ 领域模型、OLE 拖放、Lua 桥、窗口生命周期（均已模块化）。
- **只加**：新特效管道文件 + RenderFrame 挂载点 + 设置字段。
- **保留**：`DrawAcrylicNoise`（作为 Effect 管道的 Noise 节点直接复用）。
