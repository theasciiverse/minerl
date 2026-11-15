#ifndef _MAP_H_
#define _MAP_H_

#include "tiledata.h"
#include "window.h"
#include "geometry.h"
#include "monster.h"
#include <vector>
#include <iostream>

class Player;

struct Tile
{
  Tile_id id;
  glyph sym;
  int hp;
  bool seen;
  bool ready_to_fall;

  Tile();

  void set_id(Tile_id new_id);
  bool dig(int amount);  // Returns true if we were destroyed
};

class Map
{
public:

  Map();
  ~Map();

  void reset_null_tile();
  void generate();
  void add_vein(Tile_id type, int x, int y, int size);

  Tile*       get_tile     (int x, int y);
  Tile_id     get_tile_id  (int x, int y);
  Tile_datum* get_tile_data(int x, int y);
  void draw(Window* w, Player* pl);

  void set_tile(int x, int y, Tile_id tile);
  void add_ladder (int x, int y);
  void add_support(int x, int y);
  void add_falling(int x, int y);

  void add_msg(std::string message);
  void clear_msgs() { messages.clear(); }

// Asciiverse: dynamite with a short fuse, and rope let down a shaft
  struct Bomb { int x, y, fuse; };
  std::vector<Bomb> bombs;
  void add_bomb(int x, int y);
  void process_bombs(Player& pl);
  void explode(int x, int y, Player& pl);
  int add_rope(int x, int y, int ropes);  // Returns rope used

// Asciiverse: saving
  void save(std::ostream& out);
  bool load(std::istream& in);

  void process_falling(Player& pl);
  void spawn_monsters (Player& pl);
  void move_monsters  (Player& pl);
  Monster* monster_at(int x, int y);

  std::vector<Point> path(Monster_id type,
                          int origx, int origy, int destx, int desty);
  int pathing_cost(Monster_id type, int curx, int cury, int x, int y);

  std::vector<Monster> monsters;
  int heart_x, heart_y;  // Asciiverse: where the Heartstone lies

  std::vector< std::vector<Tile> > tiles;
private:
  std::vector< std::string > messages;
  std::vector<Point> falling;
  Tile null_tile;
};

  
#endif
