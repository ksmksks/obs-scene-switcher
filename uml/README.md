# UML Diagrams
# OBS Scene Switcher

このディレクトリには、OBS Scene Switcher のアーキテクチャを説明する UML 図が含まれています。

## バージョン対応

| UML 図 | 対応バージョン | 説明 |
|--------|--------------|------|
| すべての図 | v0.9.0 (Beta) | ベータ版アーキテクチャ |
| すべての図 | v1.0.0 (Planned) | 正式版で仕様確定予定 |

**注意**: v0.9.0 はベータ版です。v1.0.0 で正式リリース予定。

## 図の一覧

### システム構成図

- **`uml_component_system_overview.puml`**: システム全体のコンポーネント構成
  - プラグインコア、UI、Twitch連携の関係を示す

### クラス図

- **`uml_class_diagram_full.puml`**: 完全なクラス図
  - 全クラス、メソッド、関係性を網羅
  - enable フラグ、priority に関する詳細情報の追加

- **`uml_class_rule_model.puml`**: ルールモデルのクラス図
  - RewardRule, RuleRow, SettingsWindow の関係
  - ActionType, ActionParams の追加

### ステートマシン図

- **`uml_state_scene_switcher.puml`**: SceneSwitcher の状態遷移
  - Idle → Switched → Reverting → Idle
  - Suppressed (一時的な状態)

### シーケンス図

- **`uml_sequence_oauth_authentication.puml`**: OAuth 認証フロー
  - Twitch OAuth による認証の完全な流れ
  - トークン取得、ユーザー情報取得、保存
  - 例外処理フローの追加

- **`uml_sequence_plugin_enable.puml`**: プラグイン有効化フロー
  - Enable/Disable ボタンの動作
  - EventSub 接続/切断
  - UI 更新処理の詳細

- **`uml_sequence_eventsub_scene_switch.puml`**: EventSub 経由シーン切り替え
  - チャンネルポイント使用からシーン切り替えまで
  - シーンマッチングロジック（Any対応）
  - 抑制状態の処理（1秒表示）
  - 自動復帰
  - 例外処理とタイムアウトの追加

## 図の生成方法

PlantUML を使用して図を生成します。

### オンライン

[PlantUML Online Server](http://www.plantuml.com/plantuml/uml/)

### ローカル

```bash
# PlantUML のインストール (Java 必須)
# https://plantuml.com/ja/download

# PNG 生成
java -jar plantuml.jar uml/*.puml

# SVG 生成
java -jar plantuml.jar -tsvg uml/*.puml
```

### VS Code

[PlantUML 拡張機能](https://marketplace.visualstudio.com/items?itemName=jebbs.plantuml) をインストール

## 主要な設計原則

### 1. State-Driven Architecture

シーン切り替えは必ず State Machine を介して行う。

```
Idle → Switched → Reverting → Idle
       ↓
   Suppressed (一時的)
```

### 2. Plugin Enable/Disable Control

- プラグインは起動時に常に **Disabled**
- UI トグルボタンでのみ有効化可能
- Enabled = EventSub 接続
- Disabled = EventSub 完全停止

### 3. Rule Priority System

- ルールは上から順にチェック
- 最初の **有効な** マッチングルールのみ実行
- 無効なルールはスキップ

### 4. Scene Matching

- **「任意(Any)」**: どのシーンからでもトリガー可能
- **特定シーン指定**: そのシーンがアクティブな時のみトリガー
- マッチング順序:
  1. rewardId の一致
  2. enabled フラグの確認
  3. sourceScene のチェック:
     - 空文字列または "Any" → すべてのシーンにマッチ
     - 具体的なシーン名 → 現在のシーンと一致する必要あり

**推奨**: より具体的なシーンルールを「任意」ルールより上に配置

### 5. Suppression Behavior

- 切り替え中に新しいリクエスト → 1秒間「⚠ 抑制中」表示
- タイマーは継続（中断しない）
- 1秒後に「🔄 切替中」表示に戻る

## 今後の拡張計画（予定）

v1.0.0 正式版リリース後、以下の拡張を計画しています：

### v1.1.x — アクション実行基盤（予定）

- Action Type による処理の抽象化
- 統一実行モデルの導入
- 既存のシーン切替動作は完全に維持

**UML への影響**:
- 新しいクラス図（Action 抽象化レイヤ）の追加予定
- 既存のシーケンス図は保持

### v1.2.x — VTube Studio 連携（基本）（予定）

- VTube Studio 接続機能
- 表情トリガー実行

**UML への影響**:
- VTube Studio 連携用のシーケンス図を追加予定
- コンポーネント図に VTube Studio 接続を追加

### v1.3.x — 高度な表情制御（予定）

- 表情の自動復帰
- 複合アクション対応

**UML への影響**:
- 複合アクション用の状態遷移図を追加予定

詳細は以下を参照：
- [doc/architecture.md](../doc/architecture.md)
- [doc/versioning.md](../doc/versioning.md)
- [README.md](../README.md) のロードマップセクション

## 詳細仕様

詳細な設計仕様は以下のドキュメントを参照してください：

- **`doc/architecture.md`**: アーキテクチャ詳細
- **`doc/versioning.md`**: バージョン契約とマイルストーン
- **`.github/copilot-instructions.md`**: 実装ガイドライン
