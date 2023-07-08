/*
 * SPDX-FileCopyrightText: 2023 Rasyuqa A H <qampidh@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 **/

#include "mainwindow.h"
#include "conversionthread.h"
#include "ui_mainwindow.h"

#include <QCloseEvent>
#include <QDebug>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QProcess>
#include <QSettings>

class Q_DECL_HIDDEN MainWindow::Private
{
public:
    QProcess *m_execBin = nullptr;
    QString m_cjxlDir;
    QString m_djxlDir;
    QString m_cjpegliDir;
    QString m_djpegliDir;

    QString m_cjxlVerString;
    QString m_djxlVerString;
    QString m_cjpegliVerString;
    QString m_djpegliVerString;

    QStringList m_supportedCjxlFormats;
    QStringList m_supportedDjxlFormats;
    QStringList m_supportedCjpegliFormats;
    QStringList m_supportedDjpegliFormats;

    int m_majorVer = 0;
    int m_minorVer = 0;
    int m_patchVer = 0;
    int m_fullVer = 0;

    ConversionThread m_thread;
    QElapsedTimer m_eTimer;
    QSettings *m_currentSetting;

    int m_numFiles = 0;
    int m_numEncodeError = 0;
    int m_numFolderError = 0;
    int m_numSkippedWarning = 0;
    int m_numTimeoutWarning = 0;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , d(new Private)
{
    setupUi(this);

    d->m_supportedCjxlFormats << "*.png"
                        << "*.apng"
                        << "*.gif"
                        << "*.jpeg"
                        << "*.jpg"
                        << "*.jfif"
                        << "*.ppm"
                        << "*.pfm"
                        << "*.pgx"
                        << "*.jxl";

    d->m_supportedDjxlFormats << "*.jxl";

    d->m_supportedCjpegliFormats << "*.png"
                               << "*.apng"
                               << "*.gif"
                               << "*.jpeg"
                               << "*.jpg"
                               << "*.jfif"
                               << "*.ppm"
                               << "*.pfm"
                               << "*.pgx"
                               << "*.jxl";

    d->m_supportedDjpegliFormats << "*.jpg"
                                 << "*.jpeg";

    d->m_currentSetting =
        new QSettings(QDir::cleanPath(QDir::homePath() + QDir::separator() + "jxl-batch-converter-config.ini"),
                      QSettings::IniFormat);

    libjxlBinDir->setText(d->m_currentSetting->value("execBinDir").toString());
    batchChk->setChecked(d->m_currentSetting->value("batchMode", false).toBool());
    recursiveChk->setChecked(d->m_currentSetting->value("recursive", false).toBool());
    inputFileDir->setText(d->m_currentSetting->value("inDir").toString());
    overwriteChkBox->setChecked(d->m_currentSetting->value("overwrite", false).toBool());
    silenceChkBox->setChecked(d->m_currentSetting->value("silence", false).toBool());
    outputFileDir->setText(d->m_currentSetting->value("outDir").toString());
    overrideExtChk->setChecked(d->m_currentSetting->value("overrideExtChk", false).toBool());
    overrideExtLine->setText(d->m_currentSetting->value("overrideExtText", QString("jpg;png;gif")).toString());

    distanceRadio->setChecked(d->m_currentSetting->value("distChecked", true).toBool());
    distanceSpinBox->setValue(d->m_currentSetting->value("distValue", 1.0).toDouble());
    qualityRadio->setChecked(d->m_currentSetting->value("qualChecked", false).toBool());
    qualitySpinBox->setValue(d->m_currentSetting->value("qualValue", 90.0).toDouble());
    jpegTranChk->setChecked(d->m_currentSetting->value("jpegTranscoding", true).toBool());
    effortSpinBox->setValue(d->m_currentSetting->value("effort", 7).toInt());
    advOptBox->setChecked(d->m_currentSetting->value("advOptions", false).toBool());
    modularSpinBox->setValue(d->m_currentSetting->value("modular", -1).toInt());
    photonNoiseSpinBox->setValue(d->m_currentSetting->value("photonNoise", 0).toInt());
    dotsSpinBox->setValue(d->m_currentSetting->value("dots", -1).toInt());
    patchesSpinBox->setValue(d->m_currentSetting->value("patches", -1).toInt());
    epfSpinBox->setValue(d->m_currentSetting->value("epf", -1).toInt());
    gaborishSpinBox->setValue(d->m_currentSetting->value("gaborish", -1).toInt());
    fasterDecodeSpinBox->setValue(d->m_currentSetting->value("fastDecode", 0).toInt());

    custFlagsChkBox->setChecked(d->m_currentSetting->value("customFlagsChk", false).toBool());
    custFlagsText->appendPlainText(d->m_currentSetting->value("customFlagsStr").toString());
    overrideOptChkBox->setChecked(d->m_currentSetting->value("overrideFlags", false).toBool());

    const int fmtIndex = outFmtCombo->findText(d->m_currentSetting->value("outputFormat").toString());
    if (fmtIndex != -1) {
        outFmtCombo->setCurrentIndex(fmtIndex);
    }
    custOutFlagTxt->appendPlainText(d->m_currentSetting->value("customOutFlagsStr").toString());

    distanceCjpegliRadio->setChecked(d->m_currentSetting->value("distJpegliChecked").toBool());
    distanceCjpegliSpinBox->setValue(d->m_currentSetting->value("distJpegliValue").toDouble());
    qualityCjpegliRadio->setChecked(d->m_currentSetting->value("qualJpegliChecked").toBool());
    qualityCjpegliSpinBox->setValue(d->m_currentSetting->value("qualJpegliValue").toDouble());
    custCjpegliFlagsChkBox->setChecked(d->m_currentSetting->value("customJpegliFlagsChk").toBool());
    overrideCjpegliOptChkBox->setChecked(d->m_currentSetting->value("overrideJpegliFlags").toBool());
    custCjpegliFlagsText->appendPlainText(d->m_currentSetting->value("customJpegliFlagsStr").toString());

    const int fmtJpegliIndex = outJpegliFmtCombo->findText(d->m_currentSetting->value("outputJpegliFormat").toString());
    if (fmtJpegliIndex != -1) {
        outJpegliFmtCombo->setCurrentIndex(fmtJpegliIndex);
    }
    custJpegliOutFlagTxt->appendPlainText(d->m_currentSetting->value("customJpegliOutFlagsStr").toString());

    glbTimeoutSpinBox->setValue(d->m_currentSetting->value("globalTimeout").toUInt());
    stopOnErrorchkBox->setChecked(d->m_currentSetting->value("stopOnError", false).toBool());
    maxLinesSpinBox->setValue(d->m_currentSetting->value("maxLogLines", 1000).toInt());

    logText->document()->setMaximumBlockCount(maxLinesSpinBox->value());

    distanceSpinBox->setEnabled(distanceRadio->isChecked());
    qualitySpinBox->setEnabled(qualityRadio->isChecked());
    distanceCjpegliSpinBox->setEnabled(distanceCjpegliRadio->isChecked());
    qualityCjpegliSpinBox->setEnabled(qualityCjpegliRadio->isChecked());
    recursiveChk->setEnabled(batchChk->isChecked());

    aboutLabel->setMaximumWidth(selectionTabWdg->width() - 10);

    jxlVersionLabel->setText("Please select directory with libjxl binaries");
    selectionTabWdg->setTabEnabled(0, false);
    selectionTabWdg->setTabEnabled(1, false);
    selectionTabWdg->setTabEnabled(2, false);
    selectionTabWdg->setTabEnabled(3, false);

    convertBtn->setEnabled(false);
    printHelpBtn->setEnabled(false);
    progressBar->setVisible(false);

    selectionTabWdg->setDocumentMode(true);

    d->m_execBin = new QProcess(this);

    connect(libjxlBinBtn, SIGNAL(clicked(bool)), this, SLOT(libjxlBtnPressed()));
    connect(inputFileBtn, SIGNAL(clicked(bool)), this, SLOT(inputBtnPressed()));
    connect(outputFileBtn, SIGNAL(clicked(bool)), this, SLOT(outputBtnPressed()));
    connect(convertBtn, SIGNAL(clicked(bool)), this, SLOT(convertBtnPressed()));
    connect(printHelpBtn, SIGNAL(clicked(bool)), this, SLOT(printHelpBtnPressed()));

    // connect(batchChk, SIGNAL(stateChanged(int)), this, SLOT(dirChkChange()));
    connect(selectionTabWdg, SIGNAL(currentChanged(int)), this, SLOT(tabIndexChanged(int)));

    connect(abortBtn, SIGNAL(clicked(bool)), &d->m_thread, SLOT(stopProcess()));
    connect(&d->m_thread, SIGNAL(sendLogs(QString, QColor, LogCode)), this, SLOT(dumpLogs(QString, QColor, LogCode)));
    connect(&d->m_thread, SIGNAL(sendProgress(float)), this, SLOT(dumpProgress(float)));
    connect(&d->m_thread, SIGNAL(finished()), this, SLOT(resetUi()));

    adjustSize();
    cjxlChecker();
}

MainWindow::~MainWindow()
{
    delete d;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    d->m_currentSetting->setValue("execBinDir", libjxlBinDir->text());
    d->m_currentSetting->setValue("batchMode", batchChk->isChecked());
    d->m_currentSetting->setValue("recursive", recursiveChk->isChecked());
    d->m_currentSetting->setValue("inDir", inputFileDir->text());
    d->m_currentSetting->setValue("overwrite", overwriteChkBox->isChecked());
    d->m_currentSetting->setValue("silence", silenceChkBox->isChecked());
    d->m_currentSetting->setValue("outDir", outputFileDir->text());
    d->m_currentSetting->setValue("overrideExtChk", overrideExtChk->isChecked());
    d->m_currentSetting->setValue("overrideExtText", overrideExtLine->text());

    d->m_currentSetting->setValue("distChecked", distanceRadio->isChecked());
    d->m_currentSetting->setValue("distValue", distanceSpinBox->value());
    d->m_currentSetting->setValue("qualChecked", qualityRadio->isChecked());
    d->m_currentSetting->setValue("qualValue", qualitySpinBox->value());
    d->m_currentSetting->setValue("jpegTranscoding", jpegTranChk->isChecked());
    d->m_currentSetting->setValue("effort", effortSpinBox->value());
    d->m_currentSetting->setValue("advOptions", advOptBox->isChecked());
    d->m_currentSetting->setValue("modular", modularSpinBox->value());
    d->m_currentSetting->setValue("photonNoise", photonNoiseSpinBox->value());
    d->m_currentSetting->setValue("dots", dotsSpinBox->value());
    d->m_currentSetting->setValue("patches", patchesSpinBox->value());
    d->m_currentSetting->setValue("epf", epfSpinBox->value());
    d->m_currentSetting->setValue("gaborish", gaborishSpinBox->value());
    d->m_currentSetting->setValue("fastDecode", fasterDecodeSpinBox->value());

    d->m_currentSetting->setValue("customFlagsChk", custFlagsChkBox->isChecked());
    d->m_currentSetting->setValue("customFlagsStr", custFlagsText->toPlainText());
    d->m_currentSetting->setValue("overrideFlags", overrideOptChkBox->isChecked());

    d->m_currentSetting->setValue("outputFormat", outFmtCombo->currentText());
    d->m_currentSetting->setValue("customOutFlagsStr", custOutFlagTxt->toPlainText());

    d->m_currentSetting->setValue("distJpegliChecked", distanceCjpegliRadio->isChecked());
    d->m_currentSetting->setValue("distJpegliValue", distanceCjpegliSpinBox->value());
    d->m_currentSetting->setValue("qualJpegliChecked", qualityCjpegliRadio->isChecked());
    d->m_currentSetting->setValue("qualJpegliValue", qualityCjpegliSpinBox->value());
    d->m_currentSetting->setValue("customJpegliFlagsChk", custCjpegliFlagsChkBox->isChecked());
    d->m_currentSetting->setValue("overrideJpegliFlags", overrideCjpegliOptChkBox->isChecked());
    d->m_currentSetting->setValue("customJpegliFlagsStr", custCjpegliFlagsText->toPlainText());

    d->m_currentSetting->setValue("outputJpegliFormat", outJpegliFmtCombo->currentText());
    d->m_currentSetting->setValue("customJpegliOutFlagsStr", custJpegliOutFlagTxt->toPlainText());

    d->m_currentSetting->setValue("globalTimeout", glbTimeoutSpinBox->value());
    d->m_currentSetting->setValue("stopOnError", stopOnErrorchkBox->isChecked());
    d->m_currentSetting->setValue("maxLogLines", maxLinesSpinBox->value());

    event->accept();
}

void MainWindow::libjxlBtnPressed()
{
    const QString libDir =
        QFileDialog::getExistingDirectory(this, "Select directory containing libjxl binaries", libjxlBinDir->text());

    if (!libDir.isEmpty()) {
        libjxlBinDir->setText(libDir);
    }

    cjxlChecker();
}

void MainWindow::inputBtnPressed()
{
    QString inFile;

    if (batchChk->isChecked()) {
        inFile = QFileDialog::getExistingDirectory(this, "Select input directory", inputFileDir->text());
    } else {
        const QStringList spFormat = [&]() {
            switch (selectionTabWdg->currentIndex()) {
            case 0:
                return d->m_supportedCjxlFormats;
            case 1:
                return d->m_supportedDjxlFormats;
            case 2:
                return d->m_supportedCjpegliFormats;
            case 3:
                return d->m_supportedDjpegliFormats;
            default:
                break;
            }
            return QStringList();
        }();
        const QString supportedImages = spFormat.join(' ');
        inFile = QFileDialog::getOpenFileName(this,
                                              "Select input file",
                                              inputFileDir->text(),
                                              QString("Supported Images (%1)").arg(supportedImages));
    }

    if (!inFile.isEmpty()) {
        inputFileDir->setText(inFile);
    }
}

void MainWindow::outputBtnPressed()
{
    const QString outDir = QFileDialog::getExistingDirectory(this, "Select output directory", outputFileDir->text());

    QFileInfo outDirs(outDir);
    if (outDirs.isDir() && outDirs.isWritable()) {
        outputFileDir->setText(outDir);
    }
}

void MainWindow::convertBtnPressed()
{
    logText->clear();
    logText->setTextColor(Qt::white);

    progressBar->setVisible(true);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);

