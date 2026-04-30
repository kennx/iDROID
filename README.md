# GNSS 实时显示

## 页面与按键操作
- 默认页面：开机进入导航页（Nav）。
- 进入 GNSS：在导航页按 `Enter`。
- 返回导航：在 GNSS 页按 `BtnGO`（代码侧为 `M5.BtnA`）。
- 进入 BT Keyboard：在导航页选中 `BT Keyboard` 后按 `Enter`。
- BT Keyboard 输入：连接后可发送英文/符号，支持 `Enter`、`Backspace`、`Tab`、`Space`。
- BT Keyboard 返回：按 `BtnGO` 返回导航页。
- `BtnRST`：硬复位键，不参与业务交互。

## 输入输出
- 输入：`Serial1` 接收 NMEA 语句（当前解析 `$GxGGA`、`$GxRMC`，按行 `\n` 结束）。
- 输出：Cardputer 屏幕 7 行文本，显示 `FIX/SAT`、`LAT`、`LON`、`ALT`、`COG`、`HDG`、`UTC`。

## 串口与接入约定
- 串口参数：`115200 8N1`。
- 当前代码显式使用 `Serial1.begin(115200, SERIAL_8N1, 15, 13)`：
  - `RX=GPIO15`（接 Cap GPS_TX）
  - `TX=GPIO13`（接 Cap GPS_RX）
- 上述映射来自 Cardputer-Adv/Cap LoRa-1262 官方 PinMap（EXT UART_RX=G13, UART_TX=G15）。

## 状态语义
- `dirty 字段`：与上一帧相比发生有效变化，才会重绘对应行（避免整屏刷新闪烁）。
- `有效更新`：成功解析并通过校验和的 GGA/RMC 语句会刷新 `last_update_ms`。
- `stale`：`2000ms` 内无有效更新时，将 `fix_valid/cog_valid` 置为无效。
- `方向`：当前实现以 GNSS 的 `COG` 作为方向来源；`HDG`（机身朝向）固定显示 `N/A`（无磁力计输入时不做伪估算）。

## 刷新机制
- 主循环每次最多处理 `128` 字节串口数据，避免阻塞。
- 以 `100ms` 周期执行渲染 tick，仅重绘 dirty 字段。
- 若 `2000ms` 未收到有效更新，标记定位/航向为无效状态。

## 运行步骤
1. 连接 Cardputer-Adv 与 Cap LoRa-1262，并确保 GNSS 侧有 NMEA 输出。
2. 执行构建：`pio run -e m5stack-stamps3`
3. 执行烧录：`pio run -e m5stack-stamps3 -t upload`
4. 冷启动后确认默认显示导航页。
5. 在导航页按 `Enter` 进入 GNSS 页。
6. 在 GNSS 页按 `BtnGO` 返回导航页。
7. 在导航页选择 `BT Keyboard` 并按 `Enter` 进入。
8. 手机/电脑蓝牙搜索并连接 `iDROID-KBD`，验证键盘输入。
9. 观察 GNSS 页 7 行字段是否随定位状态更新。

## 验证命令
```bash
pio test -e native
pio run -e m5stack-stamps3
```
