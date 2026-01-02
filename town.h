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
#ifndef TOWN_H
#define TOWN_H

#include "townfilecache.h"

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef USING_QT
#include <wslay/wslay.h>

#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/timing.h"
#endif

#ifdef __3DS__
#define VIEW_WIDTH_TILES 25
#define VIEW_HEIGHT_TILES 15
#include <3ds.h>
#include <citro2d.h>
#elif defined(USING_QT)
#include <qpixmap.h>
#include <QtWebSockets/QWebSocket>
#endif

class TilemapTownClient;

// ------------------------------------
struct MapTileInfo;

struct MapTileReference {
    std::variant<std::monostate, std::string, std::shared_ptr<MapTileInfo>> tile;

    MapTileInfo* get(TilemapTownClient *client);

    MapTileReference();
    MapTileReference(struct cJSON *json, TilemapTownClient *client);
    MapTileReference(std::string str);
    MapTileReference(std::string str, TilemapTownClient *client);
    MapTileReference(MapTileInfo *tile, TilemapTownClient *client);
    MapTileReference(std::shared_ptr<MapTileInfo> tile);
};

struct MapCell {
    struct MapTileReference turf;
    std::vector<struct MapTileReference> objs;

    MapCell();
    MapCell(struct MapTileReference turf);
};

class TownMap {
public:
    int width, height;
    std::vector<MapCell> cells;

    // Metadata
    int id;

    void init_map(int width, int height);
};


struct Pic {
    std::string key; // URL or integer
    int x;
    int y;
    bool key_is_url() const;

#ifdef __3DS__
    MultiTextureInfo *get_textures(TilemapTownClient *client) const;
#elif defined(USING_QT)
    const QPixmap *get_pixmap(TilemapTownClient *client) const;
#endif

    std::size_t hash() const;
};

class Entity {
public:
    std::string name;
    struct Pic pic;
    int x;
    int y;
    bool in_user_list;

    std::string vehicle_id;
    std::unordered_set <std::string> passengers;
    bool is_following;

    bool is_typing;

    // Animation
    int walk_timer;
    int direction;
    int direction_4;
    int direction_lr;
    int offset_x;
    int offset_y;

    std::string apply_json(cJSON *json);
    void update_direction(int direction);
};

enum MapTileType {
    MAP_TILE_NONE,
    MAP_TILE_SIGN,
};

struct MapTileInfo {
    std::string key;  // Key used to look up this MapTileInfo
    std::string name; // Name, for metadata
    std::string message; // For signs
    uint32_t autotile_class;

    // Appearance
    Pic pic;          // [sheet, x, y] format
    bool over;        // Display on top of entities
    uint8_t autotile_layout;

    // Animation
    uint8_t animation_frames, animation_speed, animation_mode;
    int8_t animation_offset;

    // Game logic related
    uint8_t walls;
    bool obj;
    enum MapTileType type;

    std::size_t hash() const;
};

// ------------------------------------

class TilemapTownClient
#ifdef USING_QT
    : public QObject
#endif
{
#ifndef USING_QT
    // Network
    wslay_event_context_ptr websocket;
    HttpFileCache http;

    // TLS
    mbedtls_net_context server_fd;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt cacert;
#else
    Q_OBJECT
    QWebSocket websocket;
#endif

public:
    TownFileCache *http;
    bool connected;

    // Game state
    TownMap town_map;
    std::unordered_map<std::string, Entity> who;
    std::unordered_map<std::size_t, std::weak_ptr<MapTileInfo>> json_tileset; // Custom JSON tiles, referenced by hash

    std::unordered_map<std::string, std::string> url_for_tile_sheet; // From RSC and IMG
    std::unordered_set<std::string> requested_tile_sheets; // IMG already sent

    std::unordered_map<std::string, std::shared_ptr<MapTileInfo>> tileset; // From RSC and TSD
    std::unordered_set<std::string> requested_tilesets; // TSD already sent

    bool map_received;
    bool need_redraw;
    int animation_tick;

    // Player state
    std::string your_id;
    float camera_x;
    float camera_y;
    bool walk_through_walls;

    // Websocket functions
    int websocket_connect(std::string host, std::string path, std::string port);
    void websocket_disconnect();
    void network_update(); // Does nothing on Qt; on other platforms, it also runs HTTP transfers
    void websocket_write(std::string text);
    void websocket_write(std::string command, cJSON *json);
    void websocket_message(const char *text, size_t length);

    void update_camera(float offset_x, float offset_y);
    void draw_map(int camera_x, int camera_y);
    Entity *your_entity();

    // Utilties to send protocol messages
    void login(const char *username, const char *password);
    void turn_player(int direction);
    void move_player(int offset_x, int offset_y);
    void request_image_asset(std::string key);
    void request_tileset_asset(std::string key);

    // Autotile utilities
    bool is_turf_autotile_match(const MapTileInfo *turf, TownMap *map, int map_x, int map_y);
    bool is_obj_autotile_match(const MapTileInfo *obj, TownMap *map, int map_x, int map_y);
    unsigned int get_turf_autotile_index_4(const MapTileInfo *turf, TownMap *map, int map_x, int map_y);
    unsigned int get_obj_autotile_index_4(const MapTileInfo *obj, TownMap *map, int map_x, int map_y);
    bool calc_pic_quarters(int quarter_x[4], int quarter_y[4], const MapTileInfo *tile, bool obj, TownMap *map, int map_x, int map_y, int tenth_of_second_counter);

    // Miscellaneous utilities
    std::shared_ptr<MapTileInfo> get_shared_pointer_to_tile(MapTileInfo *tile); // Get cached copy from json_tileset, or cache the tile for later use

    // Displaying messages involves the protocol code initiating a UI change - for Qt, this is done with a signal,
    // but on other platforms it may involve writing to global state somewhere.
#ifndef USING_QT
    void log_message(const std::string text, const std::string style); // Directly writes to the chat log
#else
signals:
    void log_message(const std::string text, const std::string style); // Sends a signal to the chat log

    // Handle websocket events
private Q_SLOTS:
    void onWebSocketConnected();
    void onWebSocketDisconnected();
    void onWebSocketError(QAbstractSocket::SocketError error);
    void onWebSocketTextMessageReceived(QString message);
    void onWebSocketSslErrors(const QList<QSslError> &errors);
#endif
};

#endif // TOWN_H
