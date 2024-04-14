#ifndef LOGSTATS_H
#define LOGSTATS_H

#include "logcodes.h"

#include <QScopedPointer>
#include <QStringList>

class LogStats
{
public:
    LogStats();
    ~LogStats();

    LogStats(const LogStats &v) = delete;

    static LogStats *instance();

    void addInputBytes(quint64 v);
    void addOutputBytes(quint64 v);
    void addMpps(double v);
    void addFiles(const QString &f, LogCode flags);

    quint64 readTotalInputBytes() const;
    quint64 readTotalOutputBytes() const;
    double readAverageMpps() const;
    QStringList readFiles(LogCode flags) const;
    QStringList readFiles(int flags) const;
    quint64 countFiles(LogCode flags) const;
    quint64 countFiles(int flags = 0) const;

    void resetValues();
    bool isDataValid() const;

private:
    struct Private;
    const QScopedPointer<Private> d;
};

#endif // LOGSTATS_H
