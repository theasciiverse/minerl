# Changes from upstream

This is the Asciiverse edition of [MineRL](https://github.com/Whales/minerl)
by Whales (2014), GPL-2.0 (`LICENSE`). Upstream is the `upstream` remote;
the `asciiverse` branch starts from upstream's `master` (its 28 commits,
last on 2014-03-31) and finishes the game. VISION.md says what Whales
meant and which choices were made without him.

## The game (commit "A whole game", 2026-11-15)

- `tiledata.h`, `globals.cpp`, `map.cpp`: stone (from depth 28, more with
  depth), silver, gold, ruby, diamond, bedrock walls and floor, rope, and
  the Heartstone in a cave at the bottom.
- `player.cpp`: falls of five tiles or more hurt; out of stamina
  underground, effort costs health; `take_damage` records the cause;
  `dead()`. The sword does three damage per level, kills are counted and
  reported.
- `map.cpp`: monsters come out near you, worse the deeper (cockroaches and
  worms, termites below depth 23, goblins below depth 48); boulders landing on you
  hurt; dynamite (`Bomb`, fuse, `explode`); rope let down a shaft
  (`add_rope`); messages drawn without stopping the game (they waited for
  Space); saving and loading the claim.
- `monster.cpp`: goblins (dig with tools, carry off ore); termites eat
  rope; bites are reported.
- `main.cpp`: a title screen, help, the crossbow (f), dynamite (d), rope
  (r), a supply counter for all six supplies, the HUD's health, depth,
  cash and goal lines, winning by bringing the Heartstone up, death and a
  run summary, scores, saving (S, every 25 turns, Continue).
- Fixed: two loops that read an uninitialised key (`ch`) before the
  first input.
- Fixed (`window.cpp`): popups measured their width with the colour tags
  counted, so a long coloured message could open off the screen and never
  show; measured on the plain text now, at most 80 wide.

## Folders and logging

- The screens (`cuss/`) are read beside the program, wherever it starts;
  the save, scores and log go to `$XDG_STATE_HOME/minerl` (or
  `~/.local/state/minerl`, or Application Support on macOS).
- `MINERL_LOG=1` writes every key with the turn, position, stamina,
  health and cash to `minerl.log` there.

## Packaging (new files)

- `asciiverse/build.sh`, `asciiverse/package.sh`, `asciiverse/README.txt`,
  `asciiverse/record.sh` and the preview `asciiverse/minerl.cast`.
- `.github/workflows/release.yml`: static Linux (Alpine, musl, x86-64 and
  ARM64) and macOS (Intel and Apple silicon) archives and SHA256SUMS on a
  tag.
