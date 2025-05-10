/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Viewport/ViewportMessagesBus.h>

AZTF_INSTANTIATE_EBUS_MULTI_ADDRESS_WITH_TRAITS(AzToolsFramework::ViewportInteraction::MouseViewportRequests, AzToolsFramework::ViewportInteraction::ViewportRequestsEBusTraits);
AZTF_INSTANTIATE_EBUS_MULTI_ADDRESS_WITH_TRAITS(AzToolsFramework::ViewportInteraction::ViewportInteractionRequests, AzToolsFramework::ViewportInteraction::ViewportRequestsEBusTraits);
AZTF_INSTANTIATE_EBUS_MULTI_ADDRESS_WITH_TRAITS(AzToolsFramework::ViewportInteraction::ViewportInteractionNotifications, AzToolsFramework::ViewportInteraction::ViewportNotificationsEBusTraits);
AZTF_INSTANTIATE_EBUS_MULTI_ADDRESS_WITH_TRAITS(AzToolsFramework::ViewportInteraction::ViewportSettingNotifications, AzToolsFramework::ViewportInteraction::ViewportNotificationsEBusTraits);
AZTF_INSTANTIATE_EBUS_MULTI_ADDRESS_WITH_TRAITS(AzToolsFramework::ViewportInteraction::MainEditorViewportInteractionRequests, AzToolsFramework::ViewportInteraction::ViewportNotificationsEBusTraits);
AZTF_INSTANTIATE_EBUS_MULTI_ADDRESS_WITH_TRAITS(AzToolsFramework::ViewportInteraction::EditorEntityViewportInteractionRequests, AzToolsFramework::ViewportInteraction::ViewportNotificationsEBusTraits);
AZTF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzToolsFramework::ViewportInteraction::EditorModifierKeyRequests);
AZTF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzToolsFramework::ViewportInteraction::EditorViewportInputTimeNowRequests);
AZTF_INSTANTIATE_EBUS_MULTI_ADDRESS_WITH_TRAITS(AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequests, AzToolsFramework::ViewportInteraction::ViewportNotificationsEBusTraits);
