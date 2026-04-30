# BT Keyboard 页面 Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 在现有导航体系中增加 `BT Keyboard` 页面，让 Cardputer-ADV 可作为 BLE HID 键盘连接手机/电脑并输入英文/符号。

**Architecture:** 基于当前 `Nav/Gnss` 状态机扩展第三个视图 `BtKeyboard`。新增 `app/bt_keyboard` 模块封装 BLE 键盘状态机（Advertising/Connected/Disconnected/InitFailed）与按键发送逻辑，`main.cpp` 仅做页面路由、输入分发与界面渲染。通过新增纯函数测试覆盖导航项扩展和蓝牙状态转换核心逻辑。

**Tech Stack:** PlatformIO, Arduino(ESP32-S3), M5Unified, M5Cardputer, BLE HID keyboard library, Unity(native).

---

### Task 1: 扩展导航状态测试与纯函数（先红后绿）

**Files:**
- Modify: `include/app/view_nav.h`
- Modify: `src/app/view_nav.cpp`
- Modify: `test/test_view_nav/test_main.cpp`

**Step 1: Write the failing test**

在 `test/test_view_nav/test_main.cpp` 增加以下测试（保持现有测试不删）：

```cpp
void test_nav_move_supports_two_items() {
  TEST_ASSERT_EQUAL_UINT8(1, moveNavSelection(0, +1, 2));
  TEST_ASSERT_EQUAL_UINT8(0, moveNavSelection(1, +1, 2));
  TEST_ASSERT_EQUAL_UINT8(1, moveNavSelection(0, -1, 2));
}
```

并新增视图切换测试（需要 `ViewId::kBtKeyboard`）：

```cpp
void test_next_view_from_nav_bt_menu_with_enter() {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ViewId::kBtKeyboard),
    static_cast<int>(nextViewFromNavSelection(1, true)));
}
```

**Step 2: Run test to verify it fails**

Run: `pio test -e native -f test_view_nav`

Expected: FAIL，提示 `kBtKeyboard` 或 `nextViewFromNavSelection` 未定义。

**Step 3: Write minimal implementation**

1. 在 `include/app/view_nav.h`：
   - 扩展 `enum class ViewId` 增加 `kBtKeyboard`
   - 声明 `ViewId nextViewFromNavSelection(std::uint8_t nav_index, bool enter_pressed);`
2. 在 `src/app/view_nav.cpp`：
   - 实现 `nextViewFromNavSelection`
   - `nav_index==0` 映射 `kGnss`，`nav_index==1` 映射 `kBtKeyboard`

**Step 4: Run test to verify it passes**

Run: `pio test -e native -f test_view_nav`

Expected: PASS。

**Step 5: Commit**

```bash
git add include/app/view_nav.h src/app/view_nav.cpp test/test_view_nav/test_main.cpp
git commit -m "test: 扩展导航视图状态机覆盖蓝牙菜单"
```

### Task 2: 新增 BT Keyboard 纯逻辑状态机模块（先测后码）

**Files:**
- Create: `include/app/bt_keyboard.h`
- Create: `src/app/bt_keyboard.cpp`
- Create: `test/test_bt_keyboard/test_main.cpp`

**Step 1: Write the failing test**

创建 `test/test_bt_keyboard/test_main.cpp`，覆盖以下行为：

```cpp
void test_initial_state_is_idle();
void test_enter_view_moves_to_advertising_on_success();
void test_enter_view_moves_to_init_failed_on_init_error();
void test_disconnect_transitions_back_to_advertising();
void test_exit_view_returns_idle_and_stops_service();
```

测试只验证纯状态转换函数（不直接依赖 BLE 库）。

**Step 2: Run test to verify it fails**

Run: `pio test -e native -f test_bt_keyboard`

Expected: FAIL（头文件/符号不存在）。

**Step 3: Write minimal implementation**

在新模块定义：

- `enum class BtKbdUiState { kIdle, kAdvertising, kConnected, kDisconnected, kInitFailed };`
- `struct BtKbdState { BtKbdUiState ui; bool service_started; bool connected; };`
- 纯函数：
  - `BtKbdState enterBtKbd(bool init_ok)`
  - `BtKbdState onBtKbdConnected(BtKbdState s)`
  - `BtKbdState onBtKbdDisconnected(BtKbdState s)`
  - `BtKbdState exitBtKbd(BtKbdState s)`

先只做状态转换，不做真实 BLE I/O。

**Step 4: Run test to verify it passes**

Run: `pio test -e native -f test_bt_keyboard`

Expected: PASS。

**Step 5: Commit**

```bash
git add include/app/bt_keyboard.h src/app/bt_keyboard.cpp test/test_bt_keyboard/test_main.cpp
git commit -m "test: 新增蓝牙键盘页面状态机与单元测试"
```

### Task 3: 接入 BLE HID 键盘依赖并实现最小发送桥接

**Files:**
- Modify: `platformio.ini`
- Modify: `include/app/bt_keyboard.h`
- Modify: `src/app/bt_keyboard.cpp`

**Step 1: Write the failing test**

增加（或扩展）`test/test_bt_keyboard/test_main.cpp`：