    convertBtn->setEnabled(false);
    binDirBox->setEnabled(false);
    fileinfoBox->setEnabled(false);
    abortBtn->setEnabled(true);
    convOptBox->setEnabled(false);
    selectionTabWdg->setEnabled(false);
    addGlbSetGrp->setEnabled(false);
    maxLinesSpinBox->setEnabled(false);

    logText->document()->setMaximumBlockCount(maxLinesSpinBox->value());

    d->m_numFiles = 0;
    d->m_numEncodeError = 0;
    d->m_numFolderError = 0;
    d->m_numSkippedWarning = 0;
    d->m_numTimeoutWarning = 0;

    d->m_eTimer.start();

    QMap<QString, QString> encOptions;

    const int selectedTabIndex = selectionTabWdg->currentIndex();

    switch (selectedTabIndex) {
    case 0:
        //
        // cjxl
        //
        if (distanceRadio->isChecked()) {
            encOptions.insert("-d", QString::number(distanceSpinBox->value()));
        } else if (qualityRadio->isChecked()) {
            encOptions.insert("-q", QString::number(qualitySpinBox->value()));
        }

        encOptions.insert("-j", (jpegTranChk->isChecked() ? "1" : "0"));
        encOptions.insert("-e", QString::number(effortSpinBox->value()));

        if (advOptBox->isChecked()) {
            if (modularSpinBox->value() != -1) {
                encOptions.insert("-m", QString::number(modularSpinBox->value()));
            }
            if (dotsSpinBox->value() != -1) {
                encOptions.insert("--dots", QString::number(dotsSpinBox->value()));
            }
            if (gaborishSpinBox->value() != -1) {
                encOptions.insert("--gaborish", QString::number(gaborishSpinBox->value()));
            }
            if (patchesSpinBox->value() != -1) {
                encOptions.insert("--patches", QString::number(patchesSpinBox->value()));
            }
            if (d->m_fullVer >= 9000) {
                encOptions.insert("--photon_noise_iso", QString::number(photonNoiseSpinBox->value()));
            } else if (photonNoiseSpinBox->value() > 0) {
                encOptions.insert("--photon_noise", QString("ISO%1").arg(QString::number(photonNoiseSpinBox->value())));
            }

            encOptions.insert("--epf", QString::number(epfSpinBox->value()));
            encOptions.insert("--faster_decoding", QString::number(fasterDecodeSpinBox->value()));
        }

        if (custFlagsChkBox->isChecked()) {
            if (overrideOptChkBox->isChecked()) {
                encOptions.clear();
            }
            encOptions.insert("customFlags", custFlagsText->toPlainText());
        }
        encOptions.insert("outFormat", ".jxl");
        break;
    case 1:
        //
        // djxl
        //
        encOptions.insert("outFormat", outFmtCombo->currentText());
        encOptions.insert("customFlags", custOutFlagTxt->toPlainText());
        break;
    case 2:
        //
        // cjpegli
        //
        if (distanceCjpegliRadio->isChecked()) {
            encOptions.insert("-d", QString::number(distanceCjpegliSpinBox->value()));
        } else if (qualityCjpegliRadio->isChecked()) {
            encOptions.insert("-q", QString::number(qualityCjpegliSpinBox->value()));
        }

        if (custCjpegliFlagsChkBox->isChecked()) {
            if (overrideCjpegliOptChkBox->isChecked()) {
                encOptions.clear();
            }
            encOptions.insert("customFlags", custCjpegliFlagsText->toPlainText());
        }

        encOptions.insert("outFormat", ".jpg");
        break;
    case 3:
        //
        // djpegli
        //
        encOptions.insert("outFormat", outJpegliFmtCombo->currentText());
        encOptions.insert("customFlags", custJpegliOutFlagTxt->toPlainText());
        break;
    default:
        //
        // cosmic ray is acting up again..
        //
        dumpLogs(QString("Warning: no flags are passed, possibly an internal error."), warnLogCol, LogCode::INFO);
        break;
    }

