#ifndef LEVEL_HPP_INCLUDED
#define LEVEL_HPP_INCLUDED

#include <bitset>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "boost/array.hpp"
#include "boost/scoped_ptr.hpp"

#include "background.hpp"
#include "entity.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "geometry.hpp"
#include "gui_formula_functions.hpp"
#include "level_object.hpp"
#include "level_solid_map.hpp"
#include "movement_script.hpp"
#include "raster.hpp"
#include "tile_map.hpp"
#include "water.hpp"
#include "wml_node_fwd.hpp"
#include "color_utils.hpp"

class level : public game_logic::formula_callable
{
public:
	static level& current();
	void set_as_current_level();

	explicit level(const std::string& level_cfg);

	//function to do anything which loads the level and must be done
	//in the main thread.
	void finish_loading();

	//function which sets which player we're controlling on this machine.
	void set_multiplayer_slot(int slot);

	const std::string& replay_data() const { return replay_data_; }
	void load_save_point(const level& lvl);
	void set_save_point(int x, int y) { save_point_x_ = x; save_point_y_ = y; }

	const std::string& id() const { return id_; }
	const std::string& music() const { return music_; }

	wml::node_ptr write() const;
	void draw(int x, int y, int w, int h) const;
	void draw_status() const;
	void draw_debug_solid(int x, int y, int w, int h) const;
	void draw_background(double x, double y, int rotation) const;
	void process();
	bool standable(int x, int y, int* friction=NULL, int* traction=NULL, int* damage=NULL) const;
	bool standable_tile(int x, int y, int* friction=NULL, int* traction=NULL, int* damage=NULL) const;
	bool solid(int x, int y, int* friction=NULL, int* traction=NULL, int* damage=NULL) const;
	bool solid(const entity& e, const std::vector<point>& points, int* friction=NULL, int* traction=NULL, int* damage=NULL) const;
	bool solid(const rect& r, int* friction=NULL, int* traction=NULL, int* damage=NULL) const;
	bool may_be_solid_in_rect(const rect& r) const;
	void set_solid_area(const rect& r, bool solid);
	entity_ptr collide(int x, int y, const entity* exclude=NULL) const;
	entity_ptr collide(const rect& r, const entity* exclude=NULL) const;
	entity_ptr board(int x, int y) const;
	entity_ptr hit_by_player(const rect& r) const;
	const rect& boundaries() const { return boundaries_; }
	void set_boundaries(const rect& bounds) { boundaries_ = bounds; }
	void add_tile(const level_tile& t);
	void add_tile_rect(int zorder, int x1, int y1, int x2, int y2, const std::string& tile);
	void add_tile_rect_vector(int zorder, int x1, int y1, int x2, int y2, const std::vector<std::string>& tiles);
	void set_tile_layer_speed(int zorder, int x_speed, int y_speed);
	void refresh_tile_rect(int x1, int y1, int x2, int y2);
	void get_tile_rect(int zorder, int x1, int y1, int x2, int y2, std::vector<std::string>& tiles) const;
	void get_all_tiles_rect(int x1, int y1, int x2, int y2, std::map<int, std::vector<std::string> >& tiles) const;
	void clear_tile_rect(int x1, int y1, int x2, int y2);
	void remove_tiles_at(int x, int y);

	//function to do 'magic wand' selection -- given an x/y pixel position,
	//will return all the solid tiles connected
	std::vector<point> get_solid_contiguous_region(int xpos, int ypos) const;

	const level_tile* get_tile_at(int x, int y) const;
	void remove_character(entity_ptr e);
	std::vector<entity_ptr> get_characters_in_rect(const rect& r) const;
	entity_ptr get_character_at_point(int x, int y) const;
	const player_info* player() const { return player_ ? player_->get_player_info() : NULL; }
	player_info* player() { return player_ ? player_->get_player_info() : NULL; }
	std::vector<entity_ptr>& players() { return players_; }
	const std::vector<entity_ptr>& players() const { return players_; }
	void add_player(entity_ptr p);
	void add_character(entity_ptr p);

	//schedule a character for removal at the end of the current cycle.
	void schedule_character_removal(entity_ptr p);

	//sets the last 'touched' player. This is the player found in the level when
	//using WML, so it works reasonably well in multiplayer.
	void set_touched_player(entity_ptr p) { last_touched_player_ = p; }

