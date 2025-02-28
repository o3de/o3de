/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/ToolsApplicationBus.h>

DECLARE_EBUS_INSTANTIATION_DLL_SINGLE_ADDRESS(AzToolsFramework::ToolsApplicationEvents);
DECLARE_EBUS_INSTANTIATION_DLL_SINGLE_ADDRESS(AzToolsFramework::ToolsApplicationRequests);
DECLARE_EBUS_INSTANTIATION_DLL_MULTI_ADDRESS(AzToolsFramework::EntitySelectionEvents);
DECLARE_EBUS_INSTANTIATION_DLL_MULTI_ADDRESS(AzToolsFramework::EditorPickModeRequests);
DECLARE_EBUS_INSTANTIATION_DLL_MULTI_ADDRESS(AzToolsFramework::EditorPickModeNotifications);
DECLARE_EBUS_INSTANTIATION_DLL_SINGLE_ADDRESS(AzToolsFramework::EditorRequests);
DECLARE_EBUS_INSTANTIATION_DLL_SINGLE_ADDRESS(AzToolsFramework::EditorEvents);
DECLARE_EBUS_INSTANTIATION_DLL_MULTI_ADDRESS(AzToolsFramework::ViewPaneCallbacks);
