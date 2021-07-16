/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
