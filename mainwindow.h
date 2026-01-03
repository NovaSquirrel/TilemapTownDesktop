#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "town.h"
#include "townfilecache.h"
#include "connecttoserverdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    TilemapTownClient tilemapTownClient;
    TownFileCache townFileCache;

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionConnect_to_a_server_triggered();
    void on_actionSource_code_triggered();
    void on_actionQuit_triggered();
    void on_actionDisconnect_triggered();
    void on_actionReconnect_triggered();
    void on_actionZoom_out_triggered();
    void on_actionZoom_in_triggered();
    void on_actionReset_zoom_triggered();
    void on_actionWalk_through_walls_triggered();
    void on_tilemapTownMapView_focusChat();
    void on_tilemapTownMapView_movedPlayer();
    void on_textInput_returnPressed();
    void logMessage(std::string text, std::string style);
    void didConnectToServerDialog(QString websocket_server, QString town_nickname, QString town_username, QString town_password, bool guest_mode);
    void connected_to_server();
    void want_redraw();

private:
    Ui::MainWindow *ui;
    ConnectToServerDialog connectToServerDialog;

    // Connection variables
    QString websocket_server, town_nickname, town_username, town_password;
    bool guest_mode = true;
};
#endif // MAINWINDOW_H
