#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "town.h"
#include "townfilecache.h"

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
    void on_textInput_returnPressed();
    void logMessage(std::string text, std::string style);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
