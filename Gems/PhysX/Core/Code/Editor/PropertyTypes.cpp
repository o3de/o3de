/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/CollisionLayerWidget.h>
#include <Editor/CollisionGroupWidget.h>
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
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew PhysX::Editor::InertiaPropertyHandler());
        }

        void UnregisterPropertyTypes()
        {
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew PhysX::Editor::CollisionLayerWidget());
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew PhysX::Editor::CollisionGroupWidget());
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew PhysX::Editor::CollisionGroupEnumPropertyComboBoxHandler());
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, aznew PhysX::Editor::InertiaPropertyHandler());
        }
    } // namespace Editor
} // namespace PhysX
