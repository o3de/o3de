/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

 #include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetDatabaseLocationNotifications)
AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserComponentRequests)
AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserComponentNotifications)
AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotifications)
AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserModelRequests)
AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserModelNotifications)
AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserViewRequests)
AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequest)
AZ_INSTANTIATE_EBUS_MULTI_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications)
AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserFileActionNotifications)
AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserFavoriteRequests)
AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserFavoritesNotifications)
