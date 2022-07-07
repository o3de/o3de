/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AzToolsFramework
{
    class PropertyAssetCtrlRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! Inform any interested components that a new asset has appeared that they may be waiting for.
        virtual void OnExpectedCatalogAssetAdded(const AZ::Data::AssetId& assetId, const AZ::EntityId& entityId, const AZ::ComponentId& componentId) = 0;
    };
        
    using PropertyAssetCtrlRequestsBus = AZ::EBus<PropertyAssetCtrlRequests>;
} // namespace AzToolsFramework
