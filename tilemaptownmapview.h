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
#ifndef TILEMAPTOWNMAPVIEW_H
#define TILEMAPTOWNMAPVIEW_H

#include <QWidget>
#include "town.h"

class TilemapTownMapView : public QWidget
{
    Q_OBJECT
public:
    explicit TilemapTownMapView(QWidget *parent = nullptr);
    TilemapTownClient *tilemapTownClient;
    int scale = 2;

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;
private:
    void drawMapTile(QPainter *painter, const MapTileInfo *tiletile, bool obj, int map_x, int map_y, float draw_x, float draw_y, int scale);
signals:
    void focusChat();
};

#endif // TILEMAPTOWNMAPVIEW_H
