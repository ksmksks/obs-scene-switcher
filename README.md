# OBS Scene Switcher

An OBS Studio plugin that automatically switches scenes based on Twitch Channel Point redemptions.

> **⚠️ Current Version**: v0.9.3 beta. Please report any issues or feedback at [GitHub Issues](https://github.com/ksmksks/obs-scene-switcher/issues).

## Features

- **Twitch Channel Points Integration**: Automatically switch scenes when viewers redeem channel points
- **Flexible Scene Matching**: Choose specific scenes or "Any" scene for trigger conditions
- **Auto-Revert**: Return to the original scene after a specified duration
- **Rule Management**: Configure multiple rules with drag-and-drop priority ordering
- **Conflict Control**: Automatically suppresses duplicate executions during scene transitions
- **Enable/Disable Toggle**: Quickly enable or disable the plugin with one click
- **Streaming Sync**: Automatically enables when streaming starts and disables when streaming stops

## Installation

### Requirements

- **OBS Studio**: 30.0.0 or later
- **OS**: Windows 10/11 (64-bit)
- **Twitch Account**

> **Note**: This is a beta release. Stable v1.0.0 will be released after validation.

### Method 1: Installer (Recommended)

1. Download [obs-scene-switcher-0.9.3-installer.exe](https://github.com/ksmksks/obs-scene-switcher/releases/latest) from the latest release
2. Run the installer as administrator
3. Follow the installation wizard
   - OBS Studio will be automatically detected
   - The installer will place files in the correct directories
4. Restart OBS Studio

The installer will:
- Automatically detect your OBS Studio installation
- Install plugin files to OBS directories
- Register with Windows Settings for easy uninstallation

### Method 2: Manual Installation

1. Download [obs-scene-switcher-0.9.3-manual.zip](https://github.com/ksmksks/obs-scene-switcher/releases/latest) from the latest release
2. Extract the ZIP file
3. Copy the `obs-plugins` folder to your OBS Studio directory
   - Default: `C:\Program Files\obs-studio\`
4. Copy the `data` folder to your OBS Studio directory
5. Restart OBS Studio

**Note**: Manual installation does not require administrator privileges.

### Uninstallation

#### Using Windows Settings
1. Open **Settings** → **Apps**
2. Search for **"OBS Scene Switcher"**
3. Click **Uninstall**

#### Using Uninstaller
Run `C:\Program Files\obs-scene-switcher\uninstall.exe`

The uninstaller will remove only plugin files and leave OBS Studio untouched.

## Prerequisites

### Obtaining Twitch Client ID / Client Secret

To use Twitch EventSub and the Twitch API, obs-scene-switcher requires a **Client ID** and **Client Secret** issued via the Twitch Developer Console.

Please follow the steps below.

#### 1. Access the Twitch Developer Console

1. Open https://dev.twitch.tv/console
2. Log in with your Twitch account
3. Select **Applications** from the menu

#### 2. Create a New Application

1. Click **Register Your Application**
2. Fill in the fields as follows:

| Item | Value |
| :--- | :--- |
| Name | obs-scene-switcher (any name is fine) |
| OAuth Redirect URL | http://localhost:38915/callback |
| Category | Application Integration |
| Client Type | Confidential |

3. Click **Create**

#### 3. Confirm Client ID

On the application details page, your **Client ID** will be displayed.  
Enter this value into the obs-scene-switcher authentication settings.

#### 4. Generate and View Client Secret

- Click **New Secret**
- Copy the displayed **Client Secret**
- Enter it into the obs-scene-switcher authentication settings

⚠ **Do not share your Client Secret publicly**

#### 5. Enter Credentials into obs-scene-switcher

In the obs-scene-switcher settings screen, enter:
- Client ID
- Client Secret

After saving, OAuth authentication and EventSub connection will become active.

##### Notes

- Regenerating the Client Secret will **invalidate the previous one**
- If regenerated, you must reconfigure obs-scene-switcher
- The Redirect URL uses `http://localhost` and is safe for local use

## Usage

### 1. Twitch Authentication

1. Launch OBS Studio
2. Open **Docks** → **Scene Switcher**
3. Click **Authentication Settings**, enter the Client ID and Client Secret, then click **Save**
4. Click **Login with Twitch**
5. A browser window will open — click **Authorize**
6. When authentication succeeds, a green ● indicator will appear in the dock

### 2. Configure Rules

1. Click the **Settings** button in the dock
2. Click **+ Add Rule** to create a new rule
3. Configure each field:
   - **Source Scene**: 
     - Select **"Any"** to trigger from any scene
     - Or select a specific scene name to trigger only when that scene is active
   - **Reward**: Select a Twitch channel point reward
   - **Target Scene**: The scene to switch to
   - **Duration**: Auto-revert time in seconds (0 = no revert)
4. Click **Save**

### 3. Enable the Plugin

1. Click the **Enable/Disable** toggle button in the dock
2. The button should turn green and the status should show "🟢 Waiting"
3. During your stream, when viewers redeem the channel point, the scene will switch automatically

**Auto-Enable on Streaming:**
- When you start streaming in OBS, the plugin will automatically enable itself (if authenticated)
- When you stop streaming, the plugin will automatically disable itself
- This ensures the plugin is only active during your stream

### 4. Reorder Rules

- In the settings window, drag the ⋮⋮ handle to reorder rules
- **Rules at the top have higher priority**
- When multiple rules match the same reward, only the first **enabled** rule executes
- **Tip**: Place specific scene rules above "Any" scene rules for better control

### 5. Enable/Disable Individual Rules

- Use the checkbox on the left of each rule to enable/disable it
- Disabled rules appear grayed out and will not execute
- Temporarily disable rules without deleting them

## Scene Matching Behavior

The plugin evaluates rules **from top to bottom** and executes the **first matching enabled rule**:

### Example Rule Priority

```
Rule 1: Scene A → Reward X → Scene B (10 sec)  ✓ Enabled
Rule 2: Any     → Reward X → Scene C (5 sec)   ✓ Enabled
```

**Behavior:**
- Current scene is **Scene A** → Rule 1 matches → Switch to Scene B
- Current scene is **Scene D** → Rule 2 matches → Switch to Scene C

**Best Practice:** Place more specific rules (with specific source scenes) above generic rules (with "Any").

## Status Indicators

The dock displays the current status:

- **⏸ Waiting (Disabled)**: Plugin is disabled (click Enable button)
- **🟢 Waiting**: Plugin is enabled and waiting for redemptions
- **🔄 Switching**: Scene switched, waiting to revert (shows countdown)
- **⏱ Reverting**: Returning to the original scene
- **⚠ Suppressed**: New request received during transition and was suppressed

## Troubleshooting

### Scene Not Switching

1. **Check if plugin is enabled**
   - Verify the Enable/Disable button is green
   - Status should show "🟢 Waiting"

2. **Check if rule is enabled**
   - In settings, verify the rule's checkbox is checked
   - Grayed out rules are disabled

3. **Check authentication**
   - The indicator should be green ●
   - If red ●, click "Login with Twitch" to re-authenticate

4. **Verify rule configuration**
   - Channel point reward is correctly selected
   - Target scene exists in OBS
   - If using specific source scene, verify current scene matches

5. **Check rule priority**
   - Rules are evaluated top-to-bottom
   - A matching rule above might be executing instead
   - Try reordering rules or using "Any" for broader matching

### Authentication Errors

- **"Token refresh failed"**: Token expired
  - Logout → Login with Twitch to re-authenticate

- **"Cannot enable: not authenticated"**: Not authenticated
  - Click "Login with Twitch" to authenticate

### WebSocket Connection Errors

- Your firewall might be blocking connections to Twitch
- Check OBS logs (Help → Log Files → View Current Log)

## Debug Logging

To enable debug logging for troubleshooting:

1. Go to **Help** → **Log Files** → **View Current Log**
2. Look for `[obs-scene-switcher]` entries
3. Debug messages provide detailed information about plugin behavior

Normal log output (minimal):
```
[obs-scene-switcher] plugin loaded successfully (version X.X.X)
```

Debug log output (detailed):
```
[obs-scene-switcher] Initializing plugin
[obs-scene-switcher] Loaded 2 reward rules
[obs-scene-switcher] Authentication successful
[obs-scene-switcher] Current scene: GameScene
[obs-scene-switcher] Redemption received: reward=xxx user=yyy
[obs-scene-switcher] Matched rule: Any -> TargetScene (revert: 10 sec)
```

## Roadmap

This is a **beta release** for testing and feedback collection.

**Current Version**: v0.9.3  
**Next Release**: v1.0.0 stable release after beta feedback

### v0.9.x — Beta Validation (Current)

- v0.9.3 (2025-01-15): Authentication error feedback improvement
- v0.9.2 (2025-01-07): Critical bug fix for scene selection preservation
- v0.9.1 (2025-01-07): Bug fixes and streaming sync feature
- v0.9.0 (2025-01-07): Initial beta release
- Ongoing: User feedback collection and improvements

---

### v1.0.0 — Stable Release (Planned)

Core scene switching functionality will be finalized.  
Future development will focus on adding optional extensions while preserving
the existing scene switching behavior.

---

### v1.1.x — Action Foundation (Planned)

- Introduce a unified execution model for handling actions
- Abstract scene switching and external actions using Action Types
- Internal refactoring to allow safe and extensible feature additions

---

### v1.2.x — VTube Studio Integration (Basic) (Planned)

- Connection support for VTube Studio
- Trigger avatar expressions (Expression / Hotkey) via Twitch Channel Point redemptions
- Retrieve and select available expressions from the UI
- One-shot expression execution (no automatic revert)

---

### v1.3.x — Advanced Expression Control (Planned)

- Automatic expression revert after a specified duration
- Conflict suppression during active expression transitions
- Combined actions within a single rule
  - OBS scene switch + VTube Studio expression

---

### Future Considerations

- More flexible action definitions
- Additional streaming-oriented enhancements
- Improvements driven by community feedback

For architectural design principles and versioning rules, see  
[doc/architecture.md](doc/architecture.md) and [doc/versioning.md](doc/versioning.md).

## Developer Information

### Build Instructions

#### Build Plugin

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64 --config Release --parallel
```

#### Build Installer (Requires NSIS)

1. Install [NSIS 3.x (Unicode)](https://nsis.sourceforge.io/)
2. Build the installer:

```powershell
cmake --build build_x64 --target installer --config Release
```

Output: `installer/obs-scene-switcher-{version}-installer.exe`

#### Build Manual Installation ZIP

```powershell
cmake --build build_x64 --target package-zip --config Release
```

Output: `release/obs-scene-switcher-{version}-manual.zip`

For more details, see [doc/architecture.md](doc/architecture.md) and [doc/versioning.md](doc/versioning.md).

## Configuration File Location

Settings are stored in:

```
%APPDATA%\obs-studio\plugin_config\obs-scene-switcher\config.ini
```

## License

GNU General Public License v2.0

Copyright (C) 2025 ksmksks

This software is released under the GPL-2.0 License.
See the [LICENSE](LICENSE) file for details.

## Acknowledgments

This project was developed using [obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate) created by the [OBS Project](https://github.com/obsproject).

## Credits

- **Developer**: ksmksks
- **Twitch**: [https://twitch.tv/ksmksks](https://twitch.tv/ksmksks)
- **X**: [https://x.com/ksmksks](https://x.com/ksmksks)

## Support

For questions or bug reports, please open an [Issue](https://github.com/ksmksks/obs-scene-switcher/issues).

If you find this project useful, please consider starring the repository!

---

**日本語版**: [README_JP.md](README_JP.md) をご覧ください。
