#include <QDesktopServices>
#include "mainwindow.h"
#include "cJSON.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    connect(&this->tilemapTownClient, &TilemapTownClient::log_message, this, &MainWindow::logMessage);
    connect(&this->tilemapTownClient, &TilemapTownClient::connected_to_server, this, &MainWindow::connected_to_server);
    this->tilemapTownClient.http = &this->townFileCache;

    // Set up tabs and UI
    ui->setupUi(this);
    ui->tabCharacters->addTab("Character");
    int tabAll = ui->tabChatChannels->addTab("All");
    //int tabFocus = ui->tabChatChannels->addTab("Focus");
    ui->tabChatChannels->setTabsClosable(true);

    // Initialize server settings
    this->websocket_server = "wss://novasquirrel.com/townws_test/:443";
    this->town_nickname = "qt";
    this->town_username = "";
    this->town_password = "";
    this->guest_mode = true;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionConnect_to_a_server_triggered()
{
    ui->tilemapTownMapView->tilemapTownClient = &this->tilemapTownClient;
    this->connectToServerDialog.initializeInputFields(this->websocket_server, this->town_nickname, this->town_username, this->town_password, this->guest_mode);

    connect(&this->connectToServerDialog, &ConnectToServerDialog::returnResults, this, &MainWindow::didConnectToServerDialog, Qt::UniqueConnection);
    this->connectToServerDialog.show();
}

void MainWindow::on_actionReconnect_triggered()
{
    if (this->tilemapTownClient.connected) {
        this->tilemapTownClient.websocket_disconnect();
    }
    this->tilemapTownClient.websocket_connect(this->websocket_server.toStdString());
}

void MainWindow::on_actionSource_code_triggered()
{
    QDesktopServices::openUrl ( QUrl("https://github.com/NovaSquirrel/TilemapTownDesktop/") );
}

void MainWindow::on_actionQuit_triggered()
{
    this->close();
    //this->connectToServerDialog.close();
}

void MainWindow::on_actionDisconnect_triggered()
{
    this->tilemapTownClient.websocket_disconnect();
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

void MainWindow::connected_to_server() {
    if (this->guest_mode) {
        this->tilemapTownClient.login(this->town_nickname.toUtf8(), nullptr);
    } else {
        this->tilemapTownClient.login(this->town_username.toUtf8(), this->town_password.toUtf8());
    }
}

void MainWindow::didConnectToServerDialog(QString websocket_server, QString town_nickname, QString town_username, QString town_password, bool guest_mode) {
    this->websocket_server = websocket_server;
    this->town_nickname = town_nickname;
    this->town_username = town_username;
    this->town_password = town_password;
    this->guest_mode = guest_mode;
    this->tilemapTownClient.websocket_connect(websocket_server.toStdString());
}
