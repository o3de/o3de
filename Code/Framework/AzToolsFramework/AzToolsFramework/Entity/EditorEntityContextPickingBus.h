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