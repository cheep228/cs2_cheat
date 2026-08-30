# cs2_overlay

DX11 present-hook overlay DLL for CS2 with a Skeet-style ImGui menu
(RAGE / LEGIT / VISUALS / MISC / SKINS / CONFIG) and player + bot ESP.

> **Injecting this into CS2 violates the Valve Subscriber Agreement.** VAC detects
> third-party modules in the game process; bans arrive in delayed waves, are permanent,
> and take the inventory with them. Use only on an account you are prepared to lose.

## Layout

| File | Purpose |
| --- | --- |
| `src/dllmain.cpp` | entry point, waits for `client.dll`, owns the unload loop |
| `src/core/hooks.cpp` | DXGI `Present` / `ResizeBuffers` detours, WndProc hook, ImGui bootstrap |
| `src/core/globals.h` | shared state and the config struct |
| `src/sdk/offsets.h` | **build-specific offsets — must be re-dumped every game update** |
| `src/sdk/entity.cpp` | entity-list traversal, guarded memory reads |
| `src/features/esp.cpp` | world-to-screen, boxes, health bars, names, snaplines |
| `src/sdk/weapon.cpp` | active weapon lookup and the per-weapon ballistics table |
| `src/features/autowall.cpp` | damage estimate: falloff, armour, headshot, wall model |
| `src/features/ragebot.cpp` | ragebot orchestrator — owns the order of the pipeline, nothing else |
| `src/features/rage/` | the pipeline stages, one concern per file (see **Ragebot**) |
| `src/sdk/trace.h` | line-of-sight interface — **no backend exists**, every query returns `Unknown` |
| `src/core/config_io.cpp` | versioned config slots under `%TEMP%\4fir\configs` |
| `src/core/settings.cpp` | reads the menu key the loader wrote before injecting |
| `src/ui/skeet.h` / `skeet.cpp` | the whole visual layer: palette, chrome, and every widget drawn through `ImDrawList` |
| `src/ui/tab_*.cpp` | tab contents, one file per main tab — no style code lives here |
| `src/ui/menu.cpp` | window chrome and tab dispatch, and nothing else |

## Build

CMake is **not** required. `build.bat` drives `cl.exe` directly and only needs a
Visual Studio C++ toolset (built here with VS 18 Community, MSVC 14.51).

```bash
build.bat
```

Output: `build/cs2_overlay.dll` (x64).

Dependencies are **vendored** in `deps/`, not cloned. Do not run `git clone`
over them: `deps/imgui_nl` is the customized ImGui 1.87 tree supplied with the
Neverlose source and is the active UI dependency. Re-cloning/replacing it would
throw away the literal widget implementations. The dependency mapping and the
small compatibility/branding differences are recorded in
[`deps/UPSTREAM.md`](deps/UPSTREAM.md).

A `CMakeLists.txt` is also included for anyone who has CMake and prefers it
(`cmake -B build -A x64 && cmake --build build --config Release`).

## Offsets resolve themselves at runtime

`src/sdk/offsets.h` is now only a **fallback**. At load time `resolve::Run()` reads
the real values out of the running game, so a game update no longer breaks the ESP:

- **Schema fields** (`m_iHealth`, `m_hPlayerPawn`, …) come from Source 2's own schema
  system — `CreateInterface("SchemaSystem_001")` in `schemasystem.dll`, then the
  `client.dll` type scope, then its class bindings hash table. This is exact, not a
  guess: the same data a2x/cs2-dumper reads from the outside.
- **client.dll globals** (`dwEntityList`, `dwViewMatrix`, …) come from a signature
  scan of `.text`. The patterns are ported from cs2-dumper's `src/analysis/offsets.rs`
  so they track a maintained source.

Anything that fails to resolve falls back to the baked value and says so loudly.
The console prints every offset with `(matches baked)` or `(baked was 0x… - CORRECTED)`,
and the Main tab shows `globals N/5, fields N/10`.

Pressing **F9** also writes two files next to the log:

| File | Contents |
| --- | --- |
| `%TEMP%\cs2_overlay_offsets.txt` | every resolved offset, ready to paste back |
| `%TEMP%\cs2_overlay_schema.txt` | full field list of the classes the ESP touches |

`update_offsets.py` is still useful for refreshing the fallbacks, but it is no longer
required for the ESP to work after a patch.

