/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
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
