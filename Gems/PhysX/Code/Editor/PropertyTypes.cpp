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

#include <PhysX_precompiled.h>

#include <Editor/CollisionLayerWidget.h>
#include <Editor/CollisionGroupWidget.h>
#include <Editor/MaterialIdWidget.h>
#include <Editor/InertiaPropertyHandler.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

namespace PhysX
{
    namespace Editor
    {
        void RegisterPropertyTypes()
        {
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew PhysX::Editor::CollisionLayerWidget());
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew PhysX::Editor::CollisionGroupWidget());
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew PhysX::Editor::CollisionGroupEnumPropertyComboBoxHandler());
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew PhysX::Editor::MaterialIdWidget());
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew PhysX::Editor::InertiaPropertyHandler());
        }

        void UnregisterPropertyTypes()
        {
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew PhysX::Editor::CollisionLayerWidget());
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew PhysX::Editor::CollisionGroupWidget());
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew PhysX::Editor::CollisionGroupEnumPropertyComboBoxHandler());
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew PhysX::Editor::MaterialIdWidget());
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew PhysX::Editor::InertiaPropertyHandler());
        }
    } // namespace Editor
} // namespace PhysX
