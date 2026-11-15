# MineRL: what Whales meant, and what we built

## His intent

Whales built MineRL in ten days in March 2014, as a break from Cataclysm 2,
and described it on his dev blog on 31 March 2014
(whalesdev.tumblr.com/post/81327063452, "Side Project"):

- "a side-view roguelike, [that] takes inspiration from games like
  Boulderdash or any of the dozens of little mining games you can find
  online";
- "dig into the earth to recover valuable resources, like coal, iron, or
  copper", return to the surface, sell them, and buy "ladders, supports,
  dynamite, or crossbow bolts", or upgrade "your stamina level / health,
  pickaxe quality, sword quality, crossbow quality, or lamp (sight range)";
- foes "ranging from the easy cockroach to worms that will build their own
  complex tunnels, termites that will eat your carefully-placed ladder
  system, or goblins that will compete with you for mining resources";
- "not really in a finished state, but I don't much care about it. It's
  just a small one-off side project. It is pretty fun though!", with a plan
  to put it on an SSH server.

## What he left (28 commits, the last on 2014-03-31)

Digging, stamina, falling boulders, ladders and supports, the shop with
upgrades and supplies, cockroaches, worms and termites, coal, iron and
copper. No damage that mattered, no death, no goal, no saving; rope,
dynamite, crossbow bolts, goblins and the use of the sword and crossbow
were listed but not there.

## What we built, and the choices made without him

Everything he listed is now in, and a run has stakes and an end:

- **The goal (our choice):** the Heartstone, a single great stone in a
  cave at the bottom of the claim; carrying it to the surface wins. He
  named no goal; a find at the bottom suits the Boulder Dash and
  Motherlode lineage he named, and gives "go deeper" a reason.
- **Health:** he wrote "stamina level / health" as one upgrade, and his
  `take_damage` lowered the stamina ceiling, so health is that ceiling, as
  he had it. It drops from monster bites, falls of more than four tiles
  (6 per tile after that: our numbers), a boulder landing on you (20) and
  exhaustion (out of stamina underground, effort costs health); the
  shop's doctor restores it, as his upgrade screen already did.
- **The deep (our numbers):** dirt turns to stone from depth 28; silver,
  gold, rubies and diamonds below his coal, iron and copper; bedrock walls
  and floor so nobody digs off the map.
- **His listed tools:** crossbow bolts (6 damage per crossbow level),
  dynamite (3-turn fuse, radius 2, 30 to monsters and 35 to you), rope let
  down a shaft. The sword now does 3 per level and kills count.
- **Goblins:** dig with tools and carry off the ore they reach, as he
  described; they come out below depth 48. Termites also eat rope.
- **Saving, a title screen, help, a run summary and scores.** Messages
  no longer stop the game for Space.

What we did not do: the SSH server (Asciiverse installs it locally), worms
building "complex tunnels" beyond what his digging AI already does.
