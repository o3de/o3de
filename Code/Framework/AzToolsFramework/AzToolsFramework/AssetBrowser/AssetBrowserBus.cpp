/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

 #include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

AZTF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzToolsFramework::AssetBrowser::AssetDatabaseLocationNotifications)
AZTF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzToolsFramework::AssetBrowser::AssetBrowserComponentRequests)
AZTF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzToolsFramework::AssetBrowser::AssetBrowserComponentNotifications)
AZTF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotifications)
AZTF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzToolsFramework::AssetBrowser::AssetBrowserModelRequests)
AZTF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzToolsFramework::AssetBrowser::AssetBrowserModelNotifications)
AZTF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzToolsFramework::AssetBrowser::AssetBrowserViewRequests)
AZTF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequest)
AZTF_INSTANTIATE_EBUS_MULTI_ADDRESS(AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications)
AZTF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzToolsFramework::AssetBrowser::AssetBrowserFileActionNotifications)
AZTF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzToolsFramework::AssetBrowser::AssetBrowserFavoriteRequests)
AZTF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzToolsFramework::AssetBrowser::AssetBrowserFavoritesNotifications)
