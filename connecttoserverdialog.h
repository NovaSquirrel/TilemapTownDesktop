#ifndef CONNECTTOSERVERDIALOG_H
#define CONNECTTOSERVERDIALOG_H

#include <QDialog>

namespace Ui {
class ConnectToServerDialog;
}

class ConnectToServerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectToServerDialog(QWidget *parent = nullptr);
    ~ConnectToServerDialog();
    void initializeInputFields(QString websocket_server, QString town_nickname, QString town_username, QString town_password, bool guest_mode);
private Q_SLOTS:
    void buttonClicked(bool checked);
signals:
    void returnResults(QString websocket_server, QString town_nickname, QString town_username, QString town_password, bool guest_mode);
private:
    Ui::ConnectToServerDialog *ui;
};

#endif // CONNECTTOSERVERDIALOG_H
