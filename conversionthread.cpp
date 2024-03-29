/*
 * SPDX-FileCopyrightText: 2023 Rasyuqa A H <qampidh@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 **/

#include "conversionthread.h"

#include <QDateTime>
#include <QDebug>
#include <QProcess>
#include <QMapIterator>
#include <QRegularExpression>

#define TICKS 1
#define TICKS_MULTIPLIER 10
#define POLL_RATE_MS 100

ConversionThread::ConversionThread(QObject *parent)
    : QThread(parent)
{
    resetValues();
}

ConversionThread::~ConversionThread()
{
    mutex.lock();
    m_abort = true;
    mutex.unlock();

    wait();
}

void ConversionThread::initArgs(const QMap<QString, QString> &args)
{
    m_encOpts.clear();

    m_extension = QString(".jxl");

    QMapIterator<QString, QString> mit(args);
    while (mit.hasNext()) {
        mit.next();

        if (mit.key() == "-j" && mit.value() == "1") {
            m_isJpegTran = true;
        }

        if (mit.key() == "overwrite" && mit.value() == "1") {
            m_isOverwrite = true;
        }

        if (mit.key() == "silent" && mit.value() == "1") {
            m_isSilent = true;
        }

        if (mit.key() == "outFormat") {
            m_extension = mit.value();
        }

        if (mit.key() == "globalTimeout") {
            m_globalTimeout = mit.value().toUInt();
        }

        if (mit.key() == "globalStopOnError" && mit.value() == "1") {
            m_stopOnError = true;
        }

        if (mit.key() == "globalCopyOnError" && mit.value() == "1") {
            m_copyOnError = true;
        }

        if (mit.key() == "useMultithread" && mit.value() == "1") {
            m_isMultithread = true;
        }

        if (mit.key() == "keepDateTime" && mit.value() == "1") {
            m_keepDateTime = true;
        }

        if (mit.key() == "customFlags") {
            if (mit.value().contains("disable_output")) {
                m_disableOutput = true;
            }
            m_haveCustomArgs = true;
            static const QRegularExpression regEmpty("\\s+");
            const QStringList addArgs = mit.value().split(regEmpty, Qt::SkipEmptyParts);
            for (const QString &ar : qAsConst(addArgs)) {
                m_customArgs << ar;
            }
        }

        if (mit.key() == "directoryInput") {
            m_fin = mit.value();
        }

        m_encOpts.insert(mit.key(), mit.value());
    }

    startTimer(TICKS * TICKS_MULTIPLIER);
}

int ConversionThread::processFiles(const QString &cjxlbin,
                                   const QStringList &fin,
                                   const QString &fout,
                                   const QMap<QString, QString> &args)
{
    m_cjxlbin = cjxlbin;
    m_finBatch.clear();

    int numfiles = 0;

    for (const QString &fi : fin) {
        m_finBatch.append(fi);
        numfiles++;
    }

    m_fout = fout;

    resetValues();

    // m_useFileList = true;

    initArgs(args);

    return numfiles;
}

int ConversionThread::processFilesWithList(const QString &cjxlbin,
                                           const QStringList &fin,
                                           const QString &fout,
                                           const QMap<QString, QString> &args,
                                           const bool useList)
{
    m_cjxlbin = cjxlbin;
    m_finBatch.clear();

    int numfiles = 0;

    for (const QString &fi : fin) {
        m_finBatch.append(fi);
        numfiles++;
    }

    m_fout = fout;

    resetValues();

    m_useFileList = useList;

    initArgs(args);

    return numfiles;
}

int ConversionThread::processFiles(const QString &cjxlbin,
                                    QDirIterator &dit,
                                    const QString &fout,
                                    const QMap<QString, QString> &args)
{
    m_cjxlbin = cjxlbin;
    m_finBatch.clear();

    int numfiles = 0;

    while (dit.hasNext()) {
        /*
         * Ditto :3
         *
         * Don't include output folder on input, or else it will go bang..
         */
        const QString ditto = dit.next();
        if (!ditto.contains(fout)) {
            m_finBatch.append(ditto);
            numfiles++;
        }
    }

    m_fout = fout;

    resetValues();

    initArgs(args);

    return numfiles;
}

