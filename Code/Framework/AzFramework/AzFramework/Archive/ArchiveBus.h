/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/Asset/AssetBundleManifest.h>
#include <AzFramework/Asset/AssetRegistry.h>

namespace AZ::IO
{
    /*!
     * Events from Archive
     */
    class ArchiveNotifications
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void BundleOpened([[maybe_unused]] const char* bundleName, AZStd::shared_ptr<AzFramework::AssetBundleManifest> bundleManifest, [[maybe_unused]] const char* nextBundle, AZStd::shared_ptr<AzFramework::AssetRegistry> bundleCatalog) {}
        virtual void BundleClosed([[maybe_unused]] const char* bundleName) {}

        // Sent when a file is accessed through Archive
        virtual void FileAccess([[maybe_unused]] const char* filePath) {}
    };
    using ArchiveNotificationBus = AZ::EBus<ArchiveNotifications>;

}
