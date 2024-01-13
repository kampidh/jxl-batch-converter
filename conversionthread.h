/*
 * SPDX-FileCopyrightText: 2023 Rasyuqa A H <qampidh@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 **/

#ifndef CONVERSIONTHREAD_H
#define CONVERSIONTHREAD_H

#include "logcodes.h"

#include <QProcess>
#include <QDirIterator>
#include <QThread>
#include <QMutex>
#include <QMap>
#include <QColor>
#include <QWaitCondition>
#include <QTimerEvent>

class ConversionThread : public QThread
{
    Q_OBJECT
public:
    ConversionThread(QObject *parent = nullptr);
    ~ConversionThread();

    int processFiles(const QString &cjxlbin, QDirIterator &dit, const QString &fout, const QMap<QString, QString> &args);
    int processFilesWithList(const QString &cjxlbin, const QStringList &fin, const QString &fout, const QMap<QString, QString> &args, const bool useList);
    int processFiles(const QString &cjxlbin, const QStringList &fin, const QString &fout, const QMap<QString, QString> &args);

signals:
    void sendLogs(const QString &logs, const QColor &col, const LogCode &isErr);
    void sendProgress(const float &prog);

public slots:
    void stopProcess();

protected:
    void run() override;
    void timerEvent(QTimerEvent *event) override;

private:
    void initArgs(const QMap<QString, QString> &args);
    void calculateStats();
    void resetValues();
    bool runCjxl(QProcess &jxlBin, const QFileInfo &fin, const QString &fout);

    bool m_isJpegTran = false;
    bool m_isOverwrite = false;
    bool m_isSilent = false;
    bool m_disableOutput = false;
    bool m_stopOnError = false;
    bool m_copyOnError = false;
    bool m_haveCustomArgs = false;
    bool m_useFileList = false;

    double m_averageMps = 0.0;
    int m_mpsSamples = 0;
    uint m_globalTimeout = 0;
    qint64 m_totalBytesInput = 0;
    qint64 m_totalBytesOutput = 0;
    qint64 m_ticks = 0;

    QString m_cjxlbin;
    QString m_fin;
    QString m_fout;
    QString m_extension;
    QStringList m_args;
    QStringList m_finBatch;
    QStringList m_customArgs;
    QMap<QString, QString> m_encOpts;

    QMutex mutex;
    bool m_abort = false;
};

#endif // CONVERSIONTHREAD_H
