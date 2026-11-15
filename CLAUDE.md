# minerl

Whales' side-view mining roguelike, "inspired by Boulderdash and
Motherlode" (github.com/Whales/minerl, C++ on Whales' `cuss` curses
toolkit, 28 commits, 2014-03-22 to 2014-03-31), GPL-2.0 (`LICENSE`, the
GPL version 2 text; no file headers, so GPL-2.0-only as shipped).

**Status (2026-11-13): evaluated, not packaged.** A prototype: it builds
unchanged on today's Linux and runs, but it is not yet a game.

## What exists (built and played 2026-11-13)

- A 60x24 side view: a surface line with a shop (`=`) and the player,
  layers of soil, rock and minerals below; a 20-column HUD (stamina,
  storage, ladders, rope, supports).
- Digging with hjkl or the arrows costs stamina; the lamp reveals a radius
  around you; loose tiles fall (`process_falling`); monsters spawn every
  50 turns (and on `!`) and move towards you.
- Underground, `w` places a ladder and `s` a support from your supplies.
- At the shop (`a` on its tile) findings sell by tile value, and upgrades
  can be bought: stamina, backpack, pickaxe, sword, crossbow, lamp; then
  supplies. Back on the surface stamina refills.
- About 7,600 lines in all, much of it the shared `cuss` toolkit.

## What is missing

- **No stakes:** no death or damage from monsters or falls, no stamina
  running out mattering, no ending or goal; `q` quits.
- No saving; no depth progression (deeper layers, harder rock, rarer ore
  at depth are only partly there); the sword and crossbow upgrades have no
  combat to use them in; rope is never used.
- No help screen, no title screen.

## Finishing plan (for a night of its own)

1. Stakes: health separate from stamina, monster attacks and fall damage,
   death and a run summary; stamina out means a slow, dangerous climb.
2. Depth: layers with harder rock and better ore further down, deeper
   monsters, a goal (a fixed depth artefact, or a money target before a
   day count) so a run ends in a win.
3. Combat with the sword and crossbow, rope for long falls and shafts.
4. Title, help, high scores; saving between dives; resize handling.
5. Package per conventions (theasciiverse/minerl, state folder, preview).

Licence is settled (GPL-2.0); the name is Whales' own and can stay.
