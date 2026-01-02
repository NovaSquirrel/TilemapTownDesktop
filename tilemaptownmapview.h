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

protected:
    void paintEvent(QPaintEvent *event) override;
private:
    void drawMapTile(QPainter *painter, const MapTileInfo *tiletile, bool obj, int map_x, int map_y, float draw_x, float draw_y, int scale);
signals:
};

#endif // TILEMAPTOWNMAPVIEW_H
