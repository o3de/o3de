/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzToolsFramework/AssetEditor/AssetEditorBus.h>

AZTF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzToolsFramework::AssetEditor::AssetEditorRequests);
AZTF_INSTANTIATE_EBUS_MULTI_ADDRESS(AzToolsFramework::AssetEditor::AssetEditorValidationRequests);
AZTF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzToolsFramework::AssetEditor::AssetEditorWidgetRequests);
AZTF_INSTANTIATE_EBUS_MULTI_ADDRESS(AzToolsFramework::AssetEditor::AssetEditorNotifications);
