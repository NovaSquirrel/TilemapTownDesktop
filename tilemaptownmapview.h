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

signals:
};

#endif // TILEMAPTOWNMAPVIEW_H
