/*
 * SPDX-FileCopyrightText: 2023 Rasyuqa A H <qampidh@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 **/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "logcodes.h"

#include <QColor>
#include <QMainWindow>

#include "ui_mainwindow.h"

class MainWindow : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void cjxlChecker();

    class Private;
    Private *const d{nullptr};

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void libjxlBtnPressed();
    void inputBtnPressed();
    void outputBtnPressed();
    void convertBtnPressed();
    void printHelpBtnPressed();
    void tabIndexChanged(const int &index);

    void resetUi();
    void dirChkChange();
    void dumpLogs(const QString &logs, const QColor &col, const LogCode &isErr);
    void dumpProgress(const float &prog);
};
#endif // MAINWINDOW_H