## Offsets

`src/sdk/offsets.h` holds a snapshot that is almost certainly stale. Regenerate with a
dumper such as [`a2x/cs2-dumper`](https://github.com/a2x/cs2-dumper) and replace:

- `offsets::dwEntityList`, `dwLocalPlayerPawn`, `dwViewMatrix` — from `client_dll` offsets
- everything in `schema::` — from the schema dump (`C_BaseEntity`, `CGameSceneNode`,
  `CCSPlayerController`, `CBasePlayerController`)

The **Main** tab shows whether `client.dll` resolved and whether the entity-list read
looks sane — if it says `invalid`, the offsets are wrong.

## Usage

1. Launch CS2, get into a map.
2. Inject `cs2_overlay.dll` with the bundled injector, from an **administrator**
   prompt in the `build` directory:

   ```
   tools\build_inject.bat      (once)
   build\inject.exe
   ```

   Third-party tools with "stealth" options — Extreme Injector's manual map,
   erase PE header, randomize header — return NULL on modern Windows x64 and
   report *the injection method used returned NULL*. Use plain `LoadLibrary`.
3. **INSERT** toggles the menu, **END** unloads the module cleanly.

`inject.exe --selftest` loads the payload into the injector's own process. If it
succeeds, the image and every import are fine and a failed injection is the
injector, Defender, or VAC — not the DLL.

Run the game in **windowed** or **borderless** mode — exclusive fullscreen makes the
overlay invisible on some drivers.

## Debug console

The payload opens a console the moment it is injected and narrates every step:
module wait, MinHook status per detour, the first intercepted `Present`, device /
window / backbuffer, and then the ESP state. Nothing inside the render loop logs
per frame — messages fire on state changes, or at most once every few seconds.

Every line is mirrored to `%TEMP%\cs2_overlay.log`, so a crash or a forced exit
still leaves something to read.

A healthy start looks like this:

```
ok    core    client.dll   : 0x7FF8F9140000
ok    hook    detours armed, waiting for the first Present
ok    imgui   device 0x...  context 0x...
ok    imgui   window 0x...  class 'SDL_app'
info  d3d11   backbuffer 2560x1440  windowed=1  format=28
ok    imgui   overlay online
ok    render  first frame drawn through the hook
ok    esp     entityList=ok  localPawn=ok  viewMatrix=ok
info  esp     live players: 9 (humans 1 / bots 8), drawn 8
```

Stale offsets look like this instead:

```
ERROR esp     dwEntityList is wrong for this build - run update_offsets.py
warn  mem     617 faulting reads (+616 in 3.0 s) - offsets stale for this build
```

**Do not close the console with its X button** — it belongs to the game process
and closing it kills the game. The close item is removed from its system menu for
that reason; hide it from **Misc → Show console window** instead. That tab also
carries a verbosity toggle, a minimum-level filter and the faulting-read counter.

## Test harness

CS2 is not required to check that the console, hooks and ImGui all work.
`build_test.bat` produces a host that creates a real D3D11 swapchain, loads a stub
named `client.dll`, injects the payload and presents for eight seconds:

```bash
build.bat && build_test.bat && build\test\host.exe
```

The ESP will report failing reads against the stub — that is the expected result,
and it proves the SEH guard holds instead of crashing the host.

## Menu

The menu is not styled ImGui — it is drawn. `src/ui/skeet.h` defines the palette
(`#101010` window, `#151515` groups, `#333333` borders, `#00C8A0` accent) and every
control is rendered through `AddRect` / `AddRectFilled` / `AddLine` / `AddText`.
ImGui keeps only the parts that are genuinely hard to redo — item ids, hover and
active state, `ButtonBehavior`, `SliderBehavior` — so keyboard navigation and
click semantics stay correct while nothing carries the default look.

| Widget | Notes |
| --- | --- |
| `skeet::Checkbox` | 9 px square, accent fill when checked, label to the right |
| `skeet::SliderFloat` / `SliderInt` | 7 px track, value right-aligned on the label line |
| `skeet::Combo` / `MultiCombo` | square box with a small arrow; the popup is drawn in the same style |
| `skeet::Keybind` | click then press any key; right-click clears the bind |
| `skeet::ColorPicker` | hand-drawn SV square, hue strip and alpha strip |
| `skeet::BeginGroupBox` | caption sits inside the top border; height measures itself |
| `skeet::Tab` / `SubTab` | text buttons, accent plus an underline when active |

Style and content are kept apart on purpose: `skeet.cpp` never reads game state and
`tab_*.cpp` never draws chrome.

## Ragebot

`features/ragebot.cpp` owns the **order** of the pipeline and delegates every step:

| Step | Lives in |
| --- | --- |
| weapon group and its settings | `rage/weapon_settings.cpp` |
| collect and filter enemies (dead, dormant, team, FOV) | `rage/target_selector.cpp` |
| build the allowed hitbox list | `rage/hitbox_scanner.cpp` |
| generate aim points per hitbox | `rage/point_scanner.cpp` |
| damage and the minimum-damage rule | `rage/damage.cpp` |
| hitchance | `rage/hitchance.cpp` |
| score points, pick the best target | `rage/target_selector.cpp` |
| auto stop, auto scope, auto fire, angle write | `rage/fire.cpp` |

Two ordering rules matter. Nothing is chosen before every collected enemy has been
fully scanned, so *Highest damage* can compare real damage instead of taking the
first acceptable player. And headshot-only / force-body are applied while the
hitbox list is being built, not by scanning everything and discarding the results.

Settings are per weapon group (Global, Pistols, Heavy pistols, SMG, Rifles,
Shotguns, Snipers, Auto snipers) in one `cfg::RageWeaponConfig` array — there is no
`rifle_hitchance` / `pistol_hitchance` sprawl. A disabled group falls back to
Global rather than switching the ragebot off.

### What is exact and what is modelled

| Feature | Status |
| --- | --- |
| Minimum damage, override key | exact — range falloff, armour, helmet and headshot multiplier from the published weapon values plus the target's live armour. Values above 100 mean "enemy HP + the excess" |
| Autowall / penetration | **modelled, not traced.** No ray trace exists in a read-only overlay, so the gate applies the damage a shot keeps after N assumed walls. It never claims a wall is there — it only makes the gate conservative |
| Hitchance | estimated from `m_fAccuracyPenalty`, your speed and whether you are grounded, against the hitbox's angular size. Not `rand() % 100`, and not the engine's own spread tables either |
| Multipoint | head gets 4 points, torso 3, limbs 1, laid out in a frame built from the line of sight. Scales are fractions of a nominal hitbox radius — CS2 keeps the real hitbox set in the model data |
| Visibility | **absent.** `sdk/trace.h` returns `Unknown` for every query and the scorer awards no visibility bonus rather than inventing one |
| Auto stop | runs only once a point is chosen. `Normal` gates the shot, `Full` also injects W/A/S/D key-**ups** — it can never press a key |
| Auto scope, auto fire | synthetic mouse input via `SendInput`, rate limited per weapon class |
| Anti-aim, double tap, skin changer | **not implemented, and not faked.** All three need systems this project does not have (usercmd control, an econ layer); the ANTI-AIM and SKINS tabs say so instead of showing dead toggles |

The **RAGE → GENERAL → Status** panel exists to answer "why is it not shooting?":
stage, weapon group in force, enemies seen and how many passed the FOV, points
tested and passed, the chosen hitbox, damage against required damage, hitchance
against required hitchance, and a final `VALID` / `REJECTED - DAMAGE` /
`REJECTED - HITCHANCE` verdict.

Weapons are identified by `m_iItemDefinitionIndex`, which does not move between
builds. Everything else the ragebot reads (`m_pWeaponServices`, `m_hActiveWeapon`,
`m_bHasHelmet`, `m_fAccuracyPenalty`, …) is resolved live through the schema
system like the rest of the project.

## Notes

- Bots are separated from humans by `CBasePlayerController::m_steamID == 0`; they get
  their own toggles and colour in **Visual**.
- Box height assumes a 72-unit standing hitbox; crouching players get a slightly tall box.
- `Team check` hides teammates. Turn it off in deathmatch/bot ranges to see everyone.


## Font fallback

When the original Neverlose Museo Sans/icon font assets are unavailable, the UI now uses Windows system fonts without bundling font files: Segoe UI Regular for normal text, Segoe UI Semibold for panel headings, and Segoe UI Bold for the large logo/title slot. Arial is only a last-resort fallback.