    encOptions.insert("overwrite", (overwriteChkBox->isChecked() ? "1" : "0"));
    encOptions.insert("silent", (silenceChkBox->isChecked() ? "1" : "0"));
    encOptions.insert("globalTimeout", QString::number(glbTimeoutSpinBox->value()));
    encOptions.insert("globalStopOnError", (stopOnErrorchkBox->isChecked() ? "1" : "0"));

    QDir outUrl(outputFileDir->text());
    if (!outUrl.exists()) {
        if (!outUrl.mkpath(".")) {
            dumpLogs(QString("Error: cannot create output directory!"), errLogCol, LogCode::INFO);
            resetUi();
            return;
        }
    }

    QFileInfo outTestFile(outputFileDir->text());
    if (!outTestFile.isWritable()) {
        dumpLogs(QString("Output error: permission denied!"), errLogCol, LogCode::INFO);
        resetUi();
        progressBar->setVisible(false);
        return;
    }

    QFileInfo inFUrl(inputFileDir->text());
    if (!inFUrl.exists()) {
        dumpLogs(QString("Error: input file/dir doesn't exist!"), errLogCol, LogCode::INFO);
        resetUi();
        progressBar->setVisible(false);
        return;
    }

    encOptions.insert("directoryInput", inputFileDir->text());

