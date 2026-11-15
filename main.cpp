#include "window.h"
#include "globals.h"
#include "map.h"
#include "player.h"
#include "rng.h"
#include "files.h"
#include "stringfunc.h"
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

void sell_findings(Player &pl);

void shop_upgrades(Player &pl);
void update_upgrade_ui(cuss::interface &i_shop, Player &pl);

void shop_supplies(Player &pl);
void update_supply_ui(cuss::interface &i_shop, Player &pl);

// ---------------------------------------------------------------------
// Asciiverse: folders, the session log, saving, the title screen, help,
// and the run's end. Whales' game loop is below, grown with the stakes,
// the deep and the Heartstone.

static std::string STATE_DIR = "save/";
static FILE* LOG = NULL;

static std::string exe_dir()
{
  char buf[PATH_MAX];
#ifdef __APPLE__
  uint32_t size = sizeof(buf);
  if (_NSGetExecutablePath(buf, &size) != 0) {
    return "";
  }
  char real[PATH_MAX];
  if (!realpath(buf, real)) {
    return "";
  }
  std::string p(real);
#else
  ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (n <= 0) {
    return "";
  }
  buf[n] = 0;
  std::string p(buf);
#endif
  size_t slash = p.rfind('/');
  return (slash == std::string::npos ? "" : p.substr(0, slash));
}

static void make_dirs(const std::string& path)
{
  for (size_t i = 1; i <= path.size(); i++) {
    if (i == path.size() || path[i] == '/') {
      mkdir(path.substr(0, i).c_str(), 0755);
    }
  }
}

// The screens (cuss/) are read beside the program, wherever it is started;
// the save, the scores and the log live in the player's own folder:
// $XDG_STATE_HOME/minerl (usually ~/.local/state/minerl), or
// ~/Library/Application Support/minerl on macOS.
static void set_directories()
{
  std::string dir = exe_dir();
  if (!dir.empty() && directory_exists(dir + "/cuss")) {
    if (chdir(dir.c_str()) != 0) {
      return;
    }
  }
  const char* home = getenv("HOME");
  std::string state;
#ifdef __APPLE__
  if (home) {
    state = std::string(home) + "/Library/Application Support/minerl";
  }
#else
  const char* xdg = getenv("XDG_STATE_HOME");
  if (xdg && *xdg) {
    state = std::string(xdg) + "/minerl";
  } else if (home) {
    state = std::string(home) + "/.local/state/minerl";
  }
#endif
  if (!state.empty()) {
    make_dirs(state);
    STATE_DIR = state + "/";
  } else {
    make_dirs("save");
  }
  const char* logvar = getenv("MINERL_LOG");
  if (logvar && *logvar) {
    LOG = fopen((STATE_DIR + "minerl.log").c_str(), "a");
  }
}

static void logf(const char* fmt, ...)
{
  if (!LOG) {
    return;
  }
  va_list ap;
  va_start(ap, fmt);
  vfprintf(LOG, fmt, ap);
  va_end(ap);
  fputc('\n', LOG);
  fflush(LOG);
}

static std::string save_path()  { return STATE_DIR + "claim.sav"; }
static std::string score_path() { return STATE_DIR + "scores.txt"; }

static void save_game(Map& map, Player& pl, int turn)
{
  std::ofstream out(save_path().c_str());
  out << "minerl-save 1\n" << turn << "\n";
  out << pl.posx << " " << pl.posy << " " << pl.deepest_point << " "
      << pl.max_stamina << " " << pl.cash << " " << (pl.has_heart ? 1 : 0)
      << " " << pl.kills << "\n";
  for (int i = 0; i < E_max; i++) {
    Equipment_status& e = pl.equipment[i];
    out << e.current << " " << e.level << " " << e.price << " "
        << e.price_increase << " " << e.upgrade_amount << "\n";
  }
  for (int i = 0; i < S_max; i++) {
    out << pl.supplies[i] << " ";
  }
  out << "\n" << pl.findings.size();
  for (int i = 0; i < pl.findings.size(); i++) {
    out << " " << int(pl.findings[i]);
  }
  out << "\n";
  map.save(out);
  logf("saved turn %d", turn);
}

