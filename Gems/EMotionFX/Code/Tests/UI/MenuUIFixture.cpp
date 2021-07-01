/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/UI/UIFixture.h>
#include <Tests/UI/MenuUIFixture.h>
#include <Integration/System/SystemCommon.h>

#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphObject.h>

#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/PluginManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>

#include <EMotionFX/Source/AnimGraphReferenceNode.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>

#include <QApplication>
#include <QWidget>

namespace EMotionFX
{
    QMenu *MenuUIFixture::FindMainMenuWithName(const QString& menuName)
    {
        const QList<QMenu*> menus = EMStudio::GetMainWindow()->findChildren<QMenu*>();
        for (QMenu* menu : menus)
        {
            if (menu->objectName() == menuName)
            {
                return menu;
            }
        }

        return nullptr;
    }

    QMenu* MenuUIFixture::FindMenuWithName(const QObject* parent, const QString& objectName)
    {
        const QList<QMenu*> menus = parent->findChildren<QMenu*>();
        for (QMenu* menu : menus)
        {
            if (menu->objectName() == objectName)
            {
                return menu;
            }
        }

        return nullptr;
    }

    QAction* MenuUIFixture::FindMenuAction(const QMenu* menu, const QString itemName, const QString& parentName)
    {
        const QList<QAction*> actions = menu->findChildren<QAction*>();
        for (QAction* action : actions)
        {
            if (action->text() == itemName && action->parent()->objectName() == parentName)
            {
                return action;
            }
        }

        return nullptr;
    }

    QAction* MenuUIFixture::FindMenuActionWithObjectName(const QMenu* menu, const QString& itemName, const QString& parentName)
    {
        const QList<QAction*> actions = menu->findChildren<QAction*>();
        for (QAction* action : actions)
        {
            if (action->objectName() == itemName && action->parent()->objectName() == parentName)
            {
                return action;
            }
        }

        return nullptr;
    }
} // namespace EMotionFX