void ConversionThread::resetValues()
{
    m_abort = false;
    m_useFileList = false;
    m_isJpegTran = false;
    m_isOverwrite = false;
    m_isSilent = false;
    m_disableOutput = false;
    m_stopOnError = false;
    m_copyOnError = false;
    m_haveCustomArgs = false;
    m_isMultithread = false;
    m_keepDateTime = false;

    m_customArgs.clear();

    m_averageMps = 0.0;
    m_mpsSamples = 0;
    m_globalTimeout = 0;
    m_totalBytesInput = 0;
    m_totalBytesOutput = 0;
    m_ticks = 0;
}

void ConversionThread::run()
{
    /*
     * Process in a thread?! I must be running wild...
     *
     * I can go away using QObject instead, but I kinda risking calling
     * too many processEvents() when waiting the app to complete while still
     * needing to interrupt the process anytime. So here it is...
     */
    QProcess cjxlBin;

    QString inFUrl;
    inFUrl = m_fin;

    QFileInfo inFileFirst(inFUrl);

    QDir inUrl;
    if (inFileFirst.isFile()) {
        inUrl.setPath(inFileFirst.absolutePath());
    } else {
        inUrl.setPath(inFileFirst.absoluteFilePath());
    }

    const QString basePath = inUrl.absolutePath();

    // emit sendProgress(0);

    int sizeIter = 1;
    const int batchSize = m_finBatch.size();
    for (const QString &fin : qAsConst(m_finBatch)) {
        if (m_abort) {
            emit sendLogs(QString("Aborted\n"), errLogCol, LogCode::INFO);
            calculateStats();
            return;
        }

        const QFileInfo inFile(fin);

        const QString extraDirName = QString(inFile.absolutePath()).remove(basePath);
        const QDir outFUrl = [&]() {
            if (m_useFileList) {
                return QDir::cleanPath(m_fout);
            }
            return QDir::cleanPath(m_fout + extraDirName);
        }();
        if (!outFUrl.exists()) {
            if (!outFUrl.mkpath(".") && !outFUrl.exists()) {
                if (!m_isMultithread) {
                    const QString head =
                        QString("Processing image(s) %2/%3:\n%1")
                            .arg(inFile.absoluteFilePath(), QString::number(sizeIter), QString::number(batchSize));
                    emit sendLogs(head, Qt::white, LogCode::FILE_IN);
                } else {
                    const QString head =
                        QString("Processing image(s):\n%1")
                            .arg(inFile.absoluteFilePath());
                    emit sendLogs(head, Qt::white, LogCode::FILE_IN);
                }
                emit sendLogs(QString("Failed to create subfolder at %1").arg(outFUrl.absolutePath()),
                              errLogCol,
                              LogCode::OUT_FOLDER_ERR);
                emit sendLogs(QString("Skipping..."), errLogCol, LogCode::INFO);

                sizeIter++;
                emit sendProgress(sizeIter);

                continue;
            }
        }

        const QString outFName = inFile.completeBaseName() + m_extension;
        const QString outFPath = QDir::cleanPath(outFUrl.path() + QDir::separator() + outFName);

        const QFileInfo outFile(outFPath);
        if (!m_isOverwrite && outFile.exists()) {
            if (!m_isSilent) {
                if (!m_isMultithread) {
                    const QString head =
                        QString("Processing image(s) %2/%3:\n%1")
                            .arg(inFile.absoluteFilePath(), QString::number(sizeIter), QString::number(batchSize));
                    emit sendLogs(head, Qt::white, LogCode::FILE_IN);
                } else {
                    const QString head =
                        QString("Processing image(s):\n%1")
                            .arg(inFile.absoluteFilePath());
                    emit sendLogs(head, Qt::white, LogCode::FILE_IN);
                }

                emit sendLogs(QString("Skipped, output file already exists\n"), warnLogCol, LogCode::SKIPPED);
            } else {
                emit sendLogs(QString(), Qt::white, LogCode::FILE_IN);
                emit sendLogs(QString(), warnLogCol, LogCode::SKIPPED);
            }

            sizeIter++;
            emit sendProgress(sizeIter);

            continue;
        } else {
            if (!m_isMultithread) {
                const QString head =
                    QString("Processing image(s) %2/%3:\n%1")
                        .arg(inFile.absoluteFilePath(), QString::number(sizeIter), QString::number(batchSize));
                emit sendLogs(head, Qt::white, LogCode::FILE_IN);
            }
        }

        if (!runCjxl(cjxlBin, inFile, outFPath)) {
            calculateStats();
            return;
        }

        emit sendProgress(sizeIter);
        sizeIter++;
    }

    calculateStats();
}

