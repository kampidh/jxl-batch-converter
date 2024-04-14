#ifndef LOGCODES_H
#define LOGCODES_H

#include <QColor>
#include <QMetaType>

enum LogCode {
    INFO = 1 << 0,
    FILE_IN = 1 << 1,
    OK = 1 << 2,
    SKIPPED = 1 << 3,
    SKIPPED_ALREADY_EXIST = 1 << 4,
    SKIPPED_TIMEOUT = 1 << 5,
    OUT_FOLDER_ERR = 1 << 6,
    ENCODE_ERR_SKIP = 1 << 7,
    ENCODE_ERR_COPY = 1 << 8,
    ABORTED = 1 << 9
};

Q_DECLARE_METATYPE(LogCode);

static const QColor warnLogCol(255, 255, 100);
static const QColor errLogCol(255, 150, 150);
static const QColor statLogCol(255, 187, 255);
static const QColor okayLogCol(50, 255, 150);

#endif // LOGCODES_H
