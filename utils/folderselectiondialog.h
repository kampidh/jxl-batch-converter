#ifndef FOLDERSELECTIONDIALOG_H
#define FOLDERSELECTIONDIALOG_H

#include "ui_folderselectiondialog.h"
#include <QDialog>

class FolderSelectionDialog : public QDialog, public Ui::FolderSelectionDialog
{
    Q_OBJECT

public:
    explicit FolderSelectionDialog(const QString &inputDir, const QStringList &fl, QWidget *parent = nullptr);
    ~FolderSelectionDialog();

    QStringList readLists();

private:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    QStringList m_fl;
    QString m_inputDir;
};

#endif // FOLDERSELECTIONDIALOG_H
