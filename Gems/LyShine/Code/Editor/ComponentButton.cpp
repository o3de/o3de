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

ComponentButton::ComponentButton(HierarchyWidget* hierarchy,
    QWidget* parent)
    : QPushButton(parent)
{
    setText("Add Component...");

    QObject::connect(this,
        &QPushButton::clicked,
        hierarchy,
        [hierarchy]([[maybe_unused]] bool checked)
        {
            HierarchyMenu contextMenu(hierarchy,
                HierarchyMenu::Show::kAddComponents,
                true);

            contextMenu.exec(QCursor::pos());
        });

    QObject::connect(hierarchy,
        SIGNAL(SetUserSelection(HierarchyItemRawPtrList*)),
        SLOT(UserSelectionChanged(HierarchyItemRawPtrList*)));
}

void ComponentButton::UserSelectionChanged([[maybe_unused]] HierarchyItemRawPtrList* items)
{
     // no longer need to enable/disable because we always show the button, if nothing is selected it adds to the canvas entity
}

#include <moc_ComponentButton.cpp>
