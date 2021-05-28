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

#include "EMStudioManager.h"
#include "EMStudioPlugin.h"
#include "MainWindow.h"
#include "RemovePluginOnCloseDockWidget.h"

namespace EMStudio
{
    RemovePluginOnCloseDockWidget::RemovePluginOnCloseDockWidget(QWidget* parent, const QString& name, EMStudio::EMStudioPlugin* plugin)
        : AzQtComponents::StyledDockWidget(name, parent)
        , mPlugin(plugin)
    {}

    void RemovePluginOnCloseDockWidget::closeEvent(QCloseEvent* event)
    {
        MCORE_UNUSED(event);
        GetPluginManager()->RemoveActivePlugin(mPlugin);
        GetMainWindow()->UpdateCreateWindowMenu();
    }
}

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_RemovePluginOnCloseDockWidget.cpp>
