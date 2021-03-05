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
#include <AzCore/Component/Entity.h>

struct IRenderNode;

namespace LmbrCentral
{
    /*!
     * Messages services by anything that adds an IRenderNode to an Entity.
     */
    class RenderNodeRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        // Any number of handlers per EntityId, called in order.
        using BusIdType = AZ::EntityId;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::MultipleAndOrdered;
        bool Compare(const RenderNodeRequests* rhs) const
        {
            return GetRenderNodeRequestBusOrder() < rhs->GetRenderNodeRequestBusOrder();
        }
        //////////////////////////////////////////////////////////////////////////

        virtual IRenderNode* GetRenderNode() = 0;

        //! Order in which each bus handler is invoked, lower numbers are first.
        //! In situations where only one render node is expected,
        //! the first bus handler is used.
        virtual float GetRenderNodeRequestBusOrder() const = 0;
    };

    using RenderNodeRequestBus = AZ::EBus<RenderNodeRequests>;
} // namespace LmbrCentral
