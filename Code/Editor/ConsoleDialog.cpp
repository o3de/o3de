/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "ConsoleDialog.h"

// Qt
#include <QVBoxLayout>

// Editor
#include "Controls/ConsoleSCB.h"    // For CConsoleSCB
#include "LyViewPaneNames.h"        // for LyViewPane::


CConsoleDialog::CConsoleDialog(QWidget* parent)
    : QDialog(parent)
    , m_consoleWidget(new CConsoleSCB(this))
{
    QVBoxLayout* outterLayout = new QVBoxLayout(this);
    outterLayout->addWidget(m_consoleWidget);
    outterLayout->setMargin(0);

    setWindowTitle(LyViewPane::Console);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(842, 480);
}

void CConsoleDialog::SetInfoText(const char* text)
{
    if (gEnv && gEnv->pLog)                             // before log system was initialized
    {
        CryLogAlways("%s", text);
    }
}

void CConsoleDialog::closeEvent(QCloseEvent* ev)
{
    if (GetISystem())
    {
        GetISystem()->Quit();
    }
    QDialog::closeEvent(ev);
}

#include <moc_ConsoleDialog.cpp>
