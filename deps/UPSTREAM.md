# Vendored dependencies

Both trees under `deps/` are **vendored**, not fetched. Do not clone over them
and do not re-add `FetchContent` to `CMakeLists.txt`: `deps/imgui` is a fork,
and any mechanism that repopulates it silently deletes the port.

## deps/imgui — FORK

| | |
| --- | --- |
| Upstream | https://github.com/ocornut/imgui |
| Branch | `docking` |
| Commit | `fd13a1e8923a0a7077b404fc36fd063b25a0c0b5` |
| Commit date | 2026-08-19 13:39:46 +0200 |
| Subject | `Merge branch 'master' into docking` |
| Version | `1.93.0 WIP` (`IMGUI_VERSION_NUM` 19294) |
| Contents | complete upstream tree at that commit, minus `.git` |

To reproduce the base this fork started from:

```bash
git clone https://github.com/ocornut/imgui.git
cd imgui && git checkout fd13a1e8923a0a7077b404fc36fd063b25a0c0b5
```

Diffing that checkout against `deps/imgui` gives exactly the port and nothing
else.

### Why it is a fork

The menu is a port of the neverlose CS:GO UI, and that UI is not a widget
library layered on top of ImGui - it *is* ImGui, modified in place. Its
`Checkbox`, `SliderScalar`, `BeginCombo`, `Button`, `Selectable`, `RadioButton`
and `ScrollbarEx` are rewritten bodies of the stock functions, and the window
shape lives inside `RenderWindowDecorations`. Reusing those implementations
means replacing the stock ones, which is only possible in a tree we own.

Every line changed for the port is marked in place. Changes made purely to
reconcile the original 1.87 code with 1.93 carry:

```
// NL PORT: ImGui 1.93 compatibility
```

so a later upstream merge can tell a port decision from a compatibility fix.

## deps/minhook

| | |
| --- | --- |
| Upstream | https://github.com/TsudaKageyu/minhook |
| Branch | `master` |
| Commit | `d94c64d32ea37bc4f5ee47d580709f70c6fb6080` |
| Commit date | 2026-06-13 23:02:43 +0300 |
| Subject | `Fix link to MinHook article in README` |
| Modified | no - pristine upstream, vendored only so the build never reaches the network |
