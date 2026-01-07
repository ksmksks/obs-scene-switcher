# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

---

## [0.9.2] - 2025-01-07

Critical bug fix.

### Fixed
- **Scene selection preservation**: Rule settings are now correctly preserved when opening settings window after adding/removing scenes in OBS
  - Scene selections (source and target) are saved before updating the list
  - Selections are restored after combo boxes are rebuilt
  - Prevents unintended data loss during scene modifications

---

## [0.9.1] - 2025-01-07

Bug fixes and UI improvements.

### Fixed
- Scene list now updates dynamically when settings window is opened
- Japanese locale: Changed "秒数" (seconds count) to "秒" (sec) for duration label
- Japanese locale: Fixed "削除" (Remove) button text encoding

### Added
- Automatic enable/disable sync with OBS streaming state
  - Plugin automatically enables when streaming starts (if authenticated)
  - Plugin automatically disables when streaming stops

---

## [0.9.0] - 2025-01-07

Initial public beta release.

### Added
- Twitch Channel Point–triggered OBS scene switching
- Rule-based evaluation with deterministic priority order
- Scene restore timers and suppression control
- Explicit enable/disable lifecycle
- Stable OAuth and EventSub (WebSocket) integration
- Windows installer (NSIS) with OBS auto-detection
- Manual installation ZIP package
- Comprehensive UML diagrams (7 diagrams) documenting architecture
- Complete architecture specification (doc/architecture.md)
- Version policy and development roadmap (doc/versioning.md)
- Internationalization support (English/Japanese)

### Documentation
- Initial public documentation (English/Japanese)
- Installation and onboarding guides
- Twitch Developer Console setup guide
- Troubleshooting guide
- UML diagram generation guide (uml/README.md)

### Notes
- This is a beta release for testing and feedback collection
- Bug fixes and improvements will be released in v0.9.x series
- Stable v1.0.0 release planned after beta validation period
- Please report issues at: https://github.com/ksmksks/obs-scene-switcher/issues

[Unreleased]: https://github.com/ksmksks/obs-scene-switcher/compare/v0.9.2...HEAD
[0.9.2]: https://github.com/ksmksks/obs-scene-switcher/compare/v0.9.1...v0.9.2
[0.9.1]: https://github.com/ksmksks/obs-scene-switcher/compare/v0.9.0...v0.9.1
[0.9.0]: https://github.com/ksmksks/obs-scene-switcher/releases/tag/v0.9.0