```cpp
void test_key_event_is_ignored_when_not_connected();
void test_key_event_is_sent_when_connected();
```

通过注入 fake sender 断言调用次数，避免 native 依赖真实 BLE。

**Step 2: Run test to verify it fails**

Run: `pio test -e native -f test_bt_keyboard`

Expected: FAIL（发送接口未实现）。

**Step 3: Write minimal implementation**

1. `platformio.ini` 加入 BLE 键盘依赖（固定 owner/version，避免冲突）。
2. `bt_keyboard` 模块增加发送抽象：
   - `sendText(const char*)`
   - `sendSpecialKey(BtKey key, BtModifiers mods)`
3. 连接态才发送；未连接直接返回 false。

**Step 4: Run tests/build to verify pass**

Run:
- `pio test -e native -f test_bt_keyboard`
- `pio run -e m5stack-stamps3`

Expected: native PASS + 设备构建 SUCCESS。

**Step 5: Commit**

```bash
git add platformio.ini include/app/bt_keyboard.h src/app/bt_keyboard.cpp test/test_bt_keyboard/test_main.cpp
git commit -m "feat: 接入BLE HID键盘发送桥接"
```

### Task 4: 在 main.cpp 接入 BT Keyboard 页面与菜单路由

**Files:**
- Modify: `src/main.cpp`

**Step 1: Write the failing test**

此任务以集成为主，不新增 host 级 UI 测试；通过编译/行为检查作为验证。

先在 `main.cpp` 引入 `kBtKeyboard` 路由分支（未实现完整处理）。

**Step 2: Run build to verify it fails**

Run: `pio run -e m5stack-stamps3`

Expected: 若接口未接齐，编译失败（缺失符号）。

**Step 3: Write minimal implementation**

1. 菜单项扩展：
   - `kNavItemCount = 2`
   - `kNavItemGnss = 0`, `kNavItemBtKeyboard = 1`
2. 导航 Enter 跳转：使用 `nextViewFromNavSelection(g_nav_index, keys.enter)`。
3. 新增 `drawBtKeyboardPage()`：显示设备名、当前状态（Advertising/Connected/...）、`BtnGO: Back`。
4. 新增 `handleBtKeyboardInput()`：
   - `BtnGO` 退出
   - 键盘事件转发到 `bt_keyboard` 发送接口
5. 生命周期：
   - 进入 `kBtKeyboard` 调用 `enterBtKbd()` 并自动广播
   - 离开时调用 `exitBtKbd()`

**Step 4: Run tests/build to verify pass**

Run:
- `pio test -e native`
- `pio run -e m5stack-stamps3`

Expected: 全部 PASS/SUCCESS。

**Step 5: Commit**

```bash
git add src/main.cpp
git commit -m "feat: 新增BT Keyboard页面并接入导航路由"
```

### Task 5: 更新 README 与实机验证清单

**Files:**
- Modify: `README.md`

**Step 1: Write the failing test**

文档任务不新增单测。把“README 缺少 BT Keyboard 使用步骤”作为待修复问题。

**Step 2: Run check to verify gap exists**

Run: `pio test -e native`

Expected: PASS（确认代码稳定），同时 README 未覆盖 BT Keyboard 指引。

**Step 3: Write minimal implementation**

在 `README.md` 补充：

- 新导航项 `BT Keyboard`
- 自动广播说明
- 手机/电脑连接步骤
- 支持键说明（英文/符号 + Enter/Backspace/Tab/Space）
- `BtnGO` 返回

**Step 4: Run verification**

Run:
- `pio test -e native`
- `pio run -e m5stack-stamps3`

Expected: PASS + SUCCESS。

**Step 5: Commit**

```bash
git add README.md
git commit -m "docs: 补充BT Keyboard页面使用说明"
```

### Task 6: 设备烧录与端到端验收

**Files:**
- Modify: `src/main.cpp`（仅联调必要小修）
- Modify: `src/app/bt_keyboard.cpp`（仅联调必要小修）

**Step 1: Write the failing test**

准备验收 checklist，任何一项失败即未完成：

1. 可进入 `BT Keyboard` 页面
2. 手机/电脑可发现并连接
3. `abc123`、空格、回车、退格、Tab 可输入
4. 断开后可再次连接
5. `BtnGO` 返回导航并停止广播

**Step 2: Run upload and verify**

Run:
- `pio run -e m5stack-stamps3 -t upload`

Expected: 上传 SUCCESS；实机 checklist 全通过。

**Step 3: Write minimal fixes (if needed)**

仅修复验收失败项，不扩展功能。

**Step 4: Re-run verification**

Run:
- `pio test -e native`
- `pio run -e m5stack-stamps3`
- `pio run -e m5stack-stamps3 -t upload`

Expected: 全部通过。

**Step 5: Commit**

```bash
git add src/main.cpp src/app/bt_keyboard.cpp
git commit -m "fix: 修正蓝牙键盘联调问题"
```

## 完成定义（DoD）

- 导航可进入 `BT Keyboard`。
- 页面进入后自动广播，连接后可打字。
- 第一版支持字符与基础控制键，`BtnGO` 可返回。
- native 测试全绿，设备构建成功，实机验收通过。
