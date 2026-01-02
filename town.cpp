/*
 * Tilemap Town native client
 *
 * Copyright (C) 2023-2025 NovaSquirrel
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "town.h"
#include "cJSON.h"

using namespace std;

void TownMap::init_map(int width, int height) {
    this->width = width;
    this->height = height;
    this->cells.clear();
    this->cells.resize(width * height);
}

// .-------------------------------------------------------
// | Map tile functions
// '-------------------------------------------------------

std::shared_ptr<MapTileInfo> TilemapTownClient::get_shared_pointer_to_tile(MapTileInfo *tile) {
    std::size_t hash = tile->hash();
    std::shared_ptr<MapTileInfo> ptr;

    // Look for it in the JSON tileset
    auto it = this->json_tileset.find(hash);
    if(it != this->json_tileset.end()) {
        ptr = (*it).second.lock();
        if(ptr)
            return ptr;
    }

    // Not found, so cache it for later
    ptr = make_shared<MapTileInfo>(*tile);
    this->json_tileset[hash] = ptr;
    return ptr;
}

MapTileInfo* MapTileReference::get(TilemapTownClient *client) {
    // If there's a pointer to the tile already, just return that tile
    if(const auto ptr = std::get_if<std::shared_ptr<MapTileInfo>>(&this->tile)) {
        return (*ptr).get();
    }

    // Is the tile available yet?
    if(const auto str = std::get_if<std::string>(&this->tile)) {
        // Is it in the client's tileset?
        auto it = client->tileset.find(*str);
        if(it != client->tileset.end()) {
            // Keep the reference to avoid needing to look it up next time
            this->tile = (*it).second;
            return (*it).second.get();
        }
    }

    return nullptr;
}

MapTileReference::MapTileReference(std::string str, TilemapTownClient *client) {
    // Is it in the client's tileset?
    auto it = client->tileset.find(str);
    if(it != client->tileset.end()) {
        this->tile = (*it).second;
        return;
    }
    // Otherwise, just store the string
    this->tile = str;
}

MapTileReference::MapTileReference(std::string str) {
    this->tile = str;
}

MapTileReference::MapTileReference(std::shared_ptr<MapTileInfo> tile) {
    this->tile = tile;
}

MapTileReference::MapTileReference(MapTileInfo* tile, TilemapTownClient *client) {
    this->tile = client->get_shared_pointer_to_tile(tile);
}

MapTileReference::MapTileReference() {
    this->tile = std::monostate();
}

std::size_t hash_combine(std::size_t a, std::size_t b) {
    unsigned prime = 0x01000193;
    a *= prime;
    a ^= b;
    return a;
}

std::size_t MapTileInfo::hash() const {
    std::hash<uint32_t> uint32_hash;
    std::hash<uint8_t> uint8_hash;
    std::hash<int8_t> int8_hash;
    std::hash<std::string> str_hash;
    std::hash<bool> bool_hash;

    std::size_t hash = str_hash(this->key);
    hash = hash_combine(hash, str_hash(this->name));
    hash = hash_combine(hash, str_hash(this->message));
    hash = hash_combine(hash, uint32_hash(this->autotile_class));
    hash = hash_combine(hash, this->pic.hash());
    hash = hash_combine(hash, bool_hash(this->over));
    hash = hash_combine(hash, uint8_hash(this->autotile_layout));
    hash = hash_combine(hash, uint8_hash(this->walls));
    hash = hash_combine(hash, bool_hash(this->obj));
    hash = hash_combine(hash, uint8_hash(this->type));
    hash = hash_combine(hash, uint8_hash(this->animation_frames));
    hash = hash_combine(hash, uint8_hash(this->animation_speed));
    hash = hash_combine(hash, uint8_hash(this->animation_mode));
    hash = hash_combine(hash, int8_hash(this->animation_offset));
    return hash;
}

std::size_t Pic::hash() const {
    std::hash<int> int_hash;
    std::hash<std::string> str_hash;

    std::size_t hash = str_hash(this->key);
    hash = hash_combine(hash, int_hash(this->x));
    hash = hash_combine(hash, int_hash(this->y));
    return hash;
}

bool Pic::key_is_url() const {
    return this->key.starts_with("https://") || this->key.starts_with("http://");
}

// --------------------------------------------------------

MapCell::MapCell() {
    this->turf = MapTileReference();
}

MapCell::MapCell(struct MapTileReference turf) {
    this->turf = turf;
}

// .-------------------------------------------------------
// | Map tile appearance calculation
// '-------------------------------------------------------

bool TilemapTownClient::is_turf_autotile_match(const MapTileInfo *turf, TownMap *map, int map_x, int map_y) {
    // Is the turf tile on the map at x,y the "same" as 'turf' for autotiling purposes?
    if(map_x < 0 || map_x >= this->town_map.width || map_y < 0 || map_y >= this->town_map.height)
        return true;
    MapTileInfo *other = this->town_map.cells[map_y * this->town_map.width + map_x].turf.get(this);

    if(turf->autotile_class)
        return turf->autotile_class == other->autotile_class;
    if(!turf->name.empty())
        return turf->name == other->name;
    return false;
}

bool TilemapTownClient::is_obj_autotile_match(const MapTileInfo *obj, TownMap *map, int map_x, int map_y) {
    // Is any obj tile on the map at x,y the "same" as 'obj' for autotiling purposes?
    if(map_x < 0 || map_x >= this->town_map.width || map_y < 0 || map_y >= this->town_map.height)
        return true;

    for(auto & element : this->town_map.cells[map_y * this->town_map.width + map_x].objs) {
        MapTileInfo *other_obj = element.get(this);
        if(!other_obj)
            continue;
        if(obj->autotile_class) {
            if(obj->autotile_class == other_obj->autotile_class)
                return true;
        } else if(!obj->name.empty()) {
            if(obj->name == other_obj->name)
                return true;
        }
    }
    return false;
}

unsigned int TilemapTownClient::get_turf_autotile_index_4(const MapTileInfo *turf, TownMap *map, int map_x, int map_y) {
    /* Check on the four adjacent tiles and see if they "match", to get an index for an autotile lookup table.
        Will result in one of the following:
         0 durl  1 durL  2 duRl  3 duRL
         4 dUrl  5 dUrL  6 dURl  7 dURL
         8 Durl  9 DurL 10 DuRl 11 DuRL
        12 DUrl 13 DUrL 14 DURl 15 DURL
    */
    return (this->is_turf_autotile_match(turf, map, map_x-1, map_y) << 0)
           | (this->is_turf_autotile_match(turf, map, map_x+1, map_y) << 1)
           | (this->is_turf_autotile_match(turf, map, map_x, map_y-1) << 2)
           | (this->is_turf_autotile_match(turf, map, map_x, map_y+1) << 3);
}