static bool load_game(Map& map, Player& pl, int& turn)
{
  std::ifstream in(save_path().c_str());
  std::string tag;
  int version;
  if (!(in >> tag >> version) || tag != "minerl-save") {
    return false;
  }
  int heart;
  in >> turn >> pl.posx >> pl.posy >> pl.deepest_point >> pl.max_stamina
     >> pl.cash >> heart >> pl.kills;
  pl.has_heart = (heart != 0);
  for (int i = 0; i < E_max; i++) {
    Equipment_status& e = pl.equipment[i];
    in >> e.current >> e.level >> e.price >> e.price_increase
       >> e.upgrade_amount;
  }
  for (int i = 0; i < S_max; i++) {
    in >> pl.supplies[i];
  }
  int n;
  in >> n;
  pl.findings.clear();
  for (int i = 0; i < n; i++) {
    int id;
    in >> id;
    pl.findings.push_back(Tile_id(id));
  }
  if (in.fail()) {
    return false;
  }
  return map.load(in);
}

struct Score { int cash, depth, turn; bool won; std::string how; };

static std::vector<Score> read_scores()
{
  std::vector<Score> ret;
  std::ifstream in(score_path().c_str());
  Score s;
  int won;
  while (in >> s.cash >> s.depth >> s.turn >> won) {
    std::getline(in, s.how);
    s.how = trim(s.how);
    s.won = (won != 0);
    ret.push_back(s);
  }
  return ret;
}

static void add_score(Score s)
{
  std::ofstream out(score_path().c_str(), std::ios::app);
  out << s.cash << " " << s.depth << " " << s.turn << " " << (s.won ? 1 : 0)
      << " " << s.how << "\n";
}

static void show_help()
{
  popup_scrollable(
"<c=yellow>MINERL<c=/>\n"
"\n"
"Somewhere at the bottom of this claim lies the <c=pink>Heartstone<c=/>, the\n"
"great stone every miner in the valley has heard of and nobody has seen.\n"
"Dig down, sell what you find at the shop (<c=blue>=<c=/>), buy better tools,\n"
"and go deeper, until you can bring the Heartstone up into the daylight.\n"
"\n"
"<c=white>Moving and digging<c=/>\n"
"  arrows or hjkl   walk; walk into ground to dig it\n"
"  .                wait a turn\n"
"  a                at the shop: sell your findings and shop\n"
"\n"
"<c=white>Underground<c=/>\n"
"  w   put down a ladder (climb up on ladders, rope and supports)\n"
"  s   put up a support (stops boulders from falling)\n"
"  r   let a rope down from where you stand\n"
"  f   fire the crossbow (then a direction); needs bolts\n"
"  d   light a stick of dynamite (it blows in three turns: run!)\n"
"\n"
"<c=white>Staying alive<c=/>\n"
"  Digging and climbing cost stamina. Out of stamina, every effort costs\n"
"  health instead. Stamina comes back on the surface; health only at the\n"
"  shop's doctor (h in the upgrade screen).\n"
"  Falls of more than four tiles hurt. Boulders (0) fall when you dig\n"
"  under them. Cockroaches and worms live near the top, termites eat your\n"
"  ladders, and goblins in the deep dig for the same ore you do.\n"
"\n"
"<c=white>The ground<c=/>\n"
"  Dirt gives way to stone the deeper you go. Coal, iron and copper near\n"
"  the top; silver and gold further down; rubies and diamonds in the\n"
"  deep. Dig your way into the mine with your auger (one comes with the\n"
"  claim; buy another at the shop to open a second entrance).\n"
"\n"
"  S   save and quit          ?   this help\n"
"\n"
"Press Esc to close.");
}

static long title_screen()
{
  Window w(0, 0, 80, 24);
  std::vector<Score> scores = read_scores();
  bool saved = file_exists(save_path());
  while (true) {
    w.clear();
    const char* art[] = {
      "  __  __  ___  _  _  ___  ___  _    ",
      " |  \\/  ||_ _|| \\| || __|| _ \\| |   ",
      " | |\\/| | | | | .` || _| |   /| |__ ",
      " |_|  |_||___||_|\\_||___||_|_\\|____|",
      NULL };
    for (int i = 0; art[i]; i++) {
      w.putstr(20, 2 + i, c_yellow, c_black, art[i]);
    }
    w.putstr(17, 8, c_ltgray, c_black,
             "Dig for the Heartstone at the bottom of the claim.");
    int y = 11;
    w.putstr(30, y++, c_white, c_black, "n  New claim");
    if (saved) {
      w.putstr(30, y++, c_white, c_black, "c  Continue digging");
    }
    w.putstr(30, y++, c_white, c_black, "?  How to play");
    w.putstr(30, y++, c_white, c_black, "q  Quit");
    if (!scores.empty()) {
      int best = 0;
      for (int i = 1; i < scores.size(); i++) {
        if (scores[i].cash > scores[best].cash) {
          best = i;
        }
      }
      int wins = 0;
      for (int i = 0; i < scores.size(); i++) {
        wins += scores[i].won ? 1 : 0;
      }
      w.putstr(18, 18, c_dkgray, c_black,
               "Best run: $" + itos(scores[best].cash) + ", depth " +
               itos(scores[best].depth) + ". Heartstones brought up: " +
               itos(wins));
    }
    w.putstr(19, 22, c_dkgray, c_black,
             "MineRL by Whales, 2014. Finished for Asciiverse.");
    w.refresh();
    long ch = input();
    if (ch == 'n' || ch == 'N') return 'n';
    if ((ch == 'c' || ch == 'C') && saved) return 'c';
    if (ch == '?') { show_help(); continue; }
    if (ch == 'q' || ch == 'Q' || ch == 27) return 'q';
  }
}