bool ConversionThread::runCjxl(QProcess &cjxlBin, const QFileInfo &fin, const QString &fout)
{
    QStringList arg;

    arg << fin.absoluteFilePath() << fout;

    const bool isJpeg = [&]() {
        if (fin.suffix().contains("jpg", Qt::CaseInsensitive) || fin.suffix().contains("jpeg", Qt::CaseInsensitive)
            || fin.suffix().contains("jfif", Qt::CaseInsensitive)) {
            return true;
        }
        return false;
    }();

    QMapIterator<QString, QString> mit(m_encOpts);
    while (mit.hasNext()) {
        mit.next();

        if (isJpeg && m_isJpegTran && (mit.key() == "-d" || mit.key() == "-q"))
            continue;

        if (!mit.key().contains("-"))
            continue;

        arg << mit.key() << mit.value();
    }

    if (m_haveCustomArgs) {
        for (const QString &ar : qAsConst(m_customArgs)) {
            arg << ar;
        }
    }

    cjxlBin.start(m_cjxlbin, arg);

    const bool haveTimeout = (m_globalTimeout > 0);
    const qint64 timeout = m_globalTimeout * (1000 / (TICKS * TICKS_MULTIPLIER));

    mutex.lock();
    m_ticks = 0;
    mutex.unlock();

    // poll the abort every defined poll rate
    do {
        if (m_abort) {
            cjxlBin.kill();
            cjxlBin.waitForFinished(5000);
            emit sendLogs(QString("Aborted\n"), errLogCol, LogCode::INFO);
            return false;
        }
        if (haveTimeout) {
            if (m_ticks > timeout) {
                cjxlBin.kill();
                cjxlBin.waitForFinished(5000);
                emit sendLogs(QString("Skipped: Process exceeding set timeout of %1 second(s)\n")
                                  .arg(QString::number(m_globalTimeout)),
                              warnLogCol,
                              LogCode::SKIPPED_TIMEOUT);
                return true;
            }
        }
    } while (!cjxlBin.waitForFinished(POLL_RATE_MS));

    const bool haveErrors = (cjxlBin.exitCode() != 0);

    static const QRegularExpression newLines("\n|\r\n|\r");
    static const QRegularExpression regNum("[^0-9.]");

    const QString rawString = cjxlBin.readAllStandardError().trimmed();
    const QStringList rawStrList = rawString.split(newLines, Qt::SkipEmptyParts);

    if (m_isMultithread) {
        const QString head = QString("Processing image:\n%1").arg(fin.absoluteFilePath());
        emit sendLogs(head, Qt::white, LogCode::FILE_IN);
    }

    if (!rawStrList.isEmpty()) {
        const QString buffer = rawStrList.join("\n");

        emit sendLogs(buffer, haveErrors ? errLogCol : okayLogCol, haveErrors ? LogCode::ENCODE_ERR : LogCode::OK);

        const QString lastLine = rawStrList.last();
        if (lastLine.contains("MP/s", Qt::CaseInsensitive)) {
            const int fr = lastLine.indexOf(',');
            const int bc = lastLine.indexOf('[');

            QString mpStr = lastLine.mid(fr + 1, bc - fr - 1);

            mpStr.remove(regNum);

            const double mps = mpStr.trimmed().toDouble();

            if (mps > 0.0) {
                m_mpsSamples++;
                m_averageMps = m_averageMps + mps;
            }
        }
    }

    const QString rawStd = cjxlBin.readAllStandardOutput();
    if (!rawStd.isEmpty()) {
        emit sendLogs(rawStd, Qt::white, LogCode::INFO);
    }

    if (haveErrors && m_copyOnError) {
        emit sendLogs(QString("Copying source file to destination folder instead..."), warnLogCol, LogCode::INFO);
        const QFileInfo outp(fout);
        const QString outpfile = QDir::cleanPath(outp.absolutePath() + QDir::separator() + fin.fileName());
        if (!QFile::copy(fin.absoluteFilePath(), outpfile)) {
            if (QFile::exists(outpfile)) {
                emit sendLogs(QString("Cannot copy source file: file already exists on destination folder"), warnLogCol, LogCode::INFO);
            } else {
                emit sendLogs(QString("Failed to copy source file to destination folder"), errLogCol, LogCode::INFO);
            }
        } else {
            emit sendLogs(QString("File copied."), warnLogCol, LogCode::INFO);
            // Seems unneccessary on Windows.
            // if (m_keepDateTime) {
            //     QFile outFileOpen(outpfile);
            //     outFileOpen.open(QIODevice::ReadWrite);
            //     outFileOpen.setFileTime(fin.fileTime(QFileDevice::FileBirthTime), QFileDevice::FileBirthTime);
            //     outFileOpen.setFileTime(fin.fileTime(QFileDevice::FileAccessTime), QFileDevice::FileAccessTime);
            //     outFileOpen.setFileTime(fin.fileTime(QFileDevice::FileMetadataChangeTime), QFileDevice::FileMetadataChangeTime);
            //     outFileOpen.setFileTime(fin.fileTime(QFileDevice::FileModificationTime), QFileDevice::FileModificationTime);
            //     outFileOpen.close();
            // }
        }
        const QString outFileStr = QString("Output:\n%1\n").arg(outpfile);
        emit sendLogs(outFileStr, Qt::white, LogCode::INFO);
    }

    if (haveErrors && m_stopOnError) {
        emit sendLogs(QString("Aborted: Batch set to stop on error\n"), errLogCol, LogCode::INFO);
        return false;
    }

    QFileInfo inFile(fin);
    QFileInfo outFile(fout);
    if (inFile.exists() && outFile.exists() && !m_disableOutput) {
        m_totalBytesInput += inFile.size();
        m_totalBytesOutput += outFile.size();

        const QString outFileStr = QString("Output:\n%1\n").arg(fout);
        emit sendLogs(outFileStr, Qt::white, LogCode::INFO);
    } else {
        emit sendLogs(QString(" "), Qt::white, LogCode::INFO);
    }

    if (m_keepDateTime) {
        QFile outFileOpen(fout);
        outFileOpen.open(QIODevice::ReadWrite);
        outFileOpen.setFileTime(inFile.fileTime(QFileDevice::FileBirthTime), QFileDevice::FileBirthTime);
        outFileOpen.setFileTime(inFile.fileTime(QFileDevice::FileAccessTime), QFileDevice::FileAccessTime);
        outFileOpen.setFileTime(inFile.fileTime(QFileDevice::FileMetadataChangeTime), QFileDevice::FileMetadataChangeTime);
        outFileOpen.setFileTime(inFile.fileTime(QFileDevice::FileModificationTime), QFileDevice::FileModificationTime);
        outFileOpen.close();
    }

    return true;
}

