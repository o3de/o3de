/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzFramework/Entity/EntityContextBus.h>

namespace AzToolsFramework
{
    /**
    * Bus for making entity picking requests to a given editor entity context given a context Id.
    */
    class EditorEntityContextPickingRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AzFramework::EntityContextId BusIdType;
        //////////////////////////////////////////////////////////////////////////

        /// Check whether the entity context supports picking an entityId from the viewport
        /// \return true if the entity context supports picking an entityId from the viewport, false otherwise
        virtual bool SupportsViewportEntityIdPicking() = 0;
    };

    using EditorEntityContextPickingRequestBus = AZ::EBus<EditorEntityContextPickingRequests>;

} // namespace AzToolsFramework