	struct portal {
		portal() : dest_starting_pos(false), automatic(false), saved_game(false)
		{}
		rect area;
		std::string level_dest;
		std::string dest_label;
		std::string dest_str;
		point dest;
		bool dest_starting_pos;
		bool automatic;
		std::string transition;
		bool saved_game;
	};

	//function which will make it so the next call to get_portal() will return
	//a pointer to a copy of the given portal. i.e. this makes the character immediately
	//enter a portal.
	void force_enter_portal(const portal& p);

	//the portal the character has entered (if any)
	const portal* get_portal() const;

	int xscale() const { return xscale_; }
	int yscale() const { return yscale_; }

	int group_size(int group) const;
	void set_character_group(entity_ptr c, int group_num);
	int add_group();

	void set_editor() { editor_ = true; }
	void set_editor_highlight(entity_ptr c) { editor_highlight_ = c; }
	entity_ptr editor_highlight() const { return editor_highlight_; }

	void editor_select_object(entity_ptr c);
	void editor_clear_selection();

	const std::vector<entity_ptr>& editor_selection() const { return editor_selection_; }

	bool show_foreground() const { return show_foreground_; }
	void set_show_foreground(bool value) { show_foreground_ = value; }

	bool show_background() const { return show_background_; }
	void set_show_background(bool value) { show_background_ = value; }

	const std::string& get_background_id() const;
	void set_background_by_id(const std::string& id);

	void rebuild_tiles();

	const std::string& title() const { return title_; }
	void set_title(const std::string& t) { title_ = t; }

	int variations(int xtile, int ytile) const;
	void flip_variations(int xtile, int ytile, int delta=1);

	int auto_move_camera_x() const { return auto_move_camera_.x; }
	int auto_move_camera_y() const { return auto_move_camera_.y; }

	int air_resistance() const { return air_resistance_; }
	int water_resistance() const { return water_resistance_; }

	int camera_rotation() const;

	void set_end_game() { end_game_ = true; }
	bool end_game() const { return end_game_; }

	//Function used when the player is entering the level at a certain point.
	//Will take the name of a destination within a level and return the point
	//at that location. For now only takes "left" and "right".
	point get_dest_from_str(const std::string& key) const;

	//levels ended up at by exiting this level to the left or right.
	const std::string& previous_level() const;
	const std::string& next_level() const;

	void set_previous_level(const std::string& name);
	void set_next_level(const std::string& name);

	int cycle() const { return cycle_; }
	bool is_underwater(const rect& r, rect* res_water_area=NULL) const;

	void get_current(const entity& e, int* velocity_x, int* velocity_y) const;

	graphics::color tint() const { return tint_; }

	water* get_water() { return water_.get(); }
	const water* get_water() const { return water_.get(); }

	water& get_or_create_water();

	entity_ptr get_entity_by_label(const std::string& label);
	const_entity_ptr get_entity_by_label(const std::string& label) const;

	void get_all_labels(std::vector<std::string>& labels) const;

	const std::vector<entity_ptr>& get_chars() const { return chars_; }
	const std::vector<entity_ptr>& get_solid_chars() const;
	void swap_chars(std::vector<entity_ptr>& v) { chars_.swap(v); solid_chars_.clear(); }
	int num_active_chars() const { return active_chars_.size(); }

	void begin_movement_script(const std::string& name, entity& e);
	void end_movement_script();

	//function which, given the rect of the player's body will return true iff
	//the player can currently "interact" with a portal or object. i.e. if
	//pressing up will talk to someone or enter a door etc.
	bool can_interact(const rect& body) const;

	void replay_from_cycle(int ncycle);
	void backup();

	bool is_multiplayer() const { return players_.size() > 1; }

	void get_tile_layers(std::set<int>* all_layers, std::set<int>* hidden_layers=NULL);
	void hide_tile_layer(int layer, bool is_hidden);

	void highlight_tile_layer(int layer) { highlight_layer_ = layer; }

	const point* lock_screen() const { return lock_screen_.get(); }

	void editor_freeze_tile_updates(bool value);

private:
	level(const level&);
	void operator=(const level&);

	void read_compiled_tiles(wml::const_node_ptr node, std::vector<level_tile>::iterator& out);

	void prepare_tiles_for_drawing();

	void do_processing();

	bool add_tile_rect_vector_internal(int zorder, int x1, int y1, int x2, int y2, const std::vector<std::string>& tiles);

	void draw_layer(int layer, int x, int y, int w, int h) const;
	void draw_layer_solid(int layer, int x, int y, int w, int h) const;

