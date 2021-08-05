/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>

namespace ScriptCanvasEditor
{
    class PropertyGridRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        
        virtual void RefreshPropertyGrid() = 0;
        virtual void RebuildPropertyGrid() = 0;

        virtual void SetSelection(const AZStd::vector<AZ::EntityId>& selectedEntityIds) = 0;
        virtual void ClearSelection() = 0;
    };
    
    using PropertyGridRequestBus = AZ::EBus<PropertyGridRequests>;
}
