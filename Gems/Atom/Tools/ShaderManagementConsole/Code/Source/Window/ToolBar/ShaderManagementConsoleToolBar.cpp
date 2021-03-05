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

#include <AzCore/PlatformDef.h>
#include <Source/Window/ToolBar/ShaderManagementConsoleToolBar.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <AzQtComponents/Components/Widgets/ToolBar.h>
#include <QMenu>
#include <QToolButton>
#include <QAction>
AZ_POP_DISABLE_WARNING

namespace ShaderManagementConsole
{
    ShaderManagementConsoleToolBar::ShaderManagementConsoleToolBar(QWidget* parent)
        : QToolBar(parent)
    {
        AzQtComponents::ToolBar::addMainToolBarStyle(this);
    }

    ShaderManagementConsoleToolBar::~ShaderManagementConsoleToolBar()
    {
    }
} // namespace ShaderManagementConsole

#include <Source/Window/ToolBar/moc_ShaderManagementConsoleToolBar.cpp>
