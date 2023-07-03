#ifndef LOGCODES_H
#define LOGCODES_H

#include <QColor>
#include <QMetaType>

enum LogCode {
    INFO = 0,
    FILE_IN,
    OK,
    SKIPPED,
    SKIPPED_TIMEOUT,
    OUT_FOLDER_ERR,
    ENCODE_ERR
};

Q_DECLARE_METATYPE(LogCode);

static const QColor warnLogCol(255, 255, 100);
static const QColor errLogCol(255, 150, 150);
static const QColor statLogCol(255, 187, 255);
static const QColor okayLogCol(50, 255, 150);

#endif // LOGCODES_H
