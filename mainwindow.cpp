#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tabCharacters->addTab("Character");

    int tabAll = ui->tabChatChannels->addTab("All");
    int tabFocus = ui->tabChatChannels->addTab("Focus");

    ui->tabChatChannels->setTabsClosable(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}
