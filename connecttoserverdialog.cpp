#include "connecttoserverdialog.h"
#include "ui_connecttoserverdialog.h"

ConnectToServerDialog::ConnectToServerDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConnectToServerDialog)
{
    ui->setupUi(this);
    connect(this->ui->connectButton, &QPushButton::clicked, this, &ConnectToServerDialog::buttonClicked);
    this->setFixedSize(size());
}

ConnectToServerDialog::~ConnectToServerDialog()
{
    delete ui;
}

void ConnectToServerDialog::initializeInputFields(QString websocket_server, QString town_nickname, QString town_username, QString town_password, bool guest_mode) {
    this->ui->lineServer->setText(websocket_server);
    this->ui->lineNickname->setText(town_nickname);
    this->ui->lineUsername->setText(town_username);
    this->ui->linePassword->setText(town_password);
    this->ui->guestOrAccountTabs->setCurrentIndex(guest_mode?0:1);
}

void ConnectToServerDialog::buttonClicked(bool checked = false) {
    this->returnResults(this->ui->lineServer->text(), this->ui->lineNickname->text(), this->ui->lineUsername->text(), this->ui->linePassword->text(), this->ui->guestOrAccountTabs->currentIndex() == 0);
    this->close();
}
