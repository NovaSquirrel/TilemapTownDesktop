#include "mainwindow.h"
#include "cJSON.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    connect(&this->tilemapTownClient, &TilemapTownClient::log_message, this, &MainWindow::logMessage);
    this->tilemapTownClient.http = &this->townFileCache;

    ui->setupUi(this);
    ui->tabCharacters->addTab("Character");

    int tabAll = ui->tabChatChannels->addTab("All");
    //int tabFocus = ui->tabChatChannels->addTab("Focus");

    ui->tabChatChannels->setTabsClosable(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionConnect_to_a_server_triggered()
{
    this->tilemapTownClient.websocket_connect("novasquirrel.com", "/townws_test/", "443");
    ui->tilemapTownMapView->tilemapTownClient = &this->tilemapTownClient;
}

void MainWindow::on_textInput_returnPressed()
{
    QString text = ui->textInput->toPlainText();
    if (text.length() == 0)
        return;

    cJSON *json = cJSON_CreateObject();
    if(text == "/clear") {
        ui->chatLog->clear();
    } else if(text.startsWith("//")) {
        cJSON_AddStringToObject(json, "text", text.remove(0, 1).toUtf8());
        this->tilemapTownClient.websocket_write("MSG", json);
    } else if(text.startsWith("/") && !text.startsWith("/me ") && !text.startsWith("/ooc ") && !text.startsWith("/spoof ")) {
        cJSON_AddStringToObject(json, "text", text.remove(0, 1).toUtf8());
        this->tilemapTownClient.websocket_write("CMD", json);
    } else {
        cJSON_AddStringToObject(json, "text", text.toUtf8());
        this->tilemapTownClient.websocket_write("MSG", json);
    }
    cJSON_Delete(json);

    ui->textInput->clear();
}

void MainWindow::logMessage(std::string text, std::string style) {
    ui->chatLog->append(QString::fromStdString((text)));
}
