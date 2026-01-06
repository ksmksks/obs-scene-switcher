# Architecture
# OBS Scene Switcher

> NOTE FOR COPILOT:
> このドキュメントは OBS Scene Switcher の正規アーキテクチャ仕様である。
> 状態遷移を省略・簡略化した実装、または設計原則を無視した実装は禁止。

---

## 1. 全体構成

本プラグインは、Twitch Channel Point をトリガーとして  
OBS のシーン切替を **状態駆動で安全に実行**することを目的とする。

主要コンポーネントは以下の通り：

- **SceneSwitcher**
  - State Machine によるシーン切替制御
  - 復帰タイマー管理
  - 状態変更通知
  
- **PluginDock**
  - UI コンテナ管理
  - LoginWidget / DockMainWidget の切り替え
  
- **DockMainWidget**
  - UI 表示およびユーザー操作を担当

- **ObsSceneSwitcher**
  - プラグイン全体の制御・状態管理
  - OBS イベント監視（配信開始/停止）

- **EventSubClient**
  - Twitch EventSub WebSocket 通信
  - Redemption 通知受信
  
- **ConfigManager**
  - 設定の永続化
  - 認証情報管理

- **TwitchOAuth**
  - OAuth 認証フロー
  - トークン管理とリフレッシュ

---

## 2. Scene Switch State Machine

### 2.1 State 定義

すべてのシーン切替処理は、明示的な State Machine を介して行う。

```cpp
enum class State {
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
   Suppressed (一時的、Switched に戻る)
```

**詳細:**

- **Idle**  
  待機状態。新しいリクエストを受け付け可能。

- **Switched**  
  シーン切替後、復帰待機中。
  - UI に残り時間を表示

- **Reverting**  
  元のシーンへ戻す処理中。  
  処理完了後すぐに Idle へ遷移。

- **Suppressed**  
  切替中に新しいリクエストが来たため抑制された状態。  
  - タイマーは継続（中断しない）
  - 1秒間表示後、Switched 表示に戻る
  ※ Suppressed は内部状態遷移ではなく、UI 表示上の一時的な状態を表す。

### 2.3 State Machine 実装

State Machine に基づくシーン切替処理：

- 現在の State が Idle の場合：
  - ルールに基づいてシーンを切り替える
  - 復帰タイマーを開始する
  - State を Switched に遷移させる

- 現在の State が Switched または Reverting の場合：
  - 新しい切替要求は抑制対象とする
  - 内部 State は変更しない
  - UI に一時的な Suppressed 状態を通知する

---

## 3. Plugin Enable/Disable Control

### 3.1 基本方針

プラグインのライフサイクルは、明示的な Enable / Disable 操作によってのみ制御される。

- プラグインは起動時に常に **Disabled** 状態
- 有効 / 無効は UI 操作により制御される
- Enabled 状態と EventSub 接続状態は常に同期される
- OBS の配信／録画イベントによる有効 / 無効切替は、UI 操作と同一の setEnabled() フローを通じて行われる

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

### 3.5 配信連動

OBS の配信イベントに応じて自動的に有効/無効を切り替える：

**配信開始時**:
```
OBS_FRONTEND_EVENT_STREAMING_STARTED
  ↓
isAuthenticated() && !isEnabled() をチェック
  ↓
setEnabled(true) → WebSocket 接続
  ↓
UI: "🟢 待機中"
```

**配信停止時**:
```
OBS_FRONTEND_EVENT_STREAMING_STOPPED
  ↓
isEnabled() をチェック
  ↓
setEnabled(false) → WebSocket 切断
  ↓
UI: "⏸ 待機中（無効）"
```

**設計方針**:
- UI 操作と同じ `setEnabled()` フローを使用
- 認証済みの場合のみ自動有効化
- 一貫性のある動作を保証

### 3.6 Redemption処理

Redemption 受信時の処理方針：

- プラグインが無効（Disabled）の場合：
  - すべての Redemption を無視する
- プラグインが有効（Enabled）の場合：
  - Rule 評価および State Machine による処理を行う

---

## 4. Rule Evaluation

### 4.1 ルール評価順序

ルールは **上から順に** 評価され、**最初にマッチした有効なルール** のみが実行される。

```cpp
for (const auto &rule : rewardRules_) {
    // 1. rewardId の一致チェック
    if (rule.rewardId != rewardId)
        continue;
    
    // 2. enabled フラグの確認
    if (!rule.enabled)
        continue;
    
    // 3. sourceScene のマッチング
    bool sourceMatches = rule.sourceScene.empty() || 
                         rule.sourceScene == "Any" || 
                         rule.sourceScene == currentSceneStr;
    if (!sourceMatches)
        continue;
    
    // マッチしたルールを実行して終了
    sceneSwitcher_->switchWithRevert(rule);
    return;
}
```

### 4.2 ルール優先度

- より具体的なシーン指定ルールを上に配置
- 汎用的な「任意(Any)」ルールを下に配置
- ユーザーはドラッグ&ドロップで優先度を変更可能

**例:**
```
ルール1: シーンA → リワードX → シーンB (10秒)  ✓ 有効
ルール2: 任意(Any) → リワードX → シーンC (5秒)   ✓ 有効
```

現在のシーンが「シーンA」の場合 → ルール1がマッチ  
現在のシーンが「シーンD」の場合 → ルール2がマッチ

