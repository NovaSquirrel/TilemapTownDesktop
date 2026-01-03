/*
 * Tilemap Town desktop client
 *
 * Copyright (C) 2025 NovaSquirrel
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
#include "tilemaptownmapview.h"
#include "town.h"

#include <QPainter>
#include <QPainterStateGuard>
#include <QKeyEvent>

inline int positive_modulo(int i, unsigned int n) {
    return (i % n + n) % n;
}
bool sort_entity_by_y_pos(Entity *a, Entity *b) {
    return (a->y < b->y);
}

const QPixmap *Pic::get_pixmap(TilemapTownClient *client) const {
    if (this->key_is_url()) {
        return client->http->get_pixmap(this->key);
    }

    auto find_url = client->url_for_tile_sheet.find(this->key);
    if(find_url == client->url_for_tile_sheet.end()) {
        client->request_image_asset(this->key);
        return nullptr;
    }

    return client->http->get_pixmap((*find_url).second);
}

TilemapTownMapView::TilemapTownMapView(QWidget *parent)
    : QWidget(parent)
{
    this->tilemapTownClient = nullptr;
}

void TilemapTownMapView::drawMapTile(QPainter *painter, const MapTileInfo *tile, bool obj, int map_x, int map_y, float draw_x, float draw_y, int scale) {
    int quarters_x[4], quarters_y[4];
    const QPixmap *pixmap = tile->pic.get_pixmap(this->tilemapTownClient);
    if (!pixmap)
        return;

    if (this->tilemapTownClient->calc_pic_quarters(quarters_x, quarters_y, tile, obj, &this->tilemapTownClient->town_map, map_x, map_y, 0)) {
        // 8x8 tiles
        painter->drawPixmap(draw_x,         draw_y,         8*scale, 8*scale, *pixmap, quarters_x[0]*8, quarters_y[0]*8, 8, 8);
        painter->drawPixmap(draw_x+8*scale, draw_y,         8*scale, 8*scale, *pixmap, quarters_x[1]*8, quarters_y[1]*8, 8, 8);
        painter->drawPixmap(draw_x,         draw_y+8*scale, 8*scale, 8*scale, *pixmap, quarters_x[2]*8, quarters_y[2]*8, 8, 8);
        painter->drawPixmap(draw_x+8*scale, draw_y+8*scale, 8*scale, 8*scale, *pixmap, quarters_x[3]*8, quarters_y[3]*8, 8, 8);
    } else {
        // 16x16 tiles
        painter->drawPixmap(draw_x, draw_y, 16*scale, 16*scale, *pixmap, quarters_x[0]*16, quarters_y[0]*16, 16, 16);
    }
}

void TilemapTownMapView::paintEvent(QPaintEvent *)
{
    if (this->tilemapTownClient == nullptr || !this->tilemapTownClient->map_received)
        return;
    Entity *me = this->tilemapTownClient->your_entity();
    if (!me)
        return;
    this->tilemapTownClient->camera_x = me->x * 16 + 8;
    this->tilemapTownClient->camera_y = me->y * 16 + 8;

    int viewWidthPixels = this->width();
    int viewHeightPixels = this->height();
    int viewWidthTiles = floor(viewWidthPixels / (16 * this->scale));
    int viewHeightTiles = floor(viewHeightPixels / (16 * this->scale));
    int pixelCameraX = round(this->tilemapTownClient->camera_x * this->scale - viewWidthPixels / 2);
    int pixelCameraY = round(this->tilemapTownClient->camera_y * this->scale - viewHeightPixels / 2);

    int offsetX = positive_modulo(pixelCameraX, 16*this->scale);
    int offsetY = positive_modulo(pixelCameraY, 16*this->scale);
    int tileX = floor(pixelCameraX / (16.0 * this->scale));
    int tileY = floor(pixelCameraY / (16.0 * this->scale));

    QPainter painter(this);
    {
        QPainterStateGuard guard(&painter);

        ///////////////////////////////////////////////////////////////////////
        // Display map, and non-"over" objects
        ///////////////////////////////////////////////////////////////////////

        for (int y = 0; y < (viewHeightTiles + 2); y++) {
            for (int x = 0; x < (viewWidthTiles + 2); x++) {
                int mapCoordX = x + tileX;
                int mapCoordY = y + tileY;

                if(mapCoordX < 0 || mapCoordY < 0
                    || mapCoordX >= this->tilemapTownClient->town_map.width
                    || mapCoordY >= this->tilemapTownClient->town_map.height)
                    continue;

                struct MapCell &cell = this->tilemapTownClient->town_map.cells[mapCoordY * this->tilemapTownClient->town_map.width + mapCoordX];
                MapTileInfo *turf = cell.turf.get(this->tilemapTownClient);
                if (turf) {
                    this->drawMapTile(&painter, turf, false, mapCoordX, mapCoordY,
                        x*16*this->scale-offsetX, y*16*this->scale-offsetY, this->scale);
                }
                for (struct MapTileReference &tile : cell.objs) {
                    MapTileInfo *obj = tile.get(this->tilemapTownClient);
                    if (obj && !obj->over) {
                        this->drawMapTile(&painter, obj, true, mapCoordX, mapCoordY,
                            x*16*this->scale-offsetX, y*16*this->scale-offsetY, this->scale);
                    }
                }
            }
        }

        ///////////////////////////////////////////////////////////////////////
        // Display entities
        ///////////////////////////////////////////////////////////////////////

        std::vector<Entity*> sorted_entities;
        for(auto& [key, entity] : this->tilemapTownClient->who) {
            sorted_entities.push_back(&entity);
        }
        std::sort(sorted_entities.begin(), sorted_entities.end(), sort_entity_by_y_pos);

        for(auto& entity : sorted_entities) {
            //if(entity->walk_timer)
            //    entity->walk_timer--;
            if(
                (entity->x < (tileX - 3)) ||
                (entity->y < (tileY - 3)) ||
                (entity->x > (tileX + viewWidthTiles + 3)) ||
                (entity->y > (tileY + viewHeightTiles + 3))
                )
                continue;
            const QPixmap *pixmap = entity->pic.get_pixmap(this->tilemapTownClient);
            if(pixmap) {
                int tileset_width  = pixmap->width();
                int tileset_height = pixmap->height();

                if(tileset_width == 16 && tileset_height == 16) {
                    painter.drawPixmap(
                        (entity->x*16)*this->scale - pixelCameraX + entity->offset_x*this->scale,
                        (entity->y*16)*this->scale - pixelCameraY + entity->offset_y*this->scale,
                        16*scale,
                        16*scale,
                        *pixmap,
                        0*16, 0*16, 16, 16
                    );
                } else if(entity->pic.key_is_url()) {
                    int frame_x = 0, frame_y = 0;
                    bool is_walking = false; //entity->walk_timer != 0;
                    const int tenth_of_second_counter = 0; // TODO

                    switch(tileset_height / 32) { // Directions
                    case 2: frame_y = entity->direction_lr / 4; break;
                    case 4: frame_y = entity->direction_4 / 2; break;
                    case 8: frame_y = entity->direction; break;
                    }
                    switch(tileset_width / 32) { // Frames per direction
                    case 2: frame_x = (is_walking * 1); break;
                    case 4: frame_x = (is_walking * 2) + ((tenth_of_second_counter/2) & 1); break;
                    case 6: frame_x = (is_walking * 3) + ((tenth_of_second_counter/2) % 3); break;
                    case 8: frame_x = (is_walking * 4) + ((tenth_of_second_counter/2) & 3); break;
                    }

                    painter.drawPixmap(
                        (entity->x*16-8)*this->scale - pixelCameraX + entity->offset_x*this->scale,
                        (entity->y*16-16)*this->scale - pixelCameraY + entity->offset_y*this->scale,
                        32*scale,
                        32*scale,
                        *pixmap,
                        frame_x*32, frame_y*32, 32, 32
                        );
                } else {
                    painter.drawPixmap(
                        (entity->x*16)*this->scale - pixelCameraX + entity->offset_x*this->scale,
                        (entity->y*16)*this->scale - pixelCameraY + entity->offset_y*this->scale,
                        16*scale,
                        16*scale,
                        *pixmap,
                        entity->pic.x*16, entity->pic.y*16, 16, 16
                        );
                }

            }
        }

        ///////////////////////////////////////////////////////////////////////
        // Display only "over" objects
        ///////////////////////////////////////////////////////////////////////

        for (int y = 0; y < (viewHeightTiles + 2); y++) {
            for (int x = 0; x < (viewWidthTiles + 2); x++) {
                int mapCoordX = x + tileX;
                int mapCoordY = y + tileY;

                if(mapCoordX < 0 || mapCoordY < 0
                    || mapCoordX >= this->tilemapTownClient->town_map.width
                    || mapCoordY >= this->tilemapTownClient->town_map.height)
                    continue;

                struct MapCell &cell = this->tilemapTownClient->town_map.cells[mapCoordY * this->tilemapTownClient->town_map.width + mapCoordX];
                for (struct MapTileReference &tile : cell.objs) {
                    MapTileInfo *obj = tile.get(this->tilemapTownClient);
                    if (obj && obj->over) {
                        this->drawMapTile(&painter, obj, true, mapCoordX, mapCoordY,
                                          x*16*this->scale-offsetX, y*16*this->scale-offsetY, this->scale);
                    }
                }
            }
        }
    }
}

void TilemapTownMapView::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_Left:
    case Qt::Key_A:
        if (event->modifiers() == Qt::ShiftModifier) {
            this->tilemapTownClient->turn_player(4);
        } else if (event->modifiers() == Qt::ControlModifier) {
            this->tilemapTownClient->offset_player(-1, 0);
        } else {
            this->tilemapTownClient->move_player(-1, 0);
        }
        event->accept();
        this->update();
        break;
    case Qt::Key_Right:
    case Qt::Key_D:
        if (event->modifiers() == Qt::ShiftModifier) {
            this->tilemapTownClient->turn_player(0);
        } else if (event->modifiers() == Qt::ControlModifier) {
            this->tilemapTownClient->offset_player(1, 0);
        } else {
            this->tilemapTownClient->move_player(1, 0);
        }
        event->accept();
        this->update();
        break;
    case Qt::Key_Up:
    case Qt::Key_W:
        if (event->modifiers() == Qt::ShiftModifier) {
            this->tilemapTownClient->turn_player(6);
        } else if (event->modifiers() == Qt::ControlModifier) {
            this->tilemapTownClient->offset_player(0, -1);
        } else {
            this->tilemapTownClient->move_player(0, -1);
        }
        event->accept();
        this->update();
        break;
    case Qt::Key_Down:
    case Qt::Key_S:
        if (event->modifiers() == Qt::ShiftModifier) {
            this->tilemapTownClient->turn_player(2);
        } else if (event->modifiers() == Qt::ControlModifier) {
            this->tilemapTownClient->offset_player(0, 1);
        } else {
            this->tilemapTownClient->move_player(0, 1);
        }
        event->accept();
        this->update();
        break;
    case Qt::Key_PageUp:
        if (event->modifiers() == Qt::ShiftModifier) {
            this->tilemapTownClient->turn_player(7);
        } else if (event->modifiers() == Qt::ControlModifier) {
            this->tilemapTownClient->offset_player(1, -1);
        } else {
            this->tilemapTownClient->move_player(1, -1);
        }
        event->accept();
        this->update();
        break;
    case Qt::Key_PageDown:
        if (event->modifiers() == Qt::ShiftModifier) {
            this->tilemapTownClient->turn_player(1);
        } else if (event->modifiers() == Qt::ControlModifier) {
            this->tilemapTownClient->offset_player(1, 1);
        } else {
            this->tilemapTownClient->move_player(1, 1);
        }
        event->accept();
        this->update();
        break;
    case Qt::Key_End:
        if (event->modifiers() == Qt::ShiftModifier) {
            this->tilemapTownClient->turn_player(3);
        } else if (event->modifiers() == Qt::ControlModifier) {
            this->tilemapTownClient->offset_player(-1, 1);
        } else {
            this->tilemapTownClient->move_player(-1, 1);
        }
        event->accept();
        this->update();
        break;
    case Qt::Key_Home:
        if (event->modifiers() == Qt::ShiftModifier) {
            this->tilemapTownClient->turn_player(5);
        } else if (event->modifiers() == Qt::ControlModifier) {
            this->tilemapTownClient->offset_player(-1, -1);
        } else {
            this->tilemapTownClient->move_player(-1, -1);
        }
        event->accept();
        this->update();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (!event->isAutoRepeat())
            emit this->focusChat();
        event->accept();
        break;
    default:
        break;
    }
}
