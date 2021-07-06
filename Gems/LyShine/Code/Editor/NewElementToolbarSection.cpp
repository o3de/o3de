/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasEditor_precompiled.h"

#include "EditorCommon.h"

NewElementToolbarSection::NewElementToolbarSection(QToolBar* parent, bool addSeparator)
{
    EditorWindow* editorWindow = static_cast<EditorWindow*>(parent->parent());

    {
        QPushButton* button = new QPushButton("New...", parent);
        QObject::connect(button,
            &QPushButton::clicked,
            editorWindow,
            [editorWindow]([[maybe_unused]] bool checked)
            {
                HierarchyMenu contextMenu(editorWindow->GetHierarchy(),
                    (HierarchyMenu::Show::kNew_EmptyElementAtRoot |
                     HierarchyMenu::Show::kNew_InstantiateSliceAtRoot),
                    false);

                contextMenu.exec(QCursor::pos());
            });

        parent->addWidget(button);
    }

    if (addSeparator)
    {
        parent->addSeparator();
    }
}