unsigned int TilemapTownClient::get_obj_autotile_index_4(const MapTileInfo *turf, TownMap *map, int map_x, int map_y) {
    /* Check on the four adjacent tiles and see if they "match", to get an index for an autotile lookup table.
        Will result in one of the following:
         0 durl  1 durL  2 duRl  3 duRL
         4 dUrl  5 dUrL  6 dURl  7 dURL
         8 Durl  9 DurL 10 DuRl 11 DuRL
        12 DUrl 13 DUrL 14 DURl 15 DURL
    */
    return (this->is_obj_autotile_match(turf, map, map_x-1, map_y) << 0)
           | (this->is_obj_autotile_match(turf, map, map_x+1, map_y) << 1)
           | (this->is_obj_autotile_match(turf, map, map_x, map_y-1) << 2)
           | (this->is_obj_autotile_match(turf, map, map_x, map_y+1) << 3);
}

bool TilemapTownClient::calc_pic_quarters(int quarter_x[4], int quarter_y[4], const MapTileInfo *tile, bool obj, TownMap *map, int map_x, int map_y, int tenth_of_second_counter) {
    // Returns false when only quarter_x[0] and quarter_y[0] are used and are in 16x16 units
    // Returns true when every index is used and quarter_x,quarter_y use 8x8 units
    if (!tile || !map)
        return false;

    int animation_frame = 0;
    if(tile->animation_frames > 1) {
        int animation_frame_count = tile->animation_frames;
        int animation_timer = tenth_of_second_counter + tile->animation_offset;
        int animation_speed = tile->animation_speed;
        if (animation_speed <= 0)
            animation_speed = 1;

        switch(tile->animation_mode) {
        case 0: // Forwards
            animation_frame = animation_timer / animation_speed % animation_frame_count;
            break;
        case 1: // Backwards
            animation_frame = animation_frame_count - 1 - (animation_timer / animation_speed % animation_frame_count);
            break;
        case 2: // Ping-pong forwards
        case 3: // Ping-pong backwards
        {
            animation_frame_count--;
            int sub_animation_frame = animation_timer / animation_speed % animation_frame_count;
            bool is_backwards = (animation_timer / animation_speed / animation_frame_count) & 1;
            if(is_backwards ^ (tile->animation_mode == 3)) {
                animation_frame = animation_frame_count - sub_animation_frame;
            } else {
                animation_frame = sub_animation_frame;
            }
            break;
        }
        }
    }

    switch(tile->autotile_layout) {
    default:
    {
        quarter_x[0] = tile->pic.x;
        quarter_y[0] = tile->pic.y;
        return false;
    }
    case 0: // No autotiling
    {
        quarter_x[0] = tile->pic.x + animation_frame;
        quarter_y[0] = tile->pic.y;
        return false;
    }
    case 1: // 4-direction autotiling, 9 tiles, origin is middle
    {
        unsigned int autotile_index = obj ? this->get_obj_autotile_index_4(tile, map, map_x, map_y)
                                          : this->get_turf_autotile_index_4(tile, map, map_x, map_y);
        const static int offset_x_list[] = {0,0,0,0,   0,1,-1,0,    0, 1,-1, 0,  0,1,-1,0};
        const static int offset_y_list[] = {0,0,0,0,   0,1, 1,1,    0,-1,-1,-1,  0,0, 0,0};
        quarter_x[0] = tile->pic.x + offset_x_list[autotile_index] + animation_frame * 3;
        quarter_y[0] = tile->pic.y + offset_y_list[autotile_index];
        return false;
    }
    case 2: // 4-direction autotiling, 9 tiles, origin is middle, horizonal & vertical & single as separate tiles
    case 3: // Same as 2, but origin point is single
    {
        unsigned int autotile_index = obj ? this->get_obj_autotile_index_4(tile, map, map_x, map_y)
                                          : this->get_turf_autotile_index_4(tile, map, map_x, map_y);
        const static int offset_x_list[] = { 2,1,-1,0};
        const static int offset_y_list[] = {-2,1,-1,0};
        bool isThree = tile->autotile_layout == 3;

        quarter_x[0] = tile->pic.x + offset_x_list[autotile_index&3] - (isThree?2:0) + animation_frame * 4;
        quarter_y[0] = tile->pic.y + offset_y_list[autotile_index>>2] + (isThree?2:0);
        return false;
    }
    case 4: // 8-direction autotiling, origin point is middle
    case 5: // 8-direction autotiling, origin point is single
    {
        unsigned int autotile_index = obj ? this->get_obj_autotile_index_4(tile, map, map_x, map_y)
                                          : this->get_turf_autotile_index_4(tile, map, map_x, map_y);
        const static int offset_0x[] = {-2, 2,-2, 0,-2, 2,-2, 0,-2, 2,-2, 0,-2, 2,-2, 0};
        const static int offset_0y[] = {-4,-2,-2,-2, 2, 2, 2, 2,-2,-2,-2,-2, 0, 0, 0, 0};
        const static int offset_1x[] = {-1, 3,-1, 1, 3, 3,-1, 1, 3, 3,-1, 1, 3, 3,-1, 1};
        const static int offset_1y[] = {-4,-2,-2,-2, 2, 2, 2, 2,-2,-2,-2,-2, 0, 0, 0, 0};
        const static int offset_2x[] = {-2, 2,-2, 0,-2, 2,-2, 0,-2, 2,-2, 0,-2, 2,-2, 0};
        const static int offset_2y[] = {-3, 3, 3, 3, 3, 3, 3, 3,-1,-1,-1,-1, 1, 1, 1, 1};
        const static int offset_3x[] = {-1, 3,-1, 1, 3, 3,-1, 1, 3, 3,-1, 1, 3, 3,-1, 1};
        const static int offset_3y[] = {-3, 3, 3, 3, 3, 3, 3, 3,-1,-1,-1,-1, 1, 1, 1, 1};

        quarter_x[0] = offset_0x[autotile_index], quarter_y[0] = offset_0y[autotile_index];
        quarter_x[1] = offset_1x[autotile_index], quarter_y[1] = offset_1y[autotile_index];
        quarter_x[2] = offset_2x[autotile_index], quarter_y[2] = offset_2y[autotile_index];
        quarter_x[3] = offset_3x[autotile_index], quarter_y[3] = offset_3y[autotile_index];

        // Add the inner parts of turns
        if(((autotile_index &  5) ==  5)
            && !(obj ? this->is_obj_autotile_match(tile, map, map_x-1, map_y-1)
                     : this->is_turf_autotile_match(tile, map, map_x-1, map_y-1))) {
            quarter_x[0] = 2; quarter_y[0] = -4;
        }
        if(((autotile_index &  6) ==  6)
            && !(obj ? this->is_obj_autotile_match(tile, map, map_x+1, map_y-1)
                     : this->is_turf_autotile_match(tile, map, map_x+1, map_y-1))) {
            quarter_x[1] = 3; quarter_y[1] = -4;
        }
        if(((autotile_index &  9) ==  9)
            && !(obj ? this->is_obj_autotile_match(tile, map, map_x-1, map_y+1)
                     : this->is_turf_autotile_match(tile, map, map_x-1, map_y+1))) {
            quarter_x[2] = 2; quarter_y[2] = -3;
        }
        if(((autotile_index & 10) == 10)
            && !(obj ? this->is_obj_autotile_match(tile, map, map_x+1, map_y+1)
                     : this->is_turf_autotile_match(tile, map, map_x+1, map_y+1))) {
            quarter_x[3] = 3; quarter_y[3] = -3;
        }

        // For 4 the origin point is on the single tile instead of the all-connected tile
        if(tile->autotile_layout == 5) {
            quarter_x[0] += 2; quarter_x[1] += 2; quarter_x[2] += 2; quarter_x[3] += 2;
            quarter_y[0] += 4; quarter_y[1] += 4; quarter_y[2] += 4; quarter_y[3] += 4;
        }

        // Change quarter_x and quarter_y from offset to actual pic quarter coordinates
        quarter_x[0] += (tile->pic.x + animation_frame * 6) * 2;
        quarter_x[1] += (tile->pic.x + animation_frame * 6) * 2;
        quarter_x[2] += (tile->pic.x + animation_frame * 6) * 2;
        quarter_x[3] += (tile->pic.x + animation_frame * 6) * 2;
        quarter_y[0] += tile->pic.y * 2;
        quarter_y[1] += tile->pic.y * 2;
        quarter_y[2] += tile->pic.y * 2;
        quarter_y[3] += tile->pic.y * 2;
        return true;
    }
    case 6: // horizontal - middle 3
    {
        bool right = obj ? is_obj_autotile_match(tile, map, map_x+1, map_y) : is_turf_autotile_match(tile, map, map_x+1, map_y);
        bool left = obj ? is_obj_autotile_match(tile, map, map_x-1, map_y) : is_turf_autotile_match(tile, map, map_x-1, map_y);
        quarter_x[0] = tile->pic.x - (!left && right) + (left && !right) + animation_frame*3;
        quarter_y[0] = tile->pic.y;
        return false;
    }
    case 7: case 8: // horizontal
    {
        bool right = obj ? is_obj_autotile_match(tile, map, map_x+1, map_y) : is_turf_autotile_match(tile, map, map_x+1, map_y);
        bool left = obj ? is_obj_autotile_match(tile, map, map_x-1, map_y) : is_turf_autotile_match(tile, map, map_x-1, map_y);
        quarter_x[0] = tile->pic.x - (!left && right) + (left && !right) + animation_frame*3;
        quarter_y[0] = tile->pic.y;
        if(!left && !right) quarter_x[0] += 2;
        if(tile->autotile_layout == 8) quarter_x[0] -= 2;
        return false;
    }
    case 9: // vertical - middle 3
    {
        bool bottom = obj ? is_obj_autotile_match(tile, map, map_x, map_y+1) : is_turf_autotile_match(tile, map, map_x, map_y+1);
        bool top = obj ? is_obj_autotile_match(tile, map, map_x, map_y-1) : is_turf_autotile_match(tile, map, map_x, map_y-1);
        quarter_x[0] = tile->pic.x + animation_frame;
        quarter_y[0] = tile->pic.y - (!top && bottom) + (top && !bottom);
        return false;
    }
    case 10: case 11: // vertical
    {
        bool bottom = obj ? is_obj_autotile_match(tile, map, map_x, map_y+1) : is_turf_autotile_match(tile, map, map_x, map_y+1);
        bool top = obj ? is_obj_autotile_match(tile, map, map_x, map_y-1) : is_turf_autotile_match(tile, map, map_x, map_y-1);
        quarter_x[0] = tile->pic.x + animation_frame;
        quarter_y[0] = tile->pic.y - (!top && bottom) + (top && !bottom);
        if(!top && !bottom) quarter_y[0] -= 2;
        if(tile->autotile_layout == 11) quarter_y[0] += 2;
        return false;
    }
    case 12: case 13: case 14: case 15: // 8-way autotile
    {
        unsigned int autotile_index = obj ? this->get_obj_autotile_index_4(tile, map, map_x, map_y)
                                          : this->get_turf_autotile_index_4(tile, map, map_x, map_y);
        const static uint8_t offsets_8[16][4][2] =
            {{{0, 2},{1, 2},{0, 3},{1, 3}}, {{0, 7},{1, 2},{1, 6},{1, 3}},
             {{0, 2},{0, 7},{0, 3},{1, 6}}, {{0, 7},{0, 7},{1, 6},{1, 6}},
             {{1, 7},{0, 6},{0, 3},{1, 3}}, {{0, 0},{0, 6},{1, 6},{1, 3}},
             {{1, 7},{1, 0},{0, 3},{1, 6}}, {{0, 0},{1, 0},{1, 6},{1, 6}},
             {{0, 2},{1, 2},{1, 7},{0, 6}}, {{0, 7},{1, 2},{0, 1},{0, 6}},
             {{0, 2},{0, 7},{1, 7},{1, 1}}, {{0, 7},{0, 7},{0, 1},{1, 1}},
             {{1, 7},{0, 6},{1, 7},{0, 6}}, {{0, 0},{0, 6},{0, 1},{0, 6}},
             {{1, 7},{1, 0},{1, 7},{1, 1}}, {{0, 0},{1, 0},{0, 1},{1, 1}}};
        const static uint8_t offsets_16[16][4][2] =
            {{{0, 2},{1, 2},{0, 3},{1, 3}}, {{0, 8},{1, 2},{0, 9},{1, 3}},
             {{0, 2},{1, 8},{0, 3},{1, 9}}, {{0, 8},{1, 8},{0, 9},{1, 9}},
             {{0, 6},{1, 6},{0, 3},{1, 3}}, {{0, 0},{1, 6},{0, 9},{1, 3}},
             {{0, 6},{1, 0},{0, 3},{1, 9}}, {{0, 0},{1, 0},{0, 9},{1, 9}},
             {{0, 2},{1, 2},{0, 7},{1, 7}}, {{0, 8},{1, 2},{0, 1},{1, 7}},
             {{0, 2},{1, 8},{0, 7},{1, 1}}, {{0, 8},{1, 8},{0, 1},{1, 1}},
             {{0, 6},{1, 6},{0, 7},{1, 7}}, {{0, 0},{1, 6},{0, 1},{1, 7}},
             {{0, 6},{1, 0},{0, 7},{1, 1}}, {{0, 0},{1, 0},{0, 1},{1, 1}}};

        // Add the inner parts of turns
        if(((autotile_index &  5) ==  5)
            && !(obj ? this->is_obj_autotile_match(tile, map, map_x-1, map_y-1)
                     : this->is_turf_autotile_match(tile, map, map_x-1, map_y-1))) {
            quarter_x[0] = 0;
            quarter_y[0] = 4;
        }
        if(((autotile_index &  6) ==  6)
            && !(obj ? this->is_obj_autotile_match(tile, map, map_x+1, map_y-1)
                     : this->is_turf_autotile_match(tile, map, map_x+1, map_y-1))) {
            quarter_x[1] = 1;
            quarter_y[1] = 4;
        }
        if(((autotile_index &  9) ==  9)
            && !(obj ? this->is_obj_autotile_match(tile, map, map_x-1, map_y+1)
                     : this->is_turf_autotile_match(tile, map, map_x-1, map_y+1))) {
            quarter_x[2] = 0;
            quarter_y[2] = 5;
        }
        if(((autotile_index & 10) == 10)
            && !(obj ? this->is_obj_autotile_match(tile, map, map_x+1, map_y+1)
                     : this->is_turf_autotile_match(tile, map, map_x+1, map_y+1))) {
            quarter_x[3] = 1;
            quarter_y[3] = 5;
        }

        // Change quarter_x and quarter_y from offset to actual pic quarter coordinates
        quarter_x[0] += (tile->pic.x + animation_frame) * 2;
        quarter_x[1] += (tile->pic.x + animation_frame) * 2;
        quarter_x[2] += (tile->pic.x + animation_frame) * 2;
        quarter_x[3] += (tile->pic.x + animation_frame) * 2;
        quarter_y[0] += tile->pic.y * 2;
        quarter_y[1] += tile->pic.y * 2;
        quarter_y[2] += tile->pic.y * 2;
        quarter_y[3] += tile->pic.y * 2;
        return true;
    }
    }
}

