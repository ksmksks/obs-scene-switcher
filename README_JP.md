# OBS Scene Switcher

Twitch チャンネルポイントで OBS のシーンを自動切り替えするプラグインです。

> **⚠️ 現在のバージョン**: v0.9.2 ベータ版です。不具合やフィードバックは [GitHub Issues](https://github.com/ksmksks/obs-scene-switcher/issues) までお願いします。

## 機能

- **Twitch チャンネルポイント連携**: チャンネルポイントの使用をトリガーにシーンを自動切り替え
- **柔軟なシーンマッチング**: 特定のシーンまたは「任意」のシーンをトリガー条件に選択可能
- **自動復帰**: 指定した秒数後に元のシーンに自動で戻る
- **ルール管理**: 複数のルールを設定し、ドラッグ&ドロップで優先度を変更
- **競合制御**: 切り替え中の重複実行を自動的に抑制
- **ON/OFF 制御**: プラグインの有効/無効をワンクリックで切り替え
- **配信連動**: 配信開始時に自動有効化、配信停止時に自動無効化

## インストール

### 必要な環境

- **OBS Studio**: 30.0.0 以降
- **OS**: Windows 10/11 (64-bit)
- **Twitch アカウント**

> **注意**: これはベータ版です。v1.0.0 で安定版をリリース予定です。

### 方法1: インストーラー（推奨）

1. 最新リリースから [obs-scene-switcher-0.9.2-installer.exe](https://github.com/ksmksks/obs-scene-switcher/releases/latest) をダウンロード
2. 管理者としてインストーラーを実行
3. インストールウィザードに従う
   - OBS Studio は自動的に検出されます
   - インストーラーが適切なディレクトリにファイルを配置します
4. OBS Studio を再起動

インストーラーは以下を実行します：
- OBS Studio のインストール先を自動検出
- プラグインファイルを OBS ディレクトリにインストール
- 簡単にアンインストールできるよう Windows の設定に登録

### 方法2: 手動インストール

1. 最新リリースから [obs-scene-switcher-0.9.2-manual.zip](https://github.com/ksmksks/obs-scene-switcher/releases/latest) をダウンロード
2. ZIP ファイルを展開
3. `obs-plugins` フォルダを OBS Studio ディレクトリにコピー
   - デフォルト: `C:\Program Files\obs-studio\`
4. `data` フォルダを OBS Studio ディレクトリにコピー
5. OBS Studio を再起動

**注意**: 手動インストールには管理者権限は不要です。

### アンインストール

#### Windows 設定から
1. **設定** → **アプリ** を開く
2. **「OBS Scene Switcher」** を検索
3. **アンインストール** をクリック

#### アンインストーラーから
`C:\Program Files\obs-scene-switcher\uninstall.exe` を実行

アンインストーラーはプラグインファイルのみを削除し、OBS Studio には影響しません。

## 事前準備

### Twitch Client ID / Client Secret の取得

obs-scene-switcher では、Twitch EventSub と API を利用するために
Twitch Developer Console で発行した Client ID / Client Secret が必要です。

以下の手順に従って取得してください。

#### 1. Twitch Developer Console にアクセス

1. https://dev.twitch.tv/console にアクセス
2. Twitch アカウントでログイン
3. メニューから 「アプリケーション」 を選択

#### 2. 新しいアプリケーションを作成

1. 「アプリケーションを登録」 をクリック
2. 以下のように入力します：

| 項目 | 設定内容 |
| :--- | :--- |
| 名前 | obs-scene-switcher（任意） |
| OAuthのリダイレクトURL | http://localhost:38915/callback |
| カテゴリー | Application Integration |
| クライアントのタイプ | Confidential |

3. 「作成」を押します

#### 3. Client ID を確認

作成したアプリの詳細画面に **クライアントID(Client ID)** が表示されます。
この値を obs-scene-switcher の認証設定画面に入力してください。

#### 4. Client Secret を生成・表示

- 「**新しい秘密**」 をクリック
- 表示された **クライアントの秘密(Client Secret)** をコピー
- obs-scene-switcher の認証設定画面に入力します

⚠ **クライアントの秘密(Client Secret)は外部に公開しないでください**

#### 5. obs-scene-switcher に入力

obs-scene-switcher の設定画面で以下を入力します：
- Client ID
- Client Secret
保存後、OAuth 認証・EventSub 接続が有効になります。

##### 補足

- Client Secret を再生成すると、**以前の Secret は無効**になります
- その場合は obs-scene-switcher 側も再設定してください
- Redirect URL はローカル用途のため `http://localhost` で問題ありません

## 使い方

### 1. Twitch 認証

1. OBS を起動
2. 「ドック」→「Scene Switcher」を選択
3. 「認証設定」ボタンをクリックし、Client ID と Client Secret を入力し、「保存」ボタンをクリック
4. 「Twitch でログイン」ボタンをクリック
5. ブラウザで Twitch の認証画面が開くので、「Authorize」をクリック
6. 認証が完了すると、Dock に緑色の●が表示されます

### 2. ルール設定

1. Dock の「設定」ボタンをクリック
2. 「＋ ルール」ボタンで新しいルールを追加
3. 各項目を設定:
   - **現在シーン**: 
     - **「任意(Any)」**を選択すると、どのシーンからでもトリガー可能
     - または特定のシーン名を選択すると、そのシーンがアクティブな時のみトリガー
   - **リワード**: Twitch チャンネルポイントを選択
   - **切替先シーン**: 切り替え先のシーン
   - **秒数**: 自動復帰までの時間（0 で復帰しない）
4. 「保存」をクリック

### 3. プラグインを有効化

1. Dock の **有効/無効** トグルボタンをクリック
2. ボタンが緑色になり、状態が「🟢 待機中」になれば OK
3. 配信中にチャンネルポイントを使用すると自動的にシーンが切り替わります

**配信時の自動有効化：**
- OBS で配信を開始すると、プラグインは自動的に有効化されます（認証済みの場合）
- 配信を停止すると、プラグインは自動的に無効化されます
- これにより、プラグインは配信中のみアクティブになります

### 4. ルールの並び替え

- 設定画面で、左端の ⋮⋮ アイコンをドラッグしてルールの順序を変更できます
- **上にあるルールが優先** されます
- 同じチャンネルポイントに複数のルールがある場合、最初の **有効な** ルールのみが実行されます
- **ヒント**: より具体的なシーンルールを「任意」ルールよりも上に配置すると、より細かい制御が可能です

### 5. ルールの有効/無効

- 各ルールの左端にあるチェックボックスで有効/無効を切り替えられます
- 無効にしたルールはグレーアウトされ、実行されません
- 削除せずに一時的に無効化できます

## シーンマッチングの動作

プラグインは **上から順に** ルールを評価し、**最初にマッチした有効なルール** を実行します：

### ルール優先度の例

```
ルール1: シーンA → リワードX → シーンB (10秒)  ✓ 有効
ルール2: 任意(Any) → リワードX → シーンC (5秒)   ✓ 有効
```

**動作:**
- 現在のシーンが **シーンA** → ルール1がマッチ → シーンBに切り替え
- 現在のシーンが **シーンD** → ルール2がマッチ → シーンCに切り替え

**推奨**: より具体的なルール（特定シーン指定）を汎用的なルール（任意）よりも上に配置してください。

## 状態表示

Dock には現在の状態が表示されます：

- **⏸ 待機中（無効）**: プラグインが無効です（有効ボタンで有効化）
- **🟢 待機中**: プラグインが有効で、チャンネルポイントを待っています
- **🔄 切替中**: シーンが切り替わり、復帰待機中（秒数も表示されます）
- **⏱ 復帰中**: 元のシーンに戻る処理中
- **⚠ 抑制中**: 切り替え中に新しいリクエストが来たため抑制されています

## トラブルシューティング

### シーンが切り替わらない

1. **プラグインが有効か確認**
   - Dock の 有効/無効 ボタンが緑色になっているか確認
   - 状態が「🟢 待機中」になっているか確認

2. **ルールが有効か確認**
   - 設定画面でルールのチェックボックスがオンになっているか確認
   - グレーアウトしている場合は無効になっています

3. **認証が有効か確認**
   - Dock の丸印が緑色●になっているか確認
   - 赤色●の場合は「Twitch でログイン」で再認証

4. **ルールの設定を確認**
   - チャンネルポイントが正しく選択されているか
   - 切り替え先のシーンが存在するか
   - 特定シーンを指定している場合、現在のシーンが一致しているか

5. **ルールの優先度を確認**
   - ルールは上から順に評価されます
   - 上位のルールがマッチしている可能性があります
   - ルールの並び替えか「任意」の使用を検討してください

### 認証エラー

- **「Token refresh failed」**: トークンの有効期限が切れています
  - 「ログアウト」 → 「Twitch でログイン」 で再認証してください

- **「Cannot enable: not authenticated」**: 認証されていません
  - 「Twitch でログイン」で認証してください

### WebSocket 接続エラー

- ファイアウォールで Twitch への接続がブロックされている可能性があります
- OBS のログ（ヘルプ→ログファイル→現在のログを表示）を確認してください

## デバッグログ

問題が発生した場合は、OBS のログを確認してください：

1. **ヘルプ** → **ログファイル** → **現在のログを表示**
2. `[obs-scene-switcher]` で検索
3. デバッグメッセージでプラグインの詳細な動作を確認できます

通常のログ出力（最小限）:
```
[obs-scene-switcher] plugin loaded successfully (version X.X.X)
```

デバッグログ出力（詳細）:
```
[obs-scene-switcher] Initializing plugin
[obs-scene-switcher] Loaded 2 reward rules
[obs-scene-switcher] Authentication successful
[obs-scene-switcher] Current scene: ゲームシーン
[obs-scene-switcher] Redemption received: reward=xxx user=yyy
[obs-scene-switcher] Matched rule: Any -> ターゲットシーン (revert: 10 sec)
```

## ロードマップ

**現在のステータス**: ベータ版 - テストとフィードバック収集中

**現在のバージョン**: v0.9.2  
**次のリリース**: ベータ検証後に v1.0.0 安定版をリリース予定

### v0.9.x — ベータ検証（現在）

- v0.9.2 (2025-01-07): シーン選択保持の重大なバグ修正
- v0.9.1 (2025-01-07): バグ修正と配信連動機能
- v0.9.0 (2025-01-07): 初回ベータリリース
- 進行中: ユーザーフィードバックの収集と改善

---

### v1.0.0 — 安定版リリース（予定）

コアとなる OBS シーン切替機能を確定します。
今後の開発は、既存のシーン切替動作を維持したまま、
拡張機能を段階的に追加することに焦点を当てます。

---

### v1.1.x — アクション実行基盤（予定）

- アクション処理のための統一実行モデルの導入
- Action Types によるシーン切替と外部アクションの抽象化
- 安全かつ拡張可能な機能追加を可能にする内部リファクタリング

---

### v1.2.x — VTube Studio 連携（基本）（予定）

- VTube Studio への接続サポート
- Twitch チャンネルポイントによるアバター表情（Expression / Hotkey）のトリガー
- UI から利用可能な表情の取得と選択
- 単発表情実行（自動復帰なし）

---

### v1.3.x — 高度な表情制御（予定）

- 指定時間後の表情の自動復帰
- 表情切替中の競合抑制
- 単一ルール内での複合アクション
  - OBS シーン切替 + VTube Studio 表情

---

### 今後の検討事項

- より柔軟なアクション定義
- 配信向けの追加機能
- コミュニティフィードバックに基づく改善

アーキテクチャ設計原則とバージョン規則については  
[doc/architecture.md](doc/architecture.md) と [doc/versioning.md](doc/versioning.md) を参照してください。

## 開発者向け情報

### ビルド手順

#### プラグインのビルド

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64 --config Release --parallel
```

#### インストーラーのビルド（NSIS が必要）

1. [NSIS 3.x (Unicode)](https://nsis.sourceforge.io/) をインストール
2. インストーラーをビルド:

```powershell
cmake --build build_x64 --target installer --config Release
```

出力: `installer/obs-scene-switcher-{version}-installer.exe`

#### 手動インストール用 ZIP のビルド

```powershell
cmake --build build_x64 --target package-zip --config Release
```

出力: `release/obs-scene-switcher-{version}-manual.zip`

詳細は [doc/architecture.md](doc/architecture.md) と [doc/versioning.md](doc/versioning.md) を参照してください。

## 設定ファイルの場所

設定ファイルは以下の場所に保存されます：

```
%APPDATA%\obs-studio\plugin_config\obs-scene-switcher\config.ini
```

## ライセンス

GNU General Public License v2.0

Copyright (C) 2025 ksmksks

このソフトウェアは GPL-2.0 ライセンスのもとで公開されています。
詳細は [LICENSE](LICENSE) ファイルをご覧ください。

## 謝辞

このプロジェクトは、[OBS Project](https://github.com/obsproject) によって作成された [obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate) を使用して開発されました。

## クレジット

- **開発**: ksmksks
- **Twitch**: [https://twitch.tv/ksmksks](https://twitch.tv/ksmksks)
- **X**: [https://x.com/ksmksks](https://x.com/ksmksks)

## サポート

質問やバグ報告は [Issues](https://github.com/ksmksks/obs-scene-switcher/issues) までお願いします。

このプロジェクトが役に立ったら、ぜひスターをお願いします！

---

**English**: See [README.md](README.md)