void ConversionThread::calculateStats()
{
    QStringList stats;
    if (m_averageMps > 0.0) {
        const double avg = m_averageMps / static_cast<double>(m_mpsSamples);
        const QString speed = QString("\tAverage speed: %1 MP/s\n").arg(QString::number(avg));
        stats.append(speed);
        if (!m_isMultithread)
            emit sendLogs(speed, Qt::white, LogCode::INFO);
    }

    if (m_totalBytesInput > 0 && m_totalBytesOutput > 0) {
        const double delta =
            ((static_cast<double>(m_totalBytesOutput) * 1.0) / (static_cast<double>(m_totalBytesInput) * 1.0) * 100.0) - 100.0;
        double inKB = static_cast<double>(m_totalBytesInput) / 1024.0;
        double outKB = static_cast<double>(m_totalBytesOutput) / 1024.0;
        QString suffix;
        if (inKB > 10000.0) {
            suffix = QString("MiB");
            inKB = inKB / 1024.0;
            outKB = outKB / 1024.0;
        } else {
            suffix = QString("KiB");
        }
        const QString diff =
            QString("\tTotal in: %1 %3\n\tTotal out: %2 %3").arg(QString::number(inKB), QString::number(outKB), suffix);
        const QString diffDelta =
            QString("\tOut-in delta: %2%1\%\n").arg(QString::number(delta), ((delta > 0) ? QString("+") : QString("")));
        stats.append(diff);
        stats.append(diffDelta);
        if (!m_isMultithread) {
            emit sendLogs(diff, Qt::white, LogCode::INFO);
            emit sendLogs(diffDelta, statLogCol, LogCode::INFO);
        }
    }

    if (m_isMultithread && (m_averageMps > 0.0 || (m_totalBytesInput > 0 && m_totalBytesOutput > 0))) {
        emit sendLogs(stats.join('\n'), statLogCol, LogCode::INFO);
    }
}

void ConversionThread::stopProcess()
{
    mutex.lock();
    m_abort = true;
    mutex.unlock();
}

void ConversionThread::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);

    mutex.lock();
    m_ticks++;
    mutex.unlock();

    if (isFinished()) {
        killTimer(event->timerId());
    }
}
