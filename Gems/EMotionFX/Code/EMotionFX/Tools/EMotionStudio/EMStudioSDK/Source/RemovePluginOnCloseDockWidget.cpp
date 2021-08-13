/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EMStudioManager.h"
#include "EMStudioPlugin.h"
#include "MainWindow.h"
#include "RemovePluginOnCloseDockWidget.h"

namespace EMStudio
{
    RemovePluginOnCloseDockWidget::RemovePluginOnCloseDockWidget(QWidget* parent, const QString& name, EMStudio::EMStudioPlugin* plugin)
        : AzQtComponents::StyledDockWidget(name, parent)
        , m_plugin(plugin)
    {}

    void RemovePluginOnCloseDockWidget::closeEvent(QCloseEvent* event)
    {
        MCORE_UNUSED(event);
        GetPluginManager()->RemoveActivePlugin(m_plugin);
        GetMainWindow()->UpdateCreateWindowMenu();
    }
}

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_RemovePluginOnCloseDockWidget.cpp>