static void end_screen(Player& pl, int turn, bool won)
{
  Window w(0, 0, 80, 24);
  w.clear();
  int depth = pl.deepest_point - 12;
  if (won) {
    w.putstr(10, 4, c_pink, c_black,
             "You carry the Heartstone up into the daylight!");
    w.putstr(10, 6, c_ltgray, c_black,
             "The whole valley turns out to see it. The claim was worth it.");
  } else {
    w.putstr(10, 4, c_ltred, c_black, "You died in the mine.");
    w.putstr(10, 6, c_ltgray, c_black,
             "Killed by " + (pl.last_hurt.empty() ? std::string("the deep")
                                                  : pl.last_hurt) + ".");
  }
  w.putstr(10, 9,  c_white, c_black, "Deepest point:  " + itos(depth));
  w.putstr(10, 10, c_white, c_black, "Cash:           $" + itos(pl.cash));
  w.putstr(10, 11, c_white, c_black, "Monsters slain: " + itos(pl.kills));
  w.putstr(10, 12, c_white, c_black, "Turns:          " + itos(turn));
  w.putstr(10, 16, c_dkgray, c_black, "Press Space.");
  w.refresh();
  Score s;
  s.cash = pl.cash;
  s.depth = depth;
  s.turn = turn;
  s.won = won;
  s.how = won ? "the Heartstone" : pl.last_hurt;
  add_score(s);
  unlink(save_path().c_str());
  logf("end won=%d depth=%d cash=%d turn=%d", won ? 1 : 0, depth, pl.cash,
       turn);
  while (input() != ' ');
}

// One direction key, for the crossbow
static bool get_direction(int& dx, int& dy)
{
  long ch = input();
  dx = dy = 0;
  if (ch == KEY_LEFT  || ch == 'h') dx = -1;
  if (ch == KEY_RIGHT || ch == 'l') dx = 1;
  if (ch == KEY_UP    || ch == 'k') dy = -1;
  if (ch == KEY_DOWN  || ch == 'j') dy = 1;
  return dx != 0 || dy != 0;
}

static void draw_hud_extra(Window& w, Player& pl)
{
  int depth = pl.posy - 12;
  nc_color hcol = (pl.max_stamina * 4 < pl.equipment[E_stamina].level ?
                   c_ltred : c_ltgray);
  w.putstr(1, 8,  hcol, c_black, "Health:   " + itos(pl.max_stamina) +
                                 "/" + itos(pl.equipment[E_stamina].level));
  w.putstr(1, 9,  c_ltgray, c_black, "Bolts:    " + itos(pl.supplies[S_bolts]));
  w.putstr(1, 10, c_ltgray, c_black, "Dynamite: " + itos(pl.supplies[S_dynamite]));
  w.putstr(1, 11, c_ltgray, c_black, "Augers:   " + itos(pl.supplies[S_auger]));
  w.putstr(1, 13, c_white,  c_black, "Depth:    " + itos(depth < 0 ? 0 : depth));
  w.putstr(1, 14, c_white,  c_black, "Deepest:  " + itos(pl.deepest_point - 12));
  w.putstr(1, 15, c_yellow, c_black, "Cash:     $" + itos(pl.cash));
  if (pl.has_heart) {
    w.putstr(1, 17, c_pink, c_black, "HEARTSTONE! Up!");
  } else {
    w.putstr(1, 17, c_dkgray, c_black, "Goal: the bottom");
  }
  w.putstr(1, 21, c_dkgray, c_black, "? help  S save");
}

