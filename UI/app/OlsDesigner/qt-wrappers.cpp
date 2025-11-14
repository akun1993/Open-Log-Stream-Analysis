/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "qt-wrappers.h"
#include <QComboBox>
#include <QDataStream>
#include <QFileDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QWidget>

//#include <util/threading.h>

//#if !defined(_WIN32) && !defined(__APPLE__)
//#include <obs-nix-platform.h>
//#endif

static inline void OLSErrorBoxva(QWidget* parent, const char* msg, va_list args)
{
    char full_message[4096];
    vsnprintf(full_message, 4095, msg, args);

    QString message = QString::fromUtf8(full_message);

    QMessageBox::critical(parent, "Error", message);
}

void OLSErrorBox(QWidget* parent, const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    OLSErrorBoxva(parent, msg, args);
    va_end(args);
}

QMessageBox::StandardButton
OLSMessageBox::question(QWidget* parent, const QString& title,
    const QString& text,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton)
{
    QMessageBox mb(QMessageBox::Question, title, text,
        QMessageBox::NoButton, parent);
    mb.setDefaultButton(defaultButton);

    if (buttons & QMessageBox::Ok) {
        QPushButton* button = mb.addButton(QMessageBox::Ok);
        button->setText("OK");
    }
#define add_button(x)                                       \
    if (buttons & QMessageBox::x) {                         \
        QPushButton* button = mb.addButton(QMessageBox::x); \
        button->setText(#x);                                \
    }
    add_button(Open);
    add_button(Save);
    add_button(Cancel);
    add_button(Close);
    add_button(Discard);
    add_button(Apply);
    add_button(Reset);
    add_button(Yes);
    add_button(No);
    add_button(Abort);
    add_button(Retry);
    add_button(Ignore);
#undef add_button
    return (QMessageBox::StandardButton)mb.exec();
}

void OLSMessageBox::information(QWidget* parent, const QString& title,
    const QString& text)
{
    QMessageBox mb(QMessageBox::Information, title, text,
        QMessageBox::NoButton, parent);
    mb.addButton(("OK"), QMessageBox::AcceptRole);
    mb.exec();
}

void OLSMessageBox::warning(QWidget* parent, const QString& title,
    const QString& text, bool enableRichText)
{
    QMessageBox mb(QMessageBox::Warning, title, text, QMessageBox::NoButton,
        parent);
    if (enableRichText)
        mb.setTextFormat(Qt::RichText);
    mb.addButton(("OK"), QMessageBox::AcceptRole);
    mb.exec();
}

void OLSMessageBox::critical(QWidget* parent, const QString& title,
    const QString& text)
{
    QMessageBox mb(QMessageBox::Critical, title, text,
        QMessageBox::NoButton, parent);
    mb.addButton(("OK"), QMessageBox::AcceptRole);
    mb.exec();
}

void DeleteLayout(QLayout* layout)
{
    if (!layout)
        return;

    for (;;) {
        QLayoutItem* item = layout->takeAt(0);
        if (!item)
            break;

        QLayout* subLayout = item->layout();
        if (subLayout) {
            DeleteLayout(subLayout);
        } else {
            delete item->widget();
            delete item;
        }
    }

    delete layout;
}

class QuickThread : public QThread {
public:
    explicit inline QuickThread(std::function<void()> func_)
        : func(func_)
    {
    }

private:
    virtual void run() override { func(); }

    std::function<void()> func;
};

QThread* CreateQThread(std::function<void()> func)
{
    return new QuickThread(func);
}

std::atomic<long> insideEventLoop { 0 };

void ExecuteFuncSafeBlock(std::function<void()> func)
{
    QEventLoop eventLoop;

    auto wait = [&]() {
        func();
        QMetaObject::invokeMethod(&eventLoop, "quit",
            Qt::QueuedConnection);
    };

    ++insideEventLoop;

    QScopedPointer<QThread> thread(CreateQThread(wait));
    thread->start();
    eventLoop.exec();
    thread->wait();
    --insideEventLoop;
}

void ExecuteFuncSafeBlockMsgBox(std::function<void()> func,
    const QString& title, const QString& text)
{
    QMessageBox dlg;
    dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowCloseButtonHint);
    dlg.setWindowTitle(title);
    dlg.setText(text);
    dlg.setStandardButtons(QMessageBox::StandardButtons());

    auto wait = [&]() {
        func();
        QMetaObject::invokeMethod(&dlg, "accept", Qt::QueuedConnection);
    };

    ++insideEventLoop;
    QScopedPointer<QThread> thread(CreateQThread(wait));
    thread->start();
    dlg.exec();
    thread->wait();
    --insideEventLoop;
}

static bool enable_message_boxes = false;

void EnableThreadedMessageBoxes(bool enable)
{
    enable_message_boxes = enable;
}

void ExecThreadedWithoutBlocking(std::function<void()> func,
    const QString& title, const QString& text)
{
    if (!enable_message_boxes)
        ExecuteFuncSafeBlock(func);
    else
        ExecuteFuncSafeBlockMsgBox(func, title, text);
}

bool LineEditCanceled(QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = reinterpret_cast<QKeyEvent*>(event);
        return keyEvent->key() == Qt::Key_Escape;
    }

    return false;
}

bool LineEditChanged(QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = reinterpret_cast<QKeyEvent*>(event);

        switch (keyEvent->key()) {
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
        case Qt::Key_Enter:
        case Qt::Key_Return:
            return true;
        }
    } else if (event->type() == QEvent::FocusOut) {
        return true;
    }

    return false;
}

void SetComboItemEnabled(QComboBox* c, int idx, bool enabled)
{
    QStandardItemModel* model = dynamic_cast<QStandardItemModel*>(c->model());
    QStandardItem* item = model->item(idx);
    item->setFlags(enabled ? Qt::ItemIsSelectable | Qt::ItemIsEnabled
                           : Qt::NoItemFlags);
}

void setThemeID(QWidget* widget, const QString& themeID)
{
    if (widget->property("themeID").toString() != themeID) {
        widget->setProperty("themeID", themeID);

        /* force style sheet recalculation */
        QString qss = widget->styleSheet();
        widget->setStyleSheet("/* */");
        widget->setStyleSheet(qss);
    }
}

QString SelectDirectory(QWidget* parent, QString title, QString path)
{
    QString dir = QFileDialog::getExistingDirectory(
        parent, title, path,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    return dir;
}

QString SaveFile(QWidget* parent, QString title, QString path,
    QString extensions)
{
    QString file = QFileDialog::getSaveFileName(parent, title, path, extensions);

    return file;
}

QString OpenFile(QWidget* parent, QString title, QString path,
    QString extensions)
{
    QString file = QFileDialog::getOpenFileName(parent, title, path, extensions);

    return file;
}

QStringList OpenFiles(QWidget* parent, QString title, QString path,
    QString extensions)
{
    QStringList files = QFileDialog::getOpenFileNames(parent, title, path, extensions);

    return files;
}

static void SetLabelText(QLabel* label, const QString& newText)
{
    if (label->text() != newText)
        label->setText(newText);
}

void TruncateLabel(QLabel* label, QString newText, int length)
{
    if (newText.length() < length) {
        label->setToolTip(QString());
        SetLabelText(label, newText);
        return;
    }

    label->setToolTip(newText);
    newText.truncate(length);
    newText += "...";

    SetLabelText(label, newText);
}
