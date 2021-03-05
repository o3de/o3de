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

EnterPreviewToolbar::EnterPreviewToolbar(EditorWindow* parent)
    : QToolBar("Enter Preview Toolbar", parent)
{
    setObjectName("EnterPreviewToolbar");    // needed to save state
    setFloatable(false);

    // Add a spacer widget to force the Preview button to be on the right of the toolbar area
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    addWidget(spacer);

    // Add the Preview button to the right of the toolbar
    m_previewButton = new QPushButton("Preview", parent);
    QObject::connect(m_previewButton,
        &QPushButton::clicked,
        parent,
        [parent]([[maybe_unused]] bool checked)
        {
            parent->ToggleEditorMode();
        });

    m_previewButton->setToolTip(QString("Switch to Preview mode"));

    // this uses the "primary button" style from the global style sheet
    m_previewButton->setProperty("class", "Primary");

    m_previewButton->setIcon(QIcon(":/Icons/PreviewStart.png"));

    addWidget(m_previewButton);

    parent->addToolBar(this);
}

#include <moc_EnterPreviewToolbar.cpp>
