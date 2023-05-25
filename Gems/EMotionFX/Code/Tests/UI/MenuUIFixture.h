/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Tests/SystemComponentFixture.h>
#include <Tests/UI/UIFixture.h>

#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>

#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>

#include <QString>

namespace EMotionFX
{
    class MenuUIFixture
        : public UIFixture
    {
    public:
        static QMenu *FindMainMenuWithName(const QString& menuName);
        static QMenu* FindMenuWithName(const QObject* parent, const QString& objectName);
        static QAction* FindMenuAction(const QMenu* menu, const QString itemName, const QString& parentName);
        static QAction* FindMenuActionWithObjectName(const QMenu* menu, const QString& itemName, const QString& parentName);
    protected:
       
    };
} // end namespace EMotionFX
