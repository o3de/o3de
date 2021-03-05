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
                     HierarchyMenu::Show::kNew_ElementFromPrefabsAtRoot |
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
