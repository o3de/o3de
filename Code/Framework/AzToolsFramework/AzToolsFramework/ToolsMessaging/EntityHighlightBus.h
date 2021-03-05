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
#ifndef ENTITY_HIGHLIGHT_BUS_H
#define ENTITY_HIGHLIGHT_BUS_H

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>

namespace AzToolsFramework
{
    class EntityHighlightMessages
        : public AZ::EBusTraits
    {
    public:

        using Bus = AZ::EBus<EntityHighlightMessages>;

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // there is one bus that everyone on it listens to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // every listener registers unordered on this bus

        virtual void EntityHighlightRequested(AZ::EntityId) = 0;
        virtual void EntityStrongHighlightRequested(AZ::EntityId) = 0;
    };
}

#endif // EDITOR_ENTITY_ID_LIST_CONTAINER_H