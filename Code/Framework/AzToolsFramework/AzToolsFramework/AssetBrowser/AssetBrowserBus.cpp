/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

 #include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetDatabaseLocationNotifications)
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserComponentRequests)
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserComponentNotifications)
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotifications)
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserModelRequests)
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserModelNotifications)
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserViewRequests)
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequest)
AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications)
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserFileActionNotifications)
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserFavoriteRequests)
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::AssetBrowser::AssetBrowserFavoritesNotifications)
