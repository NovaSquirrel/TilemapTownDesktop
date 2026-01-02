// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "tilemaptownmapview.h"
#include "town.h"

#include <QPainter>
#include <QPainterStateGuard>
//#include <QTime>
//#include <QTimer>

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
    if (this->tilemapTownClient == nullptr)
        return;

    const int scale = 2;

    QPainter painter(this);
    {
        QPainterStateGuard guard(&painter);
        for(int y=0; y<20; y++) {
            for(int x=0; x<20; x++) {
                struct MapCell &cell = this->tilemapTownClient->town_map.cells[y * this->tilemapTownClient->town_map.width + x];
                MapTileInfo *turf = cell.turf.get(this->tilemapTownClient);
                if (turf) {
                    this->drawMapTile(&painter, turf, false, x, y, x*16*scale, y*16*scale, scale);
                }
                for (struct MapTileReference &tile : cell.objs) {
                    MapTileInfo *obj = tile.get(this->tilemapTownClient);
                    if (obj) {
                        this->drawMapTile(&painter, obj, true, x, y, x*16*scale, y*16*scale, scale);
                    }
                }
            }
        }
    }
}
