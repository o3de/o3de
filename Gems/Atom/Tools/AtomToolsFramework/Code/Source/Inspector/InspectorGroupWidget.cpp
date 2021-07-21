/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Inspector/InspectorGroupWidget.h>

namespace AtomToolsFramework
{
    InspectorGroupWidget::InspectorGroupWidget(QWidget* parent)
        : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }

    void InspectorGroupWidget::Refresh()
    {
    }

    void InspectorGroupWidget::Rebuild()
    {
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/Inspector/moc_InspectorGroupWidget.cpp>
