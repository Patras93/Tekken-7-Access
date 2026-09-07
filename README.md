# Tekken Accessibility

Screen reader support for blind and visually impaired players of **TEKKEN 7 on Windows**.

[Polski](README_PL.md) · [English changelog](CHANGELOG_EN.md) · [Polski changelog](CHANGELOG_PL.md)

**Current version: 2.71 test.** This project adds spoken UI and selected battle information. Full accessibility across every game mode is still in progress.

## Features

The plugin includes readers for:

- Main, offline and online menus; game, sound and graphics options.
- Select Side, Character Select and Stage Select.
- Quick Match and session search options, session results and room player lists.
- Player Match room creation, selected profile information and ranks.
- Practice options and selected command-list entries.
- Standard confirmation dialogs and selected special dialogs.
- Story chapter selection, selected narrative text and QTE prompts.
- Customization categories, outfits, selected items and palette coordinates.
- Selected replay, gallery, tournament and Tekken Bowl information.
- Health thresholds, timer milestones, Rage and round-win markers.
- Treasure Battle box colors and selected result information.

Coverage varies by screen. The presence of a reader does not mean that every action or state in that mode has been verified.

Generated speech is in **English**. Text supplied by the game follows the game's language; player names are preserved. Session and lobby identity announcements are concise, for example: `Alice. Paul. Fujin`.

## Requirements and compatibility

- A Windows x64 installation of TEKKEN 7.
- NVDA and `nvdaControllerClient64.dll`, or an existing compatible Tolk setup. The plugin prefers NVDA when available.
- A working loader for `TekkenAccessibility.dll`. The development installation uses a `dinput8.dll` proxy.

**The 2.71 package is an update to an existing accessibility installation.** It contains the plugin and its source, but does not bundle the loader or third-party speech libraries. Copying this DLL alone into a clean game installation is not a complete first-time setup.

The native addresses were checked against `TekkenGame-Win64-Shipping.exe` with this SHA-256:

```text
7f2b68727023ce9f4554b47f5442dad9dfb38c8a9b778f2aa758469e2c2d60ed
```

Other executable builds have not been verified. Installation checks compare expected hook bytes, but those checks alone do not establish compatibility with another build.

## Install or update

1. Close TEKKEN 7.
2. Open the game's `TekkenGame\Binaries\Win64` folder in its Steam installation directory.
3. Back up the existing `TekkenAccessibility.dll` outside the game folder.
4. Copy the new `TekkenAccessibility.dll` into `Win64`, replacing the old plugin. Keep the existing loader and speech libraries.
5. Start NVDA, then launch the game through your existing working setup.

The startup message is `Tekken Accessibility 2.71 loaded.` If initialization is incomplete, the plugin announces `Partial accessibility. Failed:` followed by the affected modules.

To roll back, close the game and restore your backed-up DLL. To disable this plugin, close the game and move its DLL out of `Win64`.

## Controls

Use the game's normal controls to navigate. Supported selections and value changes are announced automatically.

These additional shortcuts work while the game has focus:

| Shortcut | Action |
| --- | --- |
| Ctrl + Shift + F1 | Read all captured descriptions for the current screen |
| Ctrl + Shift + F2 | Read the next description fragment |
| Ctrl + Shift + F3 | Read the previous description fragment |
| Ctrl + Shift + F4 | Toggle automatic descriptions |
| Ctrl + Shift + F5 | Read the latest captured health values |
| Ctrl + Shift + F6 | Toggle automatic health announcements |
| Ctrl + Shift + F7 | Read the latest captured timer and Rage status |
| Ctrl + Shift + F8 | Toggle timer, Rage and round announcements |
| Ctrl + Shift + F9 | Read captured rooms on the current results page |

These shortcuts read captured information; they do not scan unseen pages. Older health and timer snapshots identify their age. Speech rate follows your screen reader settings.

## Changes in 2.71

- Identical Treasure Battle box colors can be announced again on consecutive results; colors from one result are grouped together.
- Rank-gauge speech uses the displayed player's record and resets when initialized or closed.
- Health, HUD and rank messages are appended after the primary announcement instead of being dropped by the worker's priority handling.
- A failed room-creation module no longer prevents the shared speech worker from starting. Startup reports partial initialization.
- Remaining generated Polish messages were translated into English and some were shortened.

See [CHANGELOG_EN.md](CHANGELOG_EN.md) for version history.

## Known limitations

- Complete accessibility has not been verified across all modes, screens and transitions.
- Player Match readiness, interrupted countdowns and match starts still need testing with another player.
- A character portrait in an online list does not necessarily identify the final character selected for the next match.
- Practice input symbols are not fully verified as readable button and direction instructions.
- The customization palette reports positions rather than color names or RGB values.
- Story text and selected QTE support do not provide full cutscene audio description or guarantee that every subtitle is read.
- Rapid navigation intentionally interrupts older speech. Full speech ordering during live gameplay still needs validation.

## Build from source

Install Visual Studio or Build Tools with C++ x64 tools and the Windows SDK. Keep `TekkenAccessibility.cpp`, all headers, test sources and `BUILD_X64.bat` together, then run:

```bat
BUILD_X64.bat
```

The script builds and runs the tests, then compiles `TekkenAccessibility.dll` with MSVC and C++17. It does not install the DLL into the game.

For the prepared 2.71 build, 14 test executables passed and 45 hook prologues matched the checked executable. These checks cover logic and binary assumptions, not a full playthrough with NVDA. See `TEST_RESULTS.txt`, `HOOK_AUDIT.txt` and `NATIVE_AUDIT.txt`.

## Report an issue

Include the plugin version, game language, screen reader, menu path, exact steps, expected announcement and what was actually spoken. Mention whether the problem happens on first entry, after returning to the screen, or during rapid navigation.

For example: `2.71, NVDA, English game. Offline > Treasure Battle. On the second consecutive result, the first Gold box was not announced.`

For startup problems, include any `Partial accessibility` message and, if your loader provides it, relevant lines from `TekkenAccessibilityLoader.log`. Remove private information before sharing logs.
