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
#include "UiCanvasEditor_precompiled.h"

#include "EditorCommon.h"
#include "AlignToolbarSection.h"
#include "ViewportAlign.h"

#include <QToolButton>
#include <QObject>

AlignToolbarSection::AlignToolbarSection()
{
}

void AlignToolbarSection::SetIsEnabled(bool enabled)
{
    for (auto button : m_buttons)
    {
        button->setEnabled(enabled);
    }
}

void AlignToolbarSection::SetIsVisible(bool visible)
{
    m_separator->setVisible(visible);

    for (auto button : m_buttons)
    {
        button->setVisible(visible);
    }
}

void AlignToolbarSection::AddButtons(QToolBar* parent)
{
    m_separator = parent->addSeparator();
    AddButton(parent, ViewportAlign::AlignType::VerticalTop,        "AlignVTop",    "Align Top Edges");
    AddButton(parent, ViewportAlign::AlignType::VerticalCenter,     "AlignVCenter", "Align Centers Vertically");
    AddButton(parent, ViewportAlign::AlignType::VerticalBottom,     "AlignVBottom", "Align Bottom Edges");
    AddButton(parent, ViewportAlign::AlignType::HorizontalLeft,     "AlignHLeft",   "Align Left Edges");
    AddButton(parent, ViewportAlign::AlignType::HorizontalCenter,   "AlignHCenter", "Align Centers Horizontally");
    AddButton(parent, ViewportAlign::AlignType::HorizontalRight,    "AlignHRight",  "Align Right Edges");
}

void AlignToolbarSection::AddButton(QToolBar* parent, ViewportAlign::AlignType alignType, const QString& iconName, const QString& toolTip)
{
    EditorWindow* editorWindow = static_cast<EditorWindow*>(parent->parent());

    // Setup the icon
    QString iconImageDefault = QString(":/Icons/%1Default.png").arg(iconName);

    QIcon icon(iconImageDefault);

    // Create a ToolButton to put in the parent toolbar
    QToolButton* button = new QToolButton(parent);
    button->setIcon(icon);
    button->setToolTip(toolTip);
    button->setContentsMargins(0, 0, 0, 0);

    // Connect it up to call the align operation
    QObject::connect(button,
        &QPushButton::clicked,
        [ editorWindow, alignType ]([[maybe_unused]] bool checked)
        {
            ViewportAlign::AlignSelectedElements(editorWindow, alignType);
        });

    // Add the button to the parent tool bar
    QAction* buttonAction = parent->addWidget(button);

    //! we store the QActions rather than the QToolButtons since hiding and showing will not work on the QToolButtons
    m_buttons.push_back(buttonAction);
}

