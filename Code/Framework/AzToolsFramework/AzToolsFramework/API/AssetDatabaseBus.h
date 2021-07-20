/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZTOOLSFRAMEWORK_ASSETDATABASEAPI_H
#define AZTOOLSFRAMEWORK_ASSETDATABASEAPI_H

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AzToolsFramework
{
    namespace AssetDatabase
    {
        class ProductDatabaseEntry;
        class SourceDatabaseEntry;
        typedef AZStd::vector<ProductDatabaseEntry> ProductDatabaseEntryContainer;

        /**
        * Bus used by the Tools Asset Database itself to talk to the running application environment
        * Functions on this bus could be implemented by different parts of the application
        * and are thus not result-based, but instead passed in params which are expected to not be touched
        * unless you have an answer to give.
        */
        class AssetDatabaseRequests
            : public AZ::EBusTraits
        {
        public:
            using Bus = AZ::EBus<AssetDatabaseRequests>;

            /*!
             * Used to retrieve the current database location from the running environment
             */
            virtual bool GetAssetDatabaseLocation(AZStd::string& location) = 0;
        };

        using AssetDatabaseRequestsBus = AZ::EBus<AssetDatabaseRequests>;

        class AssetDatabaseNotifications
            : public AZ::EBusTraits
        {
        public:
            typedef AZStd::recursive_mutex MutexType;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // multiple listeners
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

            virtual ~AssetDatabaseNotifications() = default;

            virtual void OnSourceFileChanged(const SourceDatabaseEntry& /*entry*/) {}
            virtual void OnSourceFileRemoved(AZ::s64 /*sourceId*/) {}

            virtual void OnProductFileChanged(const ProductDatabaseEntry& /*entry*/) {}
            virtual void OnProductFileRemoved(AZ::s64 /*productId*/) {}
            virtual void OnProductFilesRemoved(const ProductDatabaseEntryContainer& /*products*/) {}
            
        };
        using AssetDatabaseNotificationBus = AZ::EBus<AssetDatabaseNotifications>;

    } // namespace AssetDatabase
} // namespace AzToolsFramework

#endif // AZTOOLSFRAMEWORK_ASSETDATABASEAPI_H
