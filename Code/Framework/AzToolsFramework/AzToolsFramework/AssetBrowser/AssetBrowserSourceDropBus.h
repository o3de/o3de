/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/string/string.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserSourceDropEvents
            : public AZ::EBusTraits
        {
        public:
            virtual ~AssetBrowserSourceDropEvents() = default;

            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

            // File extension, includes dot.
            using BusIdType = AZStd::string;

            //! Handles source files dropped from the AssetBrowser into the scene.
            virtual void HandleSourceFileType(AZStd::string_view sourceFilePath, AZ::EntityId parentId, AZ::Vector3 position) const = 0;
        };

        using AssetBrowserSourceDropBus = AZ::EBus<AssetBrowserSourceDropEvents>;

    } // namespace AssetBrowser
} // namespace AzToolsFramework