// Returns true when the game ended (death or victory), false when saved
static bool play(Map& map, Player& player, int& turn)
{
  Window w_main(0, 0, 60, 24);
  Window w_hud(60, 0, 20, 24);

  cuss::interface i_hud;
  if (!i_hud.load_from_file("cuss/i_hud.cuss")) {
    return false;
  }
  if (turn == 0) {
    map.add_msg("Dig down: the Heartstone lies at the bottom of the claim.");
  }

  long ch = 0;
  while (true) {
    map.draw(&w_main, &player);
    player.update_hud(i_hud);
    i_hud.draw(&w_hud);
    draw_hud_extra(w_hud, player);
    w_main.refresh();
    w_hud.refresh();
    if (player.dead()) {
      end_screen(player, turn, false);
      return true;
    }
    ch = input();
    logf("turn %d key %ld pos %d,%d stam %d health %d cash %d", turn, ch,
         player.posx, player.posy, player.equipment[E_stamina].current,
         player.max_stamina, player.cash);
    if (ch == 'S' || ch == 'q' || ch == 'Q') {
      save_game(map, player, turn);
      return false;
    }
    if (ch == '?') {
      show_help();
      continue;
    }
    int newx = player.posx, newy = player.posy;
    bool player_took_turn = false;
    if (ch == KEY_LEFT  || ch == 'h')  newx--;
    if (ch == KEY_RIGHT || ch == 'l')  newx++;
    if (ch == KEY_UP    || ch == 'k')  newy--;
    if (ch == KEY_DOWN  || ch == 'j')  newy++;

    if (player.posy > 12) { // Can only use items underground
      Tile_id here = map.get_tile_id(player.posx, player.posy);
      if (ch == 'w' && here != T_ladder) {
        if (player.supplies[S_ladders] > 0) {
          player.supplies[S_ladders]--;
          map.add_ladder(player.posx, player.posy);
          player_took_turn = true;
        } else {
          map.add_msg("Out of ladders!");
        }
      } else if (ch == 's' && here != T_support) {
        if (player.supplies[S_supports] > 0) {
          player.supplies[S_supports]--;
          map.add_support(player.posx, player.posy);
          player_took_turn = true;
        } else {
          map.add_msg("Out of supports!");
        }
      } else if (ch == 'r') {
        if (player.supplies[S_rope] <= 0) {
          map.add_msg("Out of rope!");
        } else {
          int used = map.add_rope(player.posx, player.posy,
                                  player.supplies[S_rope]);
          if (used == 0) {
            map.add_msg("There's nothing to let a rope down into here.");
          } else {
            player.supplies[S_rope] -= used;
            map.add_msg("You let down " + itos(used) + " lengths of rope.");
            player_took_turn = true;
          }
        }
      } else if (ch == 'd') {
        if (player.supplies[S_dynamite] <= 0) {
          map.add_msg("No dynamite!");
        } else {
          player.supplies[S_dynamite]--;
          map.add_bomb(player.posx, player.posy);
          map.add_msg("You light the fuse. Run!");
          player_took_turn = true;
        }
      } else if (ch == 'f') {
        if (player.supplies[S_bolts] <= 0) {
          map.add_msg("No crossbow bolts!");
        } else {
          map.add_msg("Fire which way?");
          map.draw(&w_main, &player);
          w_main.refresh();
          int dx, dy;
          if (get_direction(dx, dy)) {
            player.supplies[S_bolts]--;
            player_took_turn = true;
            int x = player.posx, y = player.posy;
            bool hit = false;
            for (int i = 0; i < 10 && !hit; i++) {
              x += dx;
              y += dy;
              if (map.get_tile_data(x, y)->blocks) {
                break;
              }
              Monster* mon = map.monster_at(x, y);
              if (mon) {
                mon->take_damage(player.equipment[E_crossbow].level * 6);
                if (mon->dead) {
                  player.kills++;
                  map.add_msg("Your bolt kills the " +
                              monster_name(mon->id) + "!");
                } else {
                  map.add_msg("Your bolt hits the " +
                              monster_name(mon->id) + ".");
                }
                hit = true;
              }
            }
            if (!hit) {
              map.add_msg("Your bolt flies off into the dark.");
            }
          }
        }
      }
    }

    if (ch == 'a') {
      if (player.posx == 65 && player.posy == 12) {
        sell_findings(player);
        shop_upgrades(player);
      } else {
        map.add_msg("The shop is the = on the surface.");
      }
    }

    if (newx != player.posx || newy != player.posy) {
      Tile_id target = map.get_tile_id(newx, newy);
      int hearthp = map.get_tile(newx, newy)->hp;
      if (player.move(&map, newx, newy)) {
        player_took_turn = true;
        // Asciiverse: dug through the Heartstone?
        if (target == T_heartstone && hearthp > 0 &&
            map.get_tile_id(newx, newy) != T_heartstone) {
          player.has_heart = true;
          map.add_msg("You have the HEARTSTONE! Now get it to the surface!");
        }
        int rad = player.get_lamp_radius();
        for (int x = player.posx - rad; x <= player.posx + rad; x++) {
          for (int y = player.posy - rad; y <= player.posy + rad; y++) {
            map.get_tile(x, y)->seen = true;
          }
        }
      }
    } else if (ch == '.') {
      player_took_turn = true;
    }

    if (player.posy <= 12) {
      player.reset_values();
      if (player.has_heart) {
        map.draw(&w_main, &player);
        w_main.refresh();
        end_screen(player, turn, true);
        return true;
      }
    }

    if (player_took_turn) {
      turn++;
      map.process_falling(player);
      map.process_bombs(player);
      map.move_monsters(player);
      if (turn % 40 == 0) {
        map.spawn_monsters(player);
      }
      if (turn % 25 == 0) {
        save_game(map, player, turn);  // keep a fresh save
      }
    }
  }
}

