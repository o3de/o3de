/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

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
        CryLogAlways(text);
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
