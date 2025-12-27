# Architecture
# OBS Scene Switcher

> NOTE FOR COPILOT:
> このドキュメントはシーン切替ロジックの正規仕様である。
> 状態遷移を省略・簡略化した実装は禁止。

---

## 1. 全体構成

主要コンポーネント：

- DockMainWidget
- ObsSceneSwitcher
- EventSubClient
- RewardRule / RuleRow
- ConfigManager
- SceneSwitcher

---

## 2. Scene Switch State Machine

### 2.1 State 定義

```cpp
enum class SceneSwitchState {
    Idle,
    Switched,
    Reverting,
    Suppressed
};
```

### 2.2 状態遷移

```
Idle → Switched → Reverting → Idle
       ↓
   Suppressed
```

---

## 3. Plugin Enable/Disable Control（v0.6.1）

### 3.1 基本方針

- プラグインは起動時に常に **Disabled** 状態
- UI トグルボタンでのみ有効化可能
- Enabled 状態と EventSub 接続は常に同期

### 3.2 状態管理

```cpp
class ObsSceneSwitcher {
    bool pluginEnabled_ = false;  // 起動時は常に false
    bool authenticated_ = false;
    bool eventsubConnected_ = false;
};
```

### 3.3 有効化フロー

```
ユーザーがトグルボタンをクリック
  ↓
setEnabled(true)
  ↓
認証済みかチェック
  ↓
connectEventSub() → WebSocket 接続
  ↓
pluginEnabled_ = true
  ↓
UI: "🟢 待機中"
```

### 3.4 無効化フロー

```
ユーザーがトグルボタンをクリック
  ↓
setEnabled(false)
  ↓
disconnectEventSub() → WebSocket 切断
  ↓
pluginEnabled_ = false
  ↓
UI: "⏸ 待機中（無効）"
```

### 3.5 Redemption 処理

```cpp
void onRedemptionReceived(const std::string &rewardId, ...) {
    // v0.6.1: プラグイン無効時は無視
    if (!pluginEnabled_) {
        return;
    }
    
    // ...existing scene switch logic...
}
```

---

## 4. UI State Display（v0.6.1）

### 4.1 状態テキスト

| State | pluginEnabled | 表示テキスト |
|-------|--------------|-------------|
| - | false | `⏸ 待機中（無効）` |
| Idle | true | `🟢 待機中` |
| Switched | true | `🔄 切替中` |
| Reverting | true | `⏱ 復帰中` |
| Suppressed | true | `⚠ 抑制中` |

**注記（v0.6.1 実装）**:
- 基本的な状態表示は実装済み
- v0.6.2 でシーン名を追加予定（例: `🔄 切替中: ゲームシーン`）

### 4.2 カウントダウン

```cpp
// SceneSwitcher から1秒ごとに通知
void onCountdownTick() {
    int remaining = revertTimer_.remainingTime() / 1000;
    emit stateChanged(State::Switched, remaining);
}
```

---

## 5. 禁止事項

- 起動時の自動 EventSub 接続
- UI 操作なしでの自動有効化
- pluginEnabled_ と EventSub 接続状態の不一致
- State Machine の省略・簡略化