    const QString binPath = [&]() {
        switch (selectionTabWdg->currentIndex()) {
        case 0:
            return d->m_cjxlDir;
        case 1:
            return d->m_djxlDir;
        case 2:
            return d->m_cjpegliDir;
        case 3:
            return d->m_djpegliDir;
        default:
            break;
        }
        return QString();
    }();

    const QStringList spFormats = [&]() {
        if (overrideExtChk->isChecked()) {
            QStringList overrideFormats = overrideExtLine->text().split(';');
            for (auto &fmt : overrideFormats) {
                fmt = QString("*.") + fmt;
            }
            return overrideFormats;
        }
        switch (selectionTabWdg->currentIndex()) {
        case 0:
            return d->m_supportedCjxlFormats;
        case 1:
            return d->m_supportedDjxlFormats;
        case 2:
            return d->m_supportedCjpegliFormats;
        case 3:
            return d->m_supportedDjpegliFormats;
        default:
            break;
        }
        return QStringList();
    }();

    if (spFormats.isEmpty() || binPath.isEmpty()) {
        dumpLogs(QString("Error: please select the correct tab"), errLogCol, LogCode::INFO);
        resetUi();
        progressBar->setVisible(false);
        return;
    }

    if (!batchChk->isChecked()) {
        // inFUrl will get ignored later tho...
        const QString inFUrl = inputFileDir->text();
        const QString outFUrl = outputFileDir->text();

        d->m_thread.processFiles(binPath, inFUrl, outFUrl, encOptions);
        d->m_thread.start();
    } else {
        const bool isRecursive = recursiveChk->isChecked();

        if (overrideExtChk->isChecked()) {
            if (spFormats.isEmpty()) {
                dumpLogs(QString("Error: Extension list is empty"), errLogCol, LogCode::INFO);
            }
            const QString logOverrideExt = QString("Overriding batch extensions: %1\n").arg(spFormats.join(' '));
            dumpLogs(logOverrideExt, warnLogCol, LogCode::INFO);
        }

        const QString inFUrl = inputFileDir->text();
        QFileInfo inFile(inFUrl);

        QDir inUrl;
        if (inFile.isFile()) {
            inUrl.setPath(inFile.absolutePath());
        } else {
            inUrl.setPath(inFile.absoluteFilePath());
        }

        QDirIterator dit(inUrl.absolutePath(),
                         spFormats,
                         QDir::Files,
                         isRecursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);

        if (!dit.hasNext()) {
            dumpLogs(QString("Error: directory contains no file(s) to convert!"), errLogCol, LogCode::INFO);
            resetUi();
            progressBar->setVisible(false);
            return;
        }

        const int numFiles = d->m_thread.processFiles(binPath, dit, outputFileDir->text(), encOptions);
        if (numFiles > 0) {
            progressBar->setMaximum(numFiles);
        }
        d->m_thread.start();
    }
}

