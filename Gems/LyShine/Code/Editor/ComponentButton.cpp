/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
