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

#pragma once

#include <Tests/SystemComponentFixture.h>
#include <Tests/UI/UIFixture.h>

#include <AzCore/Memory/MemoryComponent.h>
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
