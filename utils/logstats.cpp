#include "logstats.h"

#include <QGlobalStatic>
#include <QDebug>
#include <QMutex>

Q_GLOBAL_STATIC(LogStats, s_instance)

struct Q_DECL_HIDDEN LogStats::Private
{
    QMutex mutex;
    quint64 totalFilesProcessed{0};
    quint64 totalInputBytes{0};
    quint64 totalOutputBytes{0};
    double averageMpps{0};

    bool dataAdded{false};
    QStringList successfulFiles;
    QStringList failedFiles;
    QStringList failedFilesNoOut;
};

LogStats::LogStats()
    : d(new Private)
{
}

LogStats::~LogStats()
{
}

LogStats *LogStats::instance()
{
    return s_instance;
}

void LogStats::addInputBytes(quint64 v)
{
    d->mutex.lock();
    if (!d->dataAdded) {
        d->dataAdded = true;
    }
    d->totalInputBytes += v;
    d->mutex.unlock();
}

void LogStats::addOutputBytes(quint64 v)
{
    d->mutex.lock();
    if (!d->dataAdded) {
        d->dataAdded = true;
    }
    d->totalOutputBytes += v;
    d->mutex.unlock();
}

void LogStats::addMpps(double v)
{
    d->mutex.lock();
    if (!d->dataAdded) {
        d->dataAdded = true;
    }
    // const double mpps = (d->averageMpps + v) / 2.0;
    d->averageMpps += v;
    d->totalFilesProcessed++;
    d->mutex.unlock();
}

void LogStats::addFiles(const QString &f, bool success, bool fileCopied)
{
    d->mutex.lock();
    if (!d->dataAdded) {
        d->dataAdded = true;
    }
    if (success) {
        d->successfulFiles.append(f);
    } else {
        if (fileCopied) {
            d->failedFiles.append(f);
        } else {
            d->failedFilesNoOut.append(f);
        }
    }
    d->mutex.unlock();
}

quint64 LogStats::readTotalInputBytes() const
{
    d->mutex.lock();
    const quint64 v = d->totalInputBytes;
    d->mutex.unlock();
    return v;
}

quint64 LogStats::readTotalOutputBytes() const
{
    d->mutex.lock();
    const quint64 v = d->totalOutputBytes;
    d->mutex.unlock();
    return v;
}

double LogStats::readAverageMpps() const
{
    d->mutex.lock();
    const double v = d->averageMpps / static_cast<double>(d->totalFilesProcessed);
    d->mutex.unlock();
    return v;
}

QStringList& LogStats::readSuccessfulFiles() const
{
    return d->successfulFiles;
}

QStringList& LogStats::readFailedFiles(bool copiedFile) const
{
    if (copiedFile) {
        return d->failedFiles;
    } else {
        return d->failedFilesNoOut;
    }
}

void LogStats::resetValues()
{
    d->mutex.lock();
    d->dataAdded = false;
    d->totalOutputBytes = 0;
    d->totalInputBytes = 0;
    d->averageMpps = 0;
    d->totalFilesProcessed = 0;
    d->successfulFiles.clear();
    d->failedFiles.clear();
    d->failedFilesNoOut.clear();
    d->mutex.unlock();
}

bool LogStats::isDataValid() const
{
    d->mutex.lock();
    const bool valid = d->dataAdded;
    d->mutex.unlock();
    return valid;
}
