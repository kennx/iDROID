# AGENTS.md

**项目**: iDROID — [M5Stack Cardputer ADV](./docs/spec/Cardputer_ADV.md) + [Cap LoRa-1262](./docs/spec/Cap_LoRa-1262.md) + [ENV-III](./docs/spec/ENV-III.md) 固件开发（PlatformIO）


**核心信条**：先验证，再动手。不确定时停下来，用工具查清楚。

> 这些准则偏向谨慎而非速度。简单任务可酌情处理。

设备信息:

```
SoC:ESP32-S3FN8 @ Dual-core Xtensa LX7, up to 240MHz
Flash:8MB
Display:ST7789V2@1.14", 240 x 135px
```

- [M5Stack Cardputer ADV](./docs/spec/Cardputer_ADV.md)
- [Cap LoRa-1262](./docs/spec/Cap_LoRa-1262.md)
- [ENV-III](./docs/spec/ENV-III.md) 

---

## 1. 编码前思考（Think Before Coding）

**不要假设。不要隐藏困惑。暴露权衡。**

动手之前：
- **陈述你的假设**。不确定时，**直接发问**。
- 存在多种解释时，**逐一列出**，不要暗自选定。
- 有更简单的方案时，**直接指出**。该反对时就反对。
- 不清楚的地方，**停下来**，指出困惑点，**发问**。
- **用中文回复**，直接、简洁、不绕弯。

**当遇到不确定的 API / 硬件行为时，不要瞎猜。先查文档。**

---

## 2. 简洁至上（Simplicity First）

**最小代码量解决问题。不做臆测。**

- 不添加超出需求范围的功能。
- 不为一次性代码创建抽象。
- 不添加未被要求的 `flexibility` 或 `configurability`。
- 不为不可能发生的场景编写 `error handling`。
- 如果写了 200 行只需 50 行，**重写**。

**反问自己**：资深工程师会觉得这过于复杂吗？如果是，简化。

---

## 3. 精准改动（Surgical Changes）

**只碰必须改的部分。只清理自己造成的混乱。**

编辑现有代码时：
- 不要顺手「优化」相邻的代码、注释或格式。
- 不要对没问题的代码进行 refactor。
- **遵循**现有风格，即使你会用不同方式写。
- 发现无关的 dead code，**提一句**，不要删除。

改动产生废弃项时：
- 删除因你的改动而变得未使用的 `imports`、`variables` 和 `functions`。
- 不要删除预先存在的 `dead code`，除非被要求。

**检验标准**：每一行改动都应能直接追溯到用户的请求。

---

## 4. 目标驱动执行（Goal-Driven Execution）

**定义成功标准。循环验证直至通过。**

把任务转化为可验证的目标：
- "Add validation" → "Write `tests` for invalid inputs, then make them pass"
- "Fix the bug" → "Write a `test` that reproduces it, then make it pass"
- "Refactor X" → "Ensure `tests` pass before and after"

多步骤任务列出计划：
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

强成功标准让你能独立迭代。弱标准（"让它能跑"）只会让人不断追问。

---

## 5. 遇到问题时 → 先查文档，再动手（Tools First）

**这是 card7 的核心原则。遇到任何不确定，先用工具验证。**

### 5.1 优先使用的工具

| 场景 | 工具 | 用途 |
|------|------|------|
| M5Stack API / 函数签名 / 参数 | **context7** MCP | 查询 M5Unified、M5GFX、M5Cardputer 官方文档 |
| 硬件规格 / 引脚定义 / 接线 | **context7** MCP | 查官方产品文档 |
| 通用技术问题 / 最佳实践 | **tavily** / **searxng** MCP | 搜索社区方案、GitHub Issues |
| 代码示例 / 开源项目参考 | **web_search** / **web_fetch** | 查找可运行的示例 |
| 库兼容性 / 已知 Bug | **context7** + **web_search** | 文档 + 社区验证 |

### 5.2 查询顺序

```
遇到不确定
    ↓
能否用 context7 查官方文档？
    ↓ 是 → 查文档，获取准确信息
    ↓ 否 / 文档不清
能否用 tavily/searxng 搜索社区方案？
    ↓ 是 → 搜索并交叉验证 2-3 个来源
    ↓ 否 / 搜索结果矛盾
停下来，向用户说明困惑点，请求澄清
```

### 5.3 不编造规则

- **不编造 API 参数**：如果不确定 `M5.Power.getBatteryVoltage()` 的返回值类型，用 **context7** 查 M5Unified 文档。
- **不编造引脚定义**：如果不确定 EXT 2.54-14P 的 `SCK` 对应哪个 GPIO，用 **context7** 查 Cardputer-Adv PinMap。
- **不编造硬件限制**：如果不确定 `ESP32-S3` 的 BLE 是否支持 Classic Bluetooth，用 **searxng** 搜索官方 specs。
- **不编造库版本**：如果不确定 `RadioLib` 是否支持 SX1262 的特定功能，用 **context7** 查库文档或 GitHub release notes。

### 5.4 有主见，不自负

- **有主见**：看到不合理的方案（比如用 `delay()` 做精确时序），**直接提出异议**，说明原因，给出替代方案。
- **不自负**：面对没做过的功能（比如第一次写 `LoRa` 通信），**不要认为自己一定能做**。先查文档、找示例、验证可行性，再动手。
- **对模糊问题保持警惕**：用户说 "帮我加个 GPS 功能" 时，先问清楚：是只读坐标？还是要记录轨迹？要不要离线地图？要不要导出 GPX？

---

## 6. 验证准则是否生效

如果这些准则在起作用，你会看到：
- `diff` 中的不必要变更更少
- 因过度复杂化而导致的重写更少
- 澄清性问题出现在实现之前，而非错误之后
- **遇到不确定时，先看到工具查询，再看到代码**

---

## 7. 项目特定规则

- 使用 **PlatformIO** 构建系统（`platformio.ini`）
- 框架：**Arduino**（ESP32-S3）
- 核心库：**M5Unified** + **M5GFX** + **M5Cardputer**
- 代码风格：遵循现有项目的风格，如果不确定，问用户或查 `.clang-format`
- 所有新功能必须有可验证的 **test** 或 **demo**

---

## 8. git commit 规范

- 使用中文
- 标题行使用 conventional commits 格式（feat/fix/refactor/chore 等）
- body 中按文件或功能分组，说明改了什么、为什么改、影响范围
- 修复 bug 需说明根因；架构决策需简要说明理由