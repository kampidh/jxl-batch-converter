#include "folderselectiondialog.h"
#include "ui_folderselectiondialog.h"

#include <QDebug>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileDialog>
#include <QMessageBox>

FolderSelectionDialog::FolderSelectionDialog(const QString &inputDir, const QStringList &fl, QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    m_fl = fl;
    m_inputDir = inputDir;

    folderListView->clear();

    if (!m_fl.isEmpty()) {
        foreach (const auto &f, m_fl) {
            if (f.contains(inputDir) && f != inputDir) {
                folderListView->addItem(f);
            }
        }
    }

    connect(clearBtn, SIGNAL(clicked(bool)), folderListView, SLOT(clear()));

    connect(removeBtn, &QPushButton::clicked, this, [&]() {
        if (!folderListView->selectedItems().isEmpty()) {
            foreach (const auto &item, folderListView->selectedItems()) {
                delete folderListView->takeItem(folderListView->row(item));
            }
        }
    });

    connect(addBtn, &QPushButton::clicked, this, [&]() {
        const QString sl = QFileDialog::getExistingDirectory(this, "Add directory", inputDir);
        if (!sl.isEmpty()) {
            if (!(sl.contains(inputDir) && sl != inputDir)) {
                QMessageBox::information(this,
                                         "Folder selection",
                                         "Unable to add directory.\n"
                                         "Please only add directory that's inside the input directory");
            } else {
                // hack: in case user inputs folder that's outside the input but having the same (base) name
                // eg: input, input12, input-xyz
                if (!(QString(sl).remove(m_inputDir).startsWith("/")
                      || QString(sl).remove(m_inputDir).startsWith("\\"))) {
                    return;
                }

                if (folderListView->findItems(sl, Qt::MatchExactly).isEmpty()) {
                    folderListView->addItem(sl);
                }
            }
        }
    });
}

FolderSelectionDialog::~FolderSelectionDialog()
{
}

void FolderSelectionDialog::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->accept();
    }
}

void FolderSelectionDialog::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->accept();
    }
}

void FolderSelectionDialog::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        foreach (const QUrl &url, event->mimeData()->urls()) {
            const QFileInfo fileInfo = url.toLocalFile();

            if (fileInfo.isDir() && fileInfo.absoluteFilePath().contains(m_inputDir)
                && fileInfo.absoluteFilePath() != m_inputDir) {

                // hack: in case user inputs folder that's outside the input but having the same (base) name
                // eg: input, input12, input-xyz
                if (!(fileInfo.absoluteFilePath().remove(m_inputDir).startsWith("/")
                      || fileInfo.absoluteFilePath().remove(m_inputDir).startsWith("\\"))) {
                    continue;
                }

                if (folderListView->findItems(fileInfo.absoluteFilePath(), Qt::MatchExactly).isEmpty()) {
                    folderListView->addItem(fileInfo.absoluteFilePath());
                }
            }
        }
    }
}

QStringList FolderSelectionDialog::readLists()
{
    QStringList ls;
    for (int i = 0; i < folderListView->count(); i++) {
        ls.append(folderListView->item(i)->text());
    }
    return ls;
}
