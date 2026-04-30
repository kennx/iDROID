# iDROID 蓝牙键盘页设计（方案 1）

## 背景

当前固件已具备导航页与 GNSS 页切换能力。新增需求是在导航中加入 `BT Keyboard`，使 Cardputer-ADV 可作为手机/电脑外接键盘输入设备。

## 目标

- 导航页新增 `BT Keyboard` 菜单项。
- 进入 `BT Keyboard` 页面后自动开始 BLE 键盘广播。
- 连接成功后可输入英文/符号，并支持基础控制键。
- 使用 `BtnGO` 退出到导航页。

## 非目标

- 不实现中文输入法联动。
- 不实现宏键、快捷层、按键重映射配置页面。
- 不实现 PIN 配对交互页面。

## 已确认约束

- 目标设备：手机 + 电脑（iOS/Android/macOS/Windows/Linux）。
- 入口形态：导航菜单 `BT Keyboard`。
- 进入行为：自动广播（无需二次确认）。
- 第一版输入范围：英文/符号 + Enter/Backspace/Tab/Space + 基础修饰键。

## 方案选择

采用方案 1：独立 `BT Keyboard` 页面 + BLE HID 键盘发送。

理由：

- 与当前 Nav/GNSS 状态机兼容，改动聚焦。
- 用户路径直观（进入即可连接）。
- 便于后续扩展（状态显示、设备名配置、配对管理）。

## 页面与状态流

新增页面：`BT Keyboard`

页面内运行状态：

1. `Advertising`：初始化成功后进入，等待连接。
2. `Connected`：连接建立后进入，接收本机键盘并发送 HID 按键。
3. `Disconnected`：连接断开后短暂显示并自动回到 `Advertising`。

退出逻辑：

- 任意状态按 `BtnGO` 返回导航页。
- 返回时停止广播并释放 BLE 相关资源。

## 输入映射策略（第一版）

输入来源：`M5Cardputer.Keyboard.keysState()`。

- `word`：逐字符发送为键盘输入。
- `enter` → Enter
- `del` → Backspace
- `tab` → Tab
- `space` → Space
- `shift/ctrl/alt`：作为基础修饰键参与上报。

触发条件：

- 使用 `isChange() && isPressed()` 作为发送触发门槛，避免长按重复风暴。

## 异常与降级

1. BLE 初始化失败：
   - 页面显示 `BLE init failed`。
   - 保留 `BtnGO` 返回能力。

2. 连接中断：
   - 状态切为 `Disconnected`。
   - 自动恢复 `Advertising` 并可重新连接。

3. 未连接输入：
   - 不发送按键，仅更新状态提示（如 `Not connected`）。

## 依赖与实现边界

- 在现有 PlatformIO 工程内引入 BLE HID 键盘所需依赖。
- 保持 GNSS 逻辑与渲染节拍不改。
- 保持导航交互一致（菜单选择 + Enter 进入）。

## 验收标准

1. 导航页可看到 `BT Keyboard` 菜单项并可进入。
2. 进入后设备可在手机/电脑蓝牙列表中发现并连接。
3. 连接后可输入英文/符号，Enter/Backspace/Tab/Space 可用。
4. `BtnGO` 可稳定返回导航页。
5. 返回导航后 BLE 不再继续广播（避免后台占用）。

## 验证步骤

1. 构建：`pio run -e m5stack-stamps3`
2. 烧录：`pio run -e m5stack-stamps3 -t upload`
3. 实机验证：
   - 导航进入 `BT Keyboard`
   - 手机/电脑搜索并连接
   - 测试输入 `abc123`, `Enter`, `Backspace`, `Tab`, `Space`
   - 断开后检查自动恢复广播
   - 按 `BtnGO` 返回导航并确认停止广播

## 后续演进（非本次）

- 增加设备名设置、配对清除。
- 增加键位映射探针页，校准方向键等特殊键。
- 增加低功耗策略（空闲超时自动停播）。
