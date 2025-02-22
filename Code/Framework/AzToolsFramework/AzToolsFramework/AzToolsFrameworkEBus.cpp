/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


 #include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
 DECLARE_EBUS_INSTANTIATION_DLL_SINGLE_ADDRESS(AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests);
 DECLARE_EBUS_INSTANTIATION_DLL_MULTI_ADDRESS_WITH_TRAITS(AzToolsFramework::ComponentModeFramework::ComponentModeDelegateRequests, AzToolsFramework::ComponentModeFramework::ComponentModeMouseViewportRequests)
 
 #include <AzToolsFramework/Entity/EditorEntityContextBus.h>
 DECLARE_EBUS_INSTANTIATION_DLL_SINGLE_ADDRESS(AzToolsFramework::EditorEntityContextRequests);

 #include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
 DECLARE_EBUS_INSTANTIATION_DLL_MULTI_ADDRESS(AzToolsFramework::EditorEntityInfoRequests);
 
#include <AzToolsFramework/Prefab/PrefabPublicNotificationBus.h>
DECLARE_EBUS_INSTANTIATION_DLL_SINGLE_ADDRESS(AzToolsFramework::Prefab::PrefabPublicNotifications);

#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
DECLARE_EBUS_INSTANTIATION_DLL_MULTI_ADDRESS(AzToolsFramework::EditorEntityVisibilityNotifications);

#include <AzToolsFramework/ToolsComponents/EditorComponentBus.h>
DECLARE_EBUS_INSTANTIATION_DLL_MULTI_ADDRESS_WITH_TRAITS(AzToolsFramework::Components::EditorComponentDescriptor, AZ::ComponentDescriptorBusTraits);

#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
DECLARE_EBUS_INSTANTIATION_DLL_MULTI_ADDRESS(AzToolsFramework::EditorEntityLockComponentNotifications);