	void rebuild_tiles_rect(const rect& r);
	void add_tile_solid(const level_tile& t);
	void add_solid_rect(int x1, int y1, int x2, int y2, int friction, int traction, int damage);
	void add_solid(int x, int y, int friction, int traction, int damage);
	void add_standable(int x, int y, int friction, int traction, int damage);
	typedef std::pair<int,int> tile_pos;
	typedef std::bitset<TileSize*TileSize> tile_bitmap;

	std::string id_;
	std::string music_;
	std::string replay_data_;
	int cycle_;

	std::map<std::string, variant> vars_;
	
	level_solid_map solid_;
	level_solid_map standable_;

	bool is_solid(const level_solid_map& map, int x, int y, int* friction, int* traction, int* damage) const;
	bool is_solid(const level_solid_map& map, const entity& e, const std::vector<point>& points, int* friction, int* traction, int* damage) const;

	void set_solid(level_solid_map& map, int x, int y, int friction, int traction, int damage, bool solid=true);

	variant get_value(const std::string& key) const;
	void set_value(const std::string& key, const variant& value);

	std::string title_;

	rect boundaries_;

	struct solid_rect {
		rect r;
		int friction;
		int traction;
		int damage;
	};
	std::vector<solid_rect> solid_rects_;
	std::vector<level_tile> tiles_;
	std::set<int> layers_;
	std::set<int> hidden_layers_; //layers hidden in the editor.
	int highlight_layer_;

	struct layer_blit_info {
		graphics::blit_queue blit_queue;
		rect tile_positions;
	};

	mutable std::map<int, layer_blit_info> blit_cache_;

	struct solid_color_rect {
		graphics::color color;
		rect area;
		int layer;
	};

	struct solid_color_rect_empty {
		bool operator()(const solid_color_rect& r) const { return r.area.w() == 0; }
	};

	struct solid_color_rect_cmp {
		bool operator()(const solid_color_rect& r, int zorder) const { return r.layer < zorder; }
		bool operator()(int zorder, const solid_color_rect& r) const { return zorder < r.layer; }
		bool operator()(const solid_color_rect& a, const solid_color_rect& b) const { return a.layer < b.layer; }
	};

	std::vector<solid_color_rect> solid_color_rects_;

	void erase_char(entity_ptr c);
	std::vector<entity_ptr> chars_;
	std::vector<entity_ptr> active_chars_;
	mutable std::vector<entity_ptr> solid_chars_;

	std::map<std::string, entity_ptr> chars_by_label_;
	entity_ptr player_;
	entity_ptr last_touched_player_;

	std::vector<entity_ptr> players_;

	//characters stored in wml format; they can't be loaded in a separate thread
	//they will be loaded when complete_load_level() is called.
	std::vector<wml::const_node_ptr> wml_chars_;

	void load_character(wml::const_node_ptr c);

	typedef std::vector<entity_ptr> entity_group;
	std::vector<entity_group> groups_;

	portal left_portal_, right_portal_;
	std::vector<portal> portals_;

	mutable bool entered_portal_active_;
	portal entered_portal_;

	boost::shared_ptr<background> background_;
	int widest_tile_, highest_tile_;

	std::map<int, tile_map> tile_maps_;
	int xscale_, yscale_;

	int save_point_x_, save_point_y_;
	bool editor_;
	entity_ptr editor_highlight_;

	std::vector<entity_ptr> editor_selection_;

	bool show_foreground_, show_background_;

	point auto_move_camera_;
	int air_resistance_;
	int water_resistance_;

	game_logic::const_formula_ptr camera_rotation_;
	bool end_game_;

	std::vector<std::string> preloads_; //future levels to preload

	boost::scoped_ptr<water> water_;

	graphics::color tint_;
	
	std::map<std::string, movement_script> movement_scripts_;
	std::vector<active_movement_script_ptr> active_movement_scripts_;

	boost::scoped_ptr<point> lock_screen_;

	struct backup_snapshot {
		unsigned int rng_seed;
		int cycle;
		std::vector<entity_ptr> chars;
		std::vector<entity_ptr> players;
		entity_ptr player, last_touched_player;
	};

	void restore_from_backup(backup_snapshot& snapshot);

	typedef boost::shared_ptr<backup_snapshot> backup_snapshot_ptr;

	std::deque<backup_snapshot_ptr> backups_;

	int editor_tile_updates_frozen_;

	gui_algorithm_ptr gui_algorithm_;
};

#endif
