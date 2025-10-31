/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <QAction>
#include <QObject>
#include <QMenu>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#endif

#include <Source/Translation/TranslationBus.h>

namespace ScriptCanvasDeveloperEditor
{
    void ReloadText(QWidget*)
    {
        GraphCanvas::TranslationRequestBus::Broadcast(&GraphCanvas::TranslationRequests::Restore);
    }

    QAction* TranslationDatabaseFileAction(QMenu* mainMenu, QWidget* mainWindow)
    {
        QAction* qAction = nullptr;

        if (mainWindow)
        {
            qAction = mainMenu->addAction(QAction::tr("Reload Text"));
            qAction->setAutoRepeat(false);
            qAction->setToolTip("Reloads all the text data used by Script Canvas for titles, tooltips, etc.");
            qAction->setShortcut(QAction::tr("Ctrl+Alt+R", "Developer|Reload Text"));
            QObject::connect(qAction, &QAction::triggered, [mainWindow]() { ReloadText(mainWindow); });

            qAction = mainMenu->addAction(QAction::tr("Dump Translation Database"));
            qAction->setAutoRepeat(false);
            qAction->setShortcut(QAction::tr("Ctrl+Alt+L", "Developer|Dump Translation Database"));
            QObject::connect(
                qAction, &QAction::triggered,
                [mainWindow]()
                {
                    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
                    QString directory = QFileDialog::getExistingDirectory(mainWindow,
                        QObject::tr("Select output folder for sc_translation.log file"), defaultPath);
                    if (!directory.isEmpty())
                    {
                        const QString path = QDir::toNativeSeparators(directory + "/sc_translation.log");
                        GraphCanvas::TranslationRequestBus::Broadcast(&GraphCanvas::TranslationRequests::DumpDatabase, path.toUtf8().constData());
                        QMessageBox::information(
                            mainWindow, QObject::tr("Finished writing translation database"),
                            QObject::tr("Translation database written to:<br/><a href=\"file:///%1\">%1</a>").arg(path));
                    }
                });

        }

        return qAction;
    }

} // ScriptCanvasDeveloperEditor