void MainWindow::printHelpBtnPressed()
{
    logText->clear();

    const int selectedTabIndex = selectionTabWdg->currentIndex();

    QStringList helper;

    switch (selectedTabIndex) {
    case 0:
        helper << "-h" << "-v" << "-v" << "-v";
        d->m_execBin->start(d->m_cjxlDir, helper);
        break;
    case 1:
        helper << "-h";
        d->m_execBin->start(d->m_djxlDir, helper);
        break;
    case 2:
        helper << "-h" << "-v" << "-v" << "-v";
        d->m_execBin->start(d->m_cjpegliDir, helper);
        break;
    case 3:
        helper << "-h";
        d->m_execBin->start(d->m_djpegliDir, helper);
        break;
    default:
        dumpLogs(QString("Error: please select the correct tab"), errLogCol, LogCode::INFO);
        return;
    }

    if (!d->m_execBin->waitForFinished()) {
        printHelpBtn->setEnabled(false);
        convertBtn->setEnabled(false);
        return;
    }

    dumpLogs(d->m_execBin->readAllStandardError(), okayLogCol, LogCode::INFO);
    dumpLogs(d->m_execBin->readAllStandardOutput(), Qt::white, LogCode::INFO);
}

void MainWindow::tabIndexChanged(const int &index)
{
    switch (index) {
    case 0:
        if (!d->m_cjxlVerString.isEmpty()) {
            jxlVersionLabel->setText(d->m_cjxlVerString);
        }
        break;
    case 1:
        if (!d->m_djxlVerString.isEmpty()) {
            jxlVersionLabel->setText(d->m_djxlVerString);
        }
        break;
    case 2:
        if (!d->m_cjpegliVerString.isEmpty()) {
            jxlVersionLabel->setText(d->m_cjpegliVerString);
        }
        break;
    case 3:
        if (!d->m_djpegliVerString.isEmpty()) {
            jxlVersionLabel->setText(d->m_djpegliVerString);
        }
        break;
    default:
        break;
    }

    if (index == selectionTabWdg->count() - 1) {
        jxlVersionLabel->setText("About");
        convertBtn->setEnabled(false);
        printHelpBtn->setEnabled(false);
    } else {
        convertBtn->setEnabled(true);
        printHelpBtn->setEnabled(true);
    }
}

