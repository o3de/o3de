/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzToolsFramework/AssetEditor/AssetEditorBus.h>

AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetEditor::AssetEditorRequests);
AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(AZTF_API, AzToolsFramework::AssetEditor::AssetEditorValidationRequests);
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetEditor::AssetEditorWidgetRequests);
AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(AZTF_API, AzToolsFramework::AssetEditor::AssetEditorNotifications);
