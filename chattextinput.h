#ifndef CHATTEXTINPUT_H
#define CHATTEXTINPUT_H

#include <QObject>
#include <QPlainTextEdit>
#include <QKeyEvent>

class ChatTextInput : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit ChatTextInput(QWidget* parent = nullptr) : QPlainTextEdit(parent) {}
signals:
    void returnPressed();
protected:
    void keyPressEvent(QKeyEvent* event) override;
};

#endif // CHATTEXTINPUT_H
