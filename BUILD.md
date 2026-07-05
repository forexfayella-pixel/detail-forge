# Building & Packaging Detail Forge

Detail Forge is a **single cross-platform JUCE 8 codebase**. VST3 + Standalone build everywhere;
**AU builds on macOS only** (JUCE handles this automatically). The UI is one HTML/JS WebView surface
— Windows uses the **WebView2** backend, macOS uses **WKWebView** (chosen at compile time in
`Source/PluginEditor.cpp`).

## No Faust required to build
The Faust-generated engine (`Source/generated/ClipEngine.h`) is **committed**, and the Faust
architecture headers it needs are **vendored** under `ThirdParty/faust/`. So a normal build — and CI —
never depends on a Faust install.

Only if you edit the DSP (`Source/dsp/clip.dsp`) do you need Faust, to regenerate the committed header:
```
cmake --build --preset default --target regen_faust    # requires `faust` on PATH
```
Commit the updated `Source/generated/ClipEngine.h` alongside the `.dsp` change.

---

## Windows (build locally)
**Prereqs:** CMake ≥ 3.25, Visual Studio 2022/2026 (MSVC), and — only for changing DSP — Faust.

```powershell
cmake --preset default
cmake --build --preset default --target DetailForge_VST3   # or DetailForge_All
```
Build output: `build\DetailForge_artefacts\Release\VST3\Detail Forge.vst3`.

### Package (zip + installer)
```powershell
pwsh packaging\package-windows.ps1 -Version 0.1.0
```
This builds Release, validates with **pluginval** (strictness 10), writes a versioned zip to
`packaging\output\`, and — **if Inno Setup is installed** — compiles the installer.

- Install Inno Setup: `winget install JRSoftware.InnoSetup`
- Or compile the installer manually:
  `"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" /DMyAppVersion=0.1.0 packaging\DetailForge.iss`
- The installer places `Detail Forge.vst3` in the shared VST3 folder
  (`C:\Program Files\Common Files\VST3`).

### Optional: sign the Windows installer
Configure a code-signing certificate (or Azure Trusted Signing) and either enable the `SignTool`
directive in `packaging/DetailForge.iss`, or run `signtool sign /fd SHA256 /tr <timestamp-url> ...`
on the produced `.exe`. Not required for the plugin to load.

---

## macOS (via CI — how a Windows dev ships a Mac build)
You **cannot** build/validate/sign a macOS plugin on Windows. The macOS build runs on a GitHub
Actions **macOS runner** (`.github/workflows/macos.yml`).

**Triggers (never on every commit — macOS minutes bill 10× on private repos):**
- pushing a tag `v*` (e.g. `git tag v0.1.0 && git push origin v0.1.0`)
- publishing a GitHub Release
- **Actions → macOS build → Run workflow** (manual dispatch)

The job builds a **universal** (arm64 + x86_64) **AU + VST3 + Standalone**, validates the VST3 with
**pluginval** and the AU with **auval**, then:
- **If Apple signing secrets are set:** code-signs (hardened runtime + `packaging/entitlements.mac.plist`),
  builds a signed **`.pkg`**, notarizes (`notarytool --wait`), and staples it.
- **If not:** uploads **unsigned `.zip`** artifacts so the build is still testable.

Download results from the workflow run's **Artifacts** section.

## macOS (build locally on a Mac with Xcode)
**Prereqs:** Xcode + command line tools, CMake ≥ 3.25.
```bash
cmake -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build --config Release --target DetailForge_All
```
Artefacts land under `build/DetailForge_artefacts/Release/{VST3,AU,Standalone}/`.
Validate: `pluginval --validate "…/Detail Forge.vst3"` and `auval -v aufx Dtfg Fyla`.

---

## Apple prerequisites for signed/notarized Mac builds
You need a paid **Apple Developer account** ($99/yr). From it, obtain:

| Item | What it is |
|------|------------|
| **Team ID** | 10-char team identifier (Apple Developer → Membership) |
| **Developer ID Application** cert | signs the `.vst3` / `.component` / `.app` bundles |
| **Developer ID Installer** cert | signs the distributable `.pkg` |
| **App-specific password** | for `notarytool` (appleid.apple.com → Sign-In & Security) |

Export each certificate (with its private key) from Keychain as a **`.p12`**, then base64-encode it
(`base64 -i cert.p12 | pbcopy`). Add these **GitHub repository secrets**
(Settings → Secrets and variables → Actions):

| Secret | Value |
|--------|-------|
| `DEVELOPER_ID_APP_CERT_P12_BASE64` | base64 of the Application cert `.p12` |
| `DEVELOPER_ID_INSTALLER_CERT_P12_BASE64` | base64 of the Installer cert `.p12` |
| `CERT_PASSWORD` | password used when exporting the `.p12`(s) |
| `APPLE_TEAM_ID` | your Team ID |
| `APPLE_ID` | your Apple ID email |
| `APPLE_APP_PASSWORD` | the app-specific password |

With all of these set, tagging a release produces a **signed, notarized, stapled `.pkg`**. With none
set, the workflow still runs and produces unsigned zips for testing — **no secrets are ever hardcoded.**

---

## Plugin identity (for auval / hosts)
- Manufacturer code: `Fyla` · Plugin code: `Dtfg` · AU type: effect (`aufx`)
- Bundle ID: `com.fayella.detailforge`
- `auval -v aufx Dtfg Fyla`
