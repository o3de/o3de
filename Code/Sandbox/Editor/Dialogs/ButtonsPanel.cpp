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

#include "ButtonsPanel.h"

// Qt
#include <QGridLayout>

// Editor
#include "Controls/ToolButton.h"

/////////////////////////////////////////////////////////////////////////////
// CButtonsPanel dialog
CButtonsPanel::CButtonsPanel(QWidget* parent)
    : QWidget(parent)
{
}

CButtonsPanel::~CButtonsPanel()
{
}

//////////////////////////////////////////////////////////////////////////
void CButtonsPanel::AddButton(const SButtonInfo& button)
{
    SButton b;
    b.info = button;
    m_buttons.push_back(b);
}
//////////////////////////////////////////////////////////////////////////
void CButtonsPanel::AddButton(const QString& name, const QString& toolClass)
{
    SButtonInfo bi;
    bi.name = name;
    bi.toolClassName = toolClass;
    AddButton(bi);
}
//////////////////////////////////////////////////////////////////////////
void CButtonsPanel::AddButton(const QString& name, const QMetaObject* pToolClass)
{
    SButtonInfo bi;
    bi.name = name;
    bi.pToolClass = pToolClass;
    AddButton(bi);
}
//////////////////////////////////////////////////////////////////////////
void CButtonsPanel::ClearButtons()
{
    auto buttons = layout()->findChildren<QEditorToolButton*>();
    foreach(auto button, buttons)
    {
        layout()->removeWidget(button);
        delete button;
    }
    m_buttons.clear();
}

void CButtonsPanel::UncheckAll()
{
    for (auto& button : m_buttons)
    {
        button.pButton->SetSelected(false);
    }
}

void CButtonsPanel::OnInitDialog()
{
    auto layout = new QGridLayout(this);
    setLayout(layout);

    layout->setMargin(4);
    layout->setHorizontalSpacing(4);
    layout->setVerticalSpacing(1);

    // Create Buttons.
    int index = 0;
    for (auto& button : m_buttons)
    {
        button.pButton = new QEditorToolButton(this);
        button.pButton->setObjectName(button.info.name);
        button.pButton->setText(button.info.name);
        button.pButton->SetNeedDocument(button.info.bNeedDocument);
        button.pButton->setToolTip(button.info.toolTip);

        if (button.info.pToolClass)
        {
            button.pButton->SetToolClass(button.info.pToolClass, button.info.toolUserDataKey, (void*)button.info.toolUserData.c_str());
        }
        else if (!button.info.toolClassName.isEmpty())
        {
            button.pButton->SetToolName(button.info.toolClassName, button.info.toolUserDataKey, (void*)button.info.toolUserData.c_str());
        }

        layout->addWidget(button.pButton, index / 2, index % 2);
        connect(button.pButton, &QEditorToolButton::clicked, this, [&]() { OnButtonPressed(button.info); });
        ++index;
    }
}

void CButtonsPanel::EnableButton(const QString& buttonName, bool enable)
{
    for (auto& button : m_buttons)
    {
        if (button.pButton->objectName() == buttonName)
        {
            button.pButton->setEnabled(enable);
        }
    }
}

#include <Dialogs/moc_ButtonsPanel.cpp>