// .-------------------------------------------------------
// | Game logic/movement related
// '-------------------------------------------------------

Entity *TilemapTownClient::your_entity() {
    if(this->your_id.empty())
        return nullptr;
    auto it = this->who.find(this->your_id);
    if(it == this->who.end())
        return nullptr;
    return &(*it).second;
}

#ifdef __3DS__
void TilemapTownClient::update_camera(float offset_x, float offset_y) {
    Entity *you = this->your_entity();
    if(!you)
        return;

    float target_x = you->x*16 - ((float)VIEW_WIDTH_TILES*8) + offset_x;
    float target_y = you->y*16 - ((float)VIEW_HEIGHT_TILES*8) + offset_y;

    float difference_x = target_x - this->camera_x;
    float difference_y = target_y - this->camera_y;

    if(fabs(difference_x) > 0.5)
        this->camera_x += difference_x / 12;
    if(fabs(difference_y) > 0.5)
        this->camera_y += difference_y / 12;
}
#endif

void TilemapTownClient::turn_player(int direction) {
    Entity *you = this->your_entity();
    if(!you)
        return;
    you->update_direction(direction);

    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "dir", (double)direction);
    this->websocket_write("MOV", json);
    cJSON_Delete(json);
}

void TilemapTownClient::move_player(int offset_x, int offset_y) {
    Entity *you = this->your_entity();
    if(!you)
        return;
    int original_x = you->x;
    int original_y = you->y;

    // Figure out the direction from the offset
    int new_direction = 0;
    if(offset_x > 0 && offset_y == 0) {
        new_direction = 0;
    } else if(offset_x > 0 && offset_y > 0) {
        new_direction = 1;
    } else if(offset_x == 0 && offset_y > 0) {
        new_direction = 2;
    } else if(offset_x < 0 && offset_y > 0) {
        new_direction = 3;
    } else if(offset_x < 0 && offset_y == 0) {
        new_direction = 4;
    } else if(offset_x < 0 && offset_y < 0) {
        new_direction = 5;
    } else if(offset_x == 0 && offset_y < 0) {
        new_direction = 6;
    } else if(offset_x > 0 && offset_y < 0) {
        new_direction = 7;
    }
    you->update_direction(new_direction);

    bool bumped = false;
    int bumped_x, bumped_y;

    int new_x = original_x + offset_x;
    if(new_x < 0)
        new_x = 0;
    if(new_x >= this->town_map.width)
        new_x = this->town_map.width-1;

    int new_y = original_y + offset_y;
    if(new_y < 0)
        new_y = 0;
    if(new_y >= this->town_map.height)
        new_y = this->town_map.height-1;
    if((new_x != original_x + offset_x) || (new_y != original_y + offset_y)) {
        bumped = true;
        bumped_x = original_x + offset_x;
        bumped_y = original_y + offset_y;
    }

    you->x = new_x;
    you->y = new_y;

    ////////////////////////////
    // Check old tile for walls
    ////////////////////////////
    MapCell *cell = &this->town_map.cells[original_y * this->town_map.width + original_x];

    MapTileInfo *turf = cell->turf.get(this);
    if(turf && (turf->walls & (1 << new_direction)) && !this->walk_through_walls) {
        // Go back
        bumped = true;
        bumped_x = original_x;
        bumped_y = original_y;
        you->x = original_x;
        you->y = original_y;
    }

    for(auto & obj_reference : cell->objs) {
        MapTileInfo *obj = obj_reference.get(this);
        if(!obj)
            continue;
        if((obj->walls & (1 << new_direction)) && !this->walk_through_walls) {
            if(!bumped) {
                bumped = true;
                bumped_x = original_x;
                bumped_y = original_y;
            }
            // Go back
            you->x = original_x;
            you->y = original_y;
        }
    }

    ////////////////////////////
    // Check new tile for walls
    ////////////////////////////
    if (!bumped) {
        int dense_wall_bit = 1 << ((new_direction + 4) & 7); // For the new cell, the direction to check is rotated 180 degrees
        cell = &this->town_map.cells[new_y * this->town_map.width + new_x];

        turf = cell->turf.get(this);
        if(turf && turf->type == MAP_TILE_SIGN) {
            printf("\x1b[35m%s says: %s\x1b[0m\n", (turf->name=="sign" || turf->name.empty()) ? "The sign" : turf->name.c_str(), turf->message.c_str());
        }
        if(turf && (turf->walls & dense_wall_bit) && !this->walk_through_walls) {
            // Go back
            bumped = true;
            bumped_x = you->x;
            bumped_y = you->y;
            you->x = original_x;
            you->y = original_y;
        }

        for(auto & obj_reference : cell->objs) {
            MapTileInfo *obj = obj_reference.get(this);
            if(!obj)
                continue;
            if(obj->type == MAP_TILE_SIGN) {
                printf("\x1b[35m%s says: %s\x1b[0m\n", (obj->name=="sign" || obj->name.empty()) ? "The sign" : obj->name.c_str(), obj->message.c_str());
            }
            if((obj->walls & dense_wall_bit) && !this->walk_through_walls) {
                if(!bumped) {
                    bumped = true;
                    bumped_x = you->x;
                    bumped_y = you->y;
                }
                // Go back
                you->x = original_x;
                you->y = original_y;
            }
        }
    }

    //////////////////////////////////////
    // Tell the server about the movement
    //////////////////////////////////////
    cJSON *json = cJSON_CreateObject();
    if(!bumped) {
        int from_array[2] = {original_x, original_y};
        cJSON *json_from = cJSON_CreateIntArray(from_array, 2);
        cJSON_AddItemToObject(json, "from", json_from);

        int to_array[2] = {you->x, you->y};
        cJSON *json_to = cJSON_CreateIntArray(to_array, 2);
        cJSON_AddItemToObject(json, "to", json_to);
    } else {
        int bump_array[2] = {bumped_x, bumped_y};
        cJSON *json_bump = cJSON_CreateIntArray(bump_array, 2);
        cJSON_AddItemToObject(json, "bump", json_bump);

        // Tell server what map the bump is intended for
        if(this->town_map.id != 0) {
            cJSON_AddNumberToObject(json, "if_map", this->town_map.id);
        }
    }
    cJSON_AddNumberToObject(json, "dir", (double)new_direction);

    this->websocket_write("MOV", json);
    cJSON_Delete(json);

    you->walk_timer = 30+1; // 30*(16.6666ms/1000) = 0.5
}

void Entity::update_direction(int direction) {
    this->direction = direction;

    if((direction & 1) == 0) // Four directions only
        this->direction_4 = direction;
    if(direction == 0 || direction == 4) // Left and right only
        this->direction_lr = direction;
}

#ifndef USING_QT
void TilemapTownClient::log_message(std::string text, std::string style) {
    puts(text);
}
#endif
