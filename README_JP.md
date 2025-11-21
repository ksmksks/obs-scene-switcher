# obs-scene-switcher

OBS のシーンを Twitch のチャネポ（Channel Point）などのイベントに応じて  
自動切り替えすることを目的としたプラグインです。  
C++ / Qt6 を使用し、公式 OBS Plugin Template をベースに開発しています。

---

## ビルド前の準備

1. vendor ライブラリを配置

```
src/vendor/ixwebsocket/
src/vendor/json/
```

2. OAuth シークレットファイルを作成

```cpp
// src/oauth/twitch_oauth_secrets.h
#pragma once
#define TWITCH_CLIENT_ID     "xxxxxxxx"
#define TWITCH_CLIENT_SECRET "yyyyyyyy"
#define TWITCH_REDIRECT_URI  "http://localhost:12345/callback"
```

このファイルは Git に含めないようにしてください。

---

## ビルド方法（CMake Presets 使用推奨）

プロジェクトのルートディレクトリで実行します：

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64 --parallel
```

詳細なビルド構成については、以下の公式テンプレートをご確認ください：  
https://github.com/obsproject/obs-plugintemplate

---

## ライセンス

```
Copyright (C) 2025 ksmksks
SPDX-License-Identifier: GPL-2.0-or-later
```

OBS のライセンスに従い、GPL-2.0-or-later を採用しています。

---

## サポートについて

本プラグインはオープンソースとして開発されています。
寄付やご支援は歓迎いたします。

寄付に関する情報は今後追加予定です。

---

## 作者

ksmksks

- Twitch: https://twitch.tv/ksmksks
- X: https://x.com/ksmksks

---

本プロジェクトが役に立った場合、Star をいただけると励みになります。