int main()
{
  srand(time(NULL));
  set_directories();

  init_display();

  init_data();

  while (true) {
    long choice = title_screen();
    if (choice == 'q') {
      break;
    }
    Map map;
    Player player;
    int turn = 0;
    if (choice == 'c') {
      if (!load_game(map, player, turn)) {
        popup("That save could not be read; starting a new claim.");
        map = Map();
        player = Player();
        turn = 0;
        map.generate();
      }
    } else {
      map.generate();
    }
    play(map, player, turn);
  }

  end_display();
  if (LOG) {
    fclose(LOG);
  }
  return 0;
}

void sell_findings(Player &pl)
{
  for (int i = 0; i < pl.findings.size(); i++) {
    pl.cash += TILES[pl.findings[i]].value;
  }
  pl.findings.clear();
}

void shop_upgrades(Player &pl)
{
  Window w_shop(0, 0, 80, 24);
  cuss::interface i_shop;
  i_shop.load_from_file("cuss/i_upgrade.cuss");

  update_upgrade_ui(i_shop, pl);

  while (true) {
    i_shop.draw(&w_shop);
    w_shop.refresh();
    long ch = input();

    if ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) {
      Equipment bought;
      if (ch >= 'a' && ch <= 'f') {
        bought = Equipment( ch - 'a' + 1 );
      }
      if (ch >= 'A' && ch <= 'F') {
        bought = Equipment( ch - 'A' + 1 );
      }
      Equipment_status* stat = &(pl.equipment[bought]);
      if (pl.cash >= stat->price) {
        pl.cash -= stat->price;
        stat->set_level( stat->level + stat->upgrade_amount );
        stat->price += (stat->price * stat->price_increase) / 100;
        update_upgrade_ui(i_shop, pl);
      }
    } else if (ch == 'h' || ch == 'H') {
      int needed_healing = pl.equipment[E_stamina].level - pl.max_stamina;
      int heal_cost = 2 * needed_healing;
      if (pl.cash >= heal_cost) {
        pl.cash -= heal_cost;
        pl.max_stamina = pl.equipment[E_stamina].level;
        update_upgrade_ui(i_shop, pl);
      }
    } else if (ch == 'q' || ch == 'Q') {
      return;
    } else if (ch == 's' || ch == 'S') {
      shop_supplies(pl);
      return;
    }
  }
}

