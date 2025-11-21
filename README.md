# obs-scene-switcher

> OBS Studio plugin for dynamic scene switching based on Twitch events
> (e.g., Channel Point redemption).
> Built with C++ & Qt6, using the official OBS Plugin Template.

---

## Required Before Build

1. Place vendor libraries manually

```
src/vendor/ixwebsocket/
src/vendor/json/
```

2. Create OAuth credentials file

```cpp
// src/oauth/twitch_oauth_secrets.h
#pragma once
#define TWITCH_CLIENT_ID     "xxxxxxxx"
#define TWITCH_CLIENT_SECRET "yyyyyyyy"
#define TWITCH_REDIRECT_URI  "http://localhost:12345/callback"
```

Do NOT commit this file to Git.

---

## Build Instructions (CMake Presets)

Run in the project root:

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64 --parallel
```

For full build configuration details, refer to the official template:
https://github.com/obsproject/obs-plugintemplate

---

## License

```
Copyright (C) 2025 ksmksks
SPDX-License-Identifier: GPL-2.0-or-later
```

This project follows GPL-2.0-or-later to comply with OBS licensing.

---

## Support

This plugin is developed as an open-source project.
Contributions and optional donations are welcome.

Donation information will be added in a future release.

---

## Author

ksmksks

- Twitch: https://twitch.tv/ksmksks
- X: https://x.com/ksmksks

---

If you find this project useful, please consider starring the repository.
