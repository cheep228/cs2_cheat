# cs2_linux_skinchanger

A skin changer for the **native Linux** build of CS2 (not CS2-under-Proton —
that process is `cs2.exe` and belongs to the Windows half of this repo).

## What this actually does, and does not do

It rewrites the "fallback" fields of a weapon's `C_EconItemView` inside the
game's own memory, and marks the item as not a real Steam economy item. CS2
then renders it from those fallback fields instead of looking the item up in
your (real) inventory — the same mechanism the game itself uses for preview
tooling. The result:

- Your weapon **looks** like it has the skin, to you, right now.
- Your **real Steam inventory is completely untouched.** This cannot add an
  item to it, trade it, or make it show up on your profile — that data lives
  on Valve's servers and only Valve's backend can write to it. Nothing that
  runs on your own machine can "add a skin to your inventory" in the literal
  sense; any claim otherwise is either lying to you or describing fraud
  against Steam, neither of which this does.
- It is local-only: on a VAC-secured server other players do not see it
  (and if it were ever made to replicate, that would be a much more easily
  detected and heavily punished form of cheating — this project does not
  attempt that).
- It is still a ToS violation and a VAC-ban risk like any other memory patch.
  See the warnings already given in chat before this was written.

## What's real here vs. what you still have to supply

Everything that can be found by *asking the running game* is implemented and
works the same way the Windows half of this repo does it: the item-schema
field offsets (`m_nFallbackPaintKit`, `m_iItemIDHigh`, ...) are looked up by
class+field name from CS2's own live schema system at `src/fields.cpp`, never
hardcoded. That part needs no per-build maintenance.

Three raw pointers cannot be found that way, because they are not schema
fields — they are static globals inside `libclient.so` (the entity list, the
local player pawn, the local player controller), only findable by recognising
specific machine code the Linux build's own compiler emitted. I have no Linux
CS2 binary available to scan honestly, and I am not going to invent byte
patterns for this — a wrong one does not fail loudly, it quietly corrupts
memory or crashes your game. See `src/offsets.h`: all three start at `0`, and
the skin changer logs a clear warning and stays idle until you fill them in.

### Getting the three globals

Pick one:

1. **A CS2-aware dumper you already trust.** `update_offsets.py` at the repo
   root already depends on `a2x/cs2-dumper`'s output for the Windows side;
   that same project can be run against your own `game/bin/linuxsteamrt64/`
   install to produce Linux offsets. Take `dwEntityList`,
   `dwLocalPlayerPawn`, `dwLocalPlayerController` (all *relative to
   `libclient.so`'s base*) from its output and paste them into
   `src/offsets.h`.
2. **Your own disassembler (Ghidra/IDA) against your own `libclient.so`.**
   Same three symbols, same convention (offset from module base, not an
   absolute address).

Either way, this is a one-time step per CS2 build; a game update can move
them and you'd repeat it.

## Building

```sh
cd linux
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Produces `build/libcs2_skinchanger.so`.

## Running it

Native Linux CS2 is launched directly (no Proton), so `LD_PRELOAD` works the
normal Linux way. In Steam, CS2's launch options:

```
LD_PRELOAD=/full/path/to/libcs2_skinchanger.so %command%
```

Launch the game from Steam as usual. Logging goes to stderr — run Steam from
a terminal first if you want to see it, or check `~/.local/share/Steam/...`
for wherever the game's own stdout/stderr ends up on your setup.

## Configuring skins

Copy `config/skins.example.cfg` to
`$XDG_CONFIG_HOME/cs2_linux_skinchanger/skins.cfg`
(or `~/.config/cs2_linux_skinchanger/skins.cfg` if `XDG_CONFIG_HOME` is
unset) and edit it. The file is polled for changes and reloaded automatically
— no need to relaunch the game after changing a skin.

## Known limitations

- Only the locally-controlled player's own weapons are spoofed (active
  weapon + full loadout). Spoofing other players' weapons is a different,
  much more invasive feature and is not implemented.
- If a future CS2 build stops reading the fallback fields every frame (using
  them only once, on pickup), you may need to force a re-equip in-game
  (holster/draw) after changing a config entry for it to refresh. This has
  not been verified against a live build, since I have none to test against
  — said plainly rather than claimed to work.
