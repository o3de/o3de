/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <AzAssetBrowser/AzAssetBrowserWindow.h>
 
namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AzAssetBrowserMultiWindow
        {
        public:
            static AzAssetBrowserWindow* OpenNewAssetBrowserWindow();
            static bool IsAnyAssetBrowserWindowOpen();
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