void MainWindow::resetUi()
{
    const float decodeTime = d->m_eTimer.elapsed() / 1000.0;

    if (d->m_numFiles > 0) {
        const QString separator("=----------------=");

        logText->setTextColor(Qt::darkGray);
        logText->append(separator);

        logText->setTextColor(Qt::white);

        bool haveError = false;
        logText->append(QString("Conversion done for %1 image(s)").arg(QString::number(d->m_numFiles)));

        if (d->m_numEncodeError > 0) {
            haveError = true;
            logText->setTextColor(errLogCol);
            logText->append(QString("\t%1 libjxl processing error(s)").arg(QString::number(d->m_numEncodeError)));
        }

        if (d->m_numFolderError > 0) {
            haveError = true;
            logText->setTextColor(errLogCol);
            logText->append(QString("\t%1 output folder creation error(s)").arg(QString::number(d->m_numFolderError)));
        }

        if (d->m_numSkippedWarning > 0) {
            logText->setTextColor(warnLogCol);
            logText->append(QString("\t%1 skipped existing file(s)").arg(QString::number(d->m_numSkippedWarning)));
        }

        if (d->m_numTimeoutWarning > 0) {
            logText->setTextColor(warnLogCol);
            logText->append(QString("\t%1 process timeout(s)").arg(QString::number(d->m_numTimeoutWarning)));
        }

        logText->setTextColor(Qt::white);

        if (!haveError) {
            logText->append("All image(s) successfully converted");
        } else {
            logText->append("Some image(s) have errors during conversion");
        }

        logText->append(QString("\nElapsed time: %1 second(s)").arg(QString::number(decodeTime)));
        logText->setTextColor(Qt::darkGray);
        logText->append(separator);

        logText->setTextColor(Qt::white);
    }

    selectionTabWdg->setEnabled(true);
    convertBtn->setEnabled(true);
    binDirBox->setEnabled(true);
    fileinfoBox->setEnabled(true);
    abortBtn->setEnabled(false);
    convOptBox->setEnabled(true);
    addGlbSetGrp->setEnabled(true);
    maxLinesSpinBox->setEnabled(true);
}

