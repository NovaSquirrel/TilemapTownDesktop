#ifndef TILEMAPTOWNMAPVIEW_H
#define TILEMAPTOWNMAPVIEW_H

#include <QWidget>

class TilemapTownMapView : public QWidget
{
    Q_OBJECT
public:
    explicit TilemapTownMapView(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

signals:
};

#endif // TILEMAPTOWNMAPVIEW_H
