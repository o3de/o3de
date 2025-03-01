/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/ToolsApplicationBus.h>

AZTF_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AzToolsFramework::ToolsApplicationEvents);
AZTF_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AzToolsFramework::ToolsApplicationRequests);
AZTF_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(AzToolsFramework::EntitySelectionEvents);
AZTF_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(AzToolsFramework::EditorPickModeRequests);
AZTF_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(AzToolsFramework::EditorPickModeNotifications);
AZTF_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AzToolsFramework::EditorRequests);
AZTF_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AzToolsFramework::EditorEvents);
AZTF_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(AzToolsFramework::ViewPaneCallbacks);