void MainWindow::dirChkChange()
{
    // reserved
}

void MainWindow::dumpLogs(const QString &logs, const QColor &col, const LogCode &isErr)
{
    switch (isErr) {
    case LogCode::FILE_IN:
        d->m_numFiles++;
        break;
    case LogCode::ENCODE_ERR:
        d->m_numEncodeError++;
        break;
    case LogCode::OUT_FOLDER_ERR:
        d->m_numFolderError++;
        break;
    case LogCode::SKIPPED:
        d->m_numSkippedWarning++;
        break;
    case LogCode::SKIPPED_TIMEOUT:
        d->m_numTimeoutWarning++;
        break;
    default:
        break;
    }

    if (!logs.isEmpty()) {
        logText->setTextColor(col);
        logText->append(logs);
    }
}

void MainWindow::dumpProgress(const float &prog)
{
    progressBar->setVisible(true);
    progressBar->setValue(prog);
}

void MainWindow::cjxlChecker()
{
    selectionTabWdg->setTabEnabled(0, false);
    selectionTabWdg->setTabEnabled(1, false);
    selectionTabWdg->setTabEnabled(2, false);
    selectionTabWdg->setTabEnabled(3, false);
    printHelpBtn->setEnabled(false);
    convertBtn->setEnabled(false);

    const QString libDir = libjxlBinDir->text();

    logText->clear();


#ifdef Q_OS_WIN
    const QString binSuffix = QString(".exe");
#else
    const QString binSuffix = QString();
#endif

    QList<int> activeNdx{};

    QDir rootJxl(libDir);
    if (libDir.isEmpty() || (!rootJxl.exists(QString("cjxl")) && !rootJxl.exists(QString("cjxl.exe")))) {
        jxlVersionLabel->setText("Error: cjxl is not found in selected directory!");
        d->m_cjxlDir = QString();
    } else {
      const QString cjxlDir = QDir::cleanPath(libDir + QDir::separator() +
                                              QString("cjxl") + binSuffix);

      QFileInfo cjxlFile(cjxlDir);
      if (!cjxlFile.isExecutable()) {
        jxlVersionLabel->setText("Error: cjxl is found but not executable!");
        logText->append("Error: cjxl is found but not executable!");
        d->m_cjxlDir = QString();
      } else {
        d->m_execBin->start(cjxlDir, QStringList());
        if (!d->m_execBin->waitForFinished()) {
          printHelpBtn->setEnabled(false);
          convertBtn->setEnabled(false);
          return;
        }

        QString cjxlInfo;
        cjxlInfo = d->m_execBin->readAllStandardError();

        d->m_cjxlDir = cjxlDir;

        int vPos = cjxlInfo.indexOf('v');
        QString versionTrimLeft = cjxlInfo.mid(
            vPos); // silencing clazy because 5.15 don't have midRef...
        int spPos = versionTrimLeft.indexOf(' ');
        QString versionTrim = cjxlInfo.mid(vPos + 1, spPos - 1);

        QStringList versionList = versionTrim.trimmed().split('.');
        if (versionList.size() == 3) {
          d->m_majorVer = versionList.at(0).toInt();
          d->m_minorVer = versionList.at(1).toInt();
          d->m_patchVer = versionList.at(2).toInt();
          d->m_fullVer = (d->m_majorVer * 1000000) + (d->m_minorVer * 1000) +
                         (d->m_patchVer);
        } else {
          // crude binary checking
          logText->append("Error: cannot determine cjxl version");
          selectionTabWdg->setTabEnabled(0, false);
          printHelpBtn->setEnabled(false);
          convertBtn->setEnabled(false);
          d->m_cjxlDir = QString();
          return;
        }

        jxlVersionLabel->setText(
            cjxlInfo.left(cjxlInfo.indexOf('\n')).trimmed());
        d->m_cjxlVerString = cjxlInfo.left(cjxlInfo.indexOf('\n')).trimmed();

        selectionTabWdg->setTabEnabled(0, true);
        activeNdx.append(0);
      }
    }

    // djxl checker

    if (!rootJxl.exists(QString("djxl")) && !rootJxl.exists(QString("djxl.exe"))) {
        logText->append("djxl is not found in selected directory");
        selectionTabWdg->setTabEnabled(1, false);
        d->m_djxlVerString = QString();
        d->m_djxlDir = QString();
    } else {
        const QString djxlDir = QDir::cleanPath(libDir + QDir::separator() + QString("djxl") + binSuffix);
        d->m_djxlDir = djxlDir;

        QFileInfo djxlFile(djxlDir);
        if (!djxlFile.isExecutable()) {
            logText->append("Error: djxl is found but not executable!");
            d->m_djxlDir = QString();
            selectionTabWdg->setTabEnabled(1, false);
        } else {
            d->m_execBin->start(djxlDir, QStringList());
            d->m_execBin->waitForFinished();

            QString djxlInfo;
            djxlInfo = d->m_execBin->readAllStandardError();

            if (!djxlInfo.isEmpty()) {
                d->m_djxlVerString = djxlInfo.left(djxlInfo.indexOf('\n')).trimmed();
            }

            selectionTabWdg->setTabEnabled(1, true);
            activeNdx.append(1);
        }
    }

    // cjpegli checker

    if (!rootJxl.exists(QString("cjpegli")) && !rootJxl.exists(QString("cjpegli.exe"))) {
        logText->append("cjpegli is not found in selected directory");
        selectionTabWdg->setTabEnabled(2, false);
        d->m_cjpegliVerString = QString();
        d->m_cjpegliDir = QString();
    } else {
        const QString cjpegliDir = QDir::cleanPath(libDir + QDir::separator() + QString("cjpegli") + binSuffix);
        d->m_cjpegliDir = cjpegliDir;

        QFileInfo cjpegliFile(cjpegliDir);
        if (!cjpegliFile.isExecutable()) {
            logText->append("Error: cjpegli is found but not executable!");
            d->m_cjpegliDir = QString();
            selectionTabWdg->setTabEnabled(2, false);
        } else {
            d->m_execBin->start(cjpegliDir, QStringList());
            d->m_execBin->waitForFinished();

            QString cjpegliInfo;
            cjpegliInfo = d->m_execBin->readAllStandardError();

            if (!cjpegliInfo.isEmpty()) {
                d->m_cjpegliVerString = cjpegliInfo.left(cjpegliInfo.indexOf('\n')).trimmed();
            } else {
                d->m_cjpegliVerString = QString("cjpegli found");
            }

            selectionTabWdg->setTabEnabled(2, true);
            activeNdx.append(2);
        }
    }

    // djpegli checker

    if (!rootJxl.exists(QString("djpegli")) && !rootJxl.exists(QString("djpegli.exe"))) {
        logText->append("djpegli is not found in selected directory");
        selectionTabWdg->setTabEnabled(3, false);
        d->m_djpegliVerString = QString();
        d->m_djpegliDir = QString();
    } else {
        const QString djpegliDir = QDir::cleanPath(libDir + QDir::separator() + QString("djpegli") + binSuffix);
        d->m_djpegliDir = djpegliDir;

        QFileInfo djpegliFile(djpegliDir);
        if (!djpegliFile.isExecutable()) {
            logText->append("Error: djpegli is found but not executable!");
            d->m_djpegliDir = QString();
            selectionTabWdg->setTabEnabled(3, false);
        } else {
            d->m_execBin->start(djpegliDir, QStringList());
            d->m_execBin->waitForFinished();

            QString djpegliInfo;
            djpegliInfo = d->m_execBin->readAllStandardError();

            if (!djpegliInfo.isEmpty()) {
                d->m_djpegliVerString = djpegliInfo.left(djpegliInfo.indexOf('\n')).trimmed();
            } else {
                d->m_djpegliVerString = QString("djpegli found");
            }

            selectionTabWdg->setTabEnabled(3, true);
            activeNdx.append(3);
        }
    }

    if (!activeNdx.isEmpty()) {
        const int leftmostTab = activeNdx.first();
        selectionTabWdg->setCurrentIndex(leftmostTab);
        printHelpBtn->setEnabled(true);
        convertBtn->setEnabled(true);
    } else {
        selectionTabWdg->setCurrentIndex(selectionTabWdg->count() - 1);
    }
}
