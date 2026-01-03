#include "chattextinput.h"

void ChatTextInput::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (!event->isAutoRepeat())
            emit returnPressed();
        event->accept(); // Don't propagate the event
    } else {
        QPlainTextEdit::keyPressEvent(event);
    }
}
