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

        }

        return qAction;
    }

} // ScriptCanvasDeveloperEditor
