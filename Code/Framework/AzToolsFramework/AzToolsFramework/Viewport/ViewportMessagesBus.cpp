/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Viewport/ViewportMessagesBus.h>

AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS_WITH_TRAITS(AZTF_API, AzToolsFramework::ViewportInteraction::MouseViewportRequests, AzToolsFramework::ViewportInteraction::ViewportRequestsEBusTraits);
AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS_WITH_TRAITS(AZTF_API, AzToolsFramework::ViewportInteraction::ViewportInteractionRequests, AzToolsFramework::ViewportInteraction::ViewportRequestsEBusTraits);
AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS_WITH_TRAITS(AZTF_API, AzToolsFramework::ViewportInteraction::ViewportInteractionNotifications, AzToolsFramework::ViewportInteraction::ViewportNotificationsEBusTraits);
AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS_WITH_TRAITS(AZTF_API, AzToolsFramework::ViewportInteraction::ViewportSettingNotifications, AzToolsFramework::ViewportInteraction::ViewportNotificationsEBusTraits);
AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS_WITH_TRAITS(AZTF_API, AzToolsFramework::ViewportInteraction::MainEditorViewportInteractionRequests, AzToolsFramework::ViewportInteraction::ViewportNotificationsEBusTraits);
AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS_WITH_TRAITS(AZTF_API, AzToolsFramework::ViewportInteraction::EditorEntityViewportInteractionRequests, AzToolsFramework::ViewportInteraction::ViewportNotificationsEBusTraits);
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::ViewportInteraction::EditorModifierKeyRequests);
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::ViewportInteraction::EditorViewportInputTimeNowRequests);
AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS_WITH_TRAITS(AZTF_API, AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequests, AzToolsFramework::ViewportInteraction::ViewportNotificationsEBusTraits);