---

## 5. UI State Display

### 5.1 状態テキスト

| State | pluginEnabled | 表示テキスト |
|-------|--------------|-------------|
| - | false | `⏸ 待機中（無効）` |
| Idle | true | `🟢 待機中` |
| Switched | true | `🔄 切替中: シーン名` |
| Reverting | true | `⏱ 復帰中: シーン名 へ` |
| Suppressed | true | `⚠ 抑制中` |

- 表示は現在の State を忠実に反映する
- 表示目的でのロジック分岐追加は禁止

### 5.2 カウントダウンタイマー

Switched 状態中は、一定間隔で残り復帰時間を更新する。

- 現在の State が Switched の場合のみ更新を行う
- 残り時間が有効な場合、以下の情報を UI に通知する：
  - 現在の State（Switched）
  - 残り復帰時間
  - 切替先シーン
  - 元のシーン

---

## 6. Suppression Behavior

### 6.1 抑制の目的

切り替え中に新しいリクエストが来た場合、既存のタイマーを尊重し、  
新しいリクエストは抑制する。

### 6.2 抑制の実装

Switched または Reverting 状態で新しい切替要求を受信した場合：

- 内部状態（state_）は変更しない
- 一時的に Suppressed 状態を UI に通知する
- 一定時間後、元の表示状態（Switched）に戻す
- 復帰タイマーは中断・再設定しない

### 6.3 抑制時の動作

- **タイマーは継続**: 中断・リセットしない
- **カウントダウン継続**
- **一時的な表示**: 1秒間「⚠ 抑制中」を表示後、「🔄 切替中」に戻る

---

## 7. 禁止事項（厳守）

- 起動時の自動 EventSub 接続
- UI 操作なしでの自動有効化
- pluginEnabled_ と EventSub 接続状態の不一致
- State Machine を介さないシーン切替
- 状態遷移の省略・簡略化
- OBS API を直接呼び出してシーンを切り替えるショートカット実装

---

## 8. 技術スタック

- **言語**: C++17
- **UI フレームワーク**: Qt 6
- **OBS API**: obs-frontend-api
- **WebSocket**: IXWebSocket
- **JSON パース**: nlohmann/json
- **HTTP**: Windows WinINet API

## 9. v0.9.0 のスコープ（ベータ版）

本ドキュメントは v0.9.0 ベータ版における正規アーキテクチャ仕様を定義する。

v0.9.0 ベータ版の機能：

- Twitch Channel Point をトリガーとした OBS シーン切替
- 状態駆動による安全な実行
- 明示的な有効 / 無効ライフサイクル
- 決定的なルール評価と競合抑制
- インストーラーによる配布
- 包括的なドキュメントと UML 図

**ベータ版の目的**: ユーザーフィードバックに基づくバグ修正と改善を行い、v1.0.0 で正式リリース予定。

---

## 9.1 v0.9.1 の追加機能

v0.9.1 では以下の改善が追加された：

- シーンリストの動的更新
- 日本語ロケールの改善
- **配信連動機能**: OBS の配信開始/停止に応じた自動有効化/無効化

配信連動機能により、プラグインは配信中のみアクティブとなり、
リソースの効率的な使用と意図しない動作の防止が実現される。

---

## 10. 将来の拡張（v1.0.0 以降）

### 10.1 拡張の原則

v1.0.0 以降の機能拡張は、以下の原則に従う：

1. **既存機能の保護**
   - コアのシーン切替ロジックに対する破壊的変更は禁止
   - State Machine の基本挙動は維持

2. **拡張レイヤとしての実装**
   - 新機能は既存機能に影響を与えない形で追加
   - オプトイン方式（デフォルトで無効、明示的に有効化）

3. **Action 抽象化**
   - 各種処理は Action として抽象化
   - OBS シーン切替専用の分岐ロジックを増やすことは禁止

### 10.2 Action 抽象化（v1.1.x 以降）

将来の機能拡張に備え、各種処理は Action として抽象化される。

#### Action Type

```cpp
enum class ActionType {
    ObsScene,
    // External actions may be added in future versions
};
```

- `ObsScene` は OBS のシーン切替を表す
- 新しい Action Type は v1.1.x 以降に追加可能

#### Action 実行方針

- すべての Action は State Machine を介して実行される
- State を無視した直接実行は禁止
- Action 実行時は以下を必ず考慮する：
  - プラグインの有効 / 無効状態
  - 現在の State
  - 抑制状態（Suppressed）

#### 設計原則

- 新しい挙動は必ず Action として抽象化する
- Action の追加は既存の State 遷移を壊してはならない
- 複数の Action を組み合わせる場合も、State Machine の制御下で行う

### 10.3 今後の拡張計画

詳細な拡張計画については以下を参照：

- [doc/versioning.md](versioning.md) - バージョン契約と実装範囲
- [README.md](../README.md) - ロードマップセクション

---

## 11. ドキュメント参照

本アーキテクチャ仕様と合わせて、以下のドキュメントを参照すること：

- **[doc/versioning.md](versioning.md)**: バージョン契約とマイルストーン
- **[.github/copilot-instructions.md](../.github/copilot-instructions.md)**: 実装ガイドライン
- **[uml/README.md](../uml/README.md)**: UML 図集