void update_upgrade_ui(cuss::interface &i_shop, Player &pl)
{
  i_shop.set_data("num_cash",         pl.cash);

  i_shop.set_data("num_cur_stamina",  pl.equipment[E_stamina].level );
  i_shop.set_data("num_cur_backpack", pl.equipment[E_backpack].level);
  i_shop.set_data("num_cur_pickaxe",  pl.equipment[E_pickaxe].level );
  i_shop.set_data("num_cur_sword",    pl.equipment[E_sword].level   );
  i_shop.set_data("num_cur_crossbow", pl.equipment[E_crossbow].level);
  i_shop.set_data("num_cur_lamp",     pl.equipment[E_lamp].level    );

  i_shop.set_data("num_post_stamina",
                  pl.equipment[E_stamina].level +
                  pl.equipment[E_stamina].upgrade_amount);
  i_shop.set_data("num_post_backpack",
                  pl.equipment[E_backpack].level +
                  pl.equipment[E_backpack].upgrade_amount);
  i_shop.set_data("num_post_pickaxe",
                  pl.equipment[E_pickaxe].level +
                  pl.equipment[E_pickaxe].upgrade_amount);
  i_shop.set_data("num_post_sword",
                  pl.equipment[E_sword].level +
                  pl.equipment[E_sword].upgrade_amount);
  i_shop.set_data("num_post_crossbow",
                  pl.equipment[E_crossbow].level +
                  pl.equipment[E_crossbow].upgrade_amount);
  i_shop.set_data("num_post_lamp",
                  pl.equipment[E_lamp].level +
                  pl.equipment[E_lamp].upgrade_amount);

  i_shop.set_data("num_cost_stamina",  pl.equipment[E_stamina].price );
  i_shop.set_data("num_cost_backpack", pl.equipment[E_backpack].price);
  i_shop.set_data("num_cost_pickaxe",  pl.equipment[E_pickaxe].price );
  i_shop.set_data("num_cost_sword",    pl.equipment[E_sword].price   );
  i_shop.set_data("num_cost_crossbow", pl.equipment[E_crossbow].price);
  i_shop.set_data("num_cost_lamp",     pl.equipment[E_lamp].price    );

  i_shop.set_data("num_max_stamina",   pl.max_stamina);
  int needed_healing = pl.equipment[E_stamina].level - pl.max_stamina;
  int heal_cost = 2 * needed_healing;
  i_shop.set_data("num_healed_stamina",pl.equipment[E_stamina].level );
  i_shop.set_data("num_cost_healing",  heal_cost);
  if (heal_cost > pl.cash) {
    i_shop.set_data("num_cost_healing", c_red);
  }
}

void shop_supplies(Player &pl)
{
// Asciiverse: the supply counter, drawn here so it can carry Whales' full
// list (ladders, rope, supports, bolts, dynamite, augers).
  Window w_shop(0, 0, 80, 24);
  struct Line { Supplies s; const char* name; const char* what; };
  Line lines[] = {
    { S_ladders,  "Ladder",   "climb back up out of any shaft" },
    { S_rope,     "Rope",     "a length to let down a deep drop" },
    { S_supports, "Support",  "holds up the boulders over your head" },
    { S_bolts,    "Bolt",     "for the crossbow" },
    { S_dynamite, "Dynamite", "clears ground, and monsters, all around" },
    { S_auger,    "Auger",    "bores a new mine entrance from the grass" }
  };
  while (true) {
    w_shop.clear();
    w_shop.putstr(2, 1, c_yellow, c_black, "SUPPLIES");
    w_shop.putstr(50, 1, c_yellow, c_black, "Cash: $" + itos(pl.cash));
    for (int i = 0; i < 6; i++) {
      Supplies s = lines[i].s;
      nc_color col = (pl.cash >= COST[s] ? c_white : c_dkgray);
      w_shop.putstr(2, 4 + i * 2, col, c_black,
                    std::string(1, char('a' + i)) + ")  " + lines[i].name);
      w_shop.putstr(17, 4 + i * 2, col, c_black, "$" + itos(COST[s]));
      w_shop.putstr(25, 4 + i * 2, c_ltgray, c_black,
                    "have " + itos(pl.supplies[s]));
      w_shop.putstr(36, 4 + i * 2, c_dkgray, c_black, lines[i].what);
    }
    w_shop.putstr(2, 18, c_ltgray, c_black,
                  "a to f: buy one (capital letter: buy five)");
    w_shop.putstr(2, 19, c_ltgray, c_black, "u: upgrades   q: back to the mine");
    w_shop.refresh();
    long ch = input();
    int n = 0, i = -1;
    if (ch >= 'a' && ch <= 'f') { i = ch - 'a'; n = 1; }
    if (ch >= 'A' && ch <= 'F') { i = ch - 'A'; n = 5; }
    if (i >= 0) {
      Supplies s = lines[i].s;
      for (int k = 0; k < n && pl.cash >= COST[s]; k++) {
        pl.cash -= COST[s];
        pl.supplies[s] += AMT[s];
      }
    }
    if (ch == 'q' || ch == 'Q' || ch == 27) {
      return;
    }
    if (ch == 'u' || ch == 'U') {
      shop_upgrades(pl);
      return;
    }
  }
}

void update_supply_ui(cuss::interface &i_shop, Player &pl)
{
}
