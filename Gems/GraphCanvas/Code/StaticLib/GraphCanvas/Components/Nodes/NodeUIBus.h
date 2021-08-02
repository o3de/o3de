/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <qglobal.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>

namespace GraphCanvas
{
    //! NodeUIRequests
    //! Requests involving the visual state/behavior of a node.
    //! Generally independent of any sort of logical underpinning and more related to the UX
    //! of the node.
    class NodeUIRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void AdjustSize() = 0;

        virtual void SetSnapToGrid(bool enabled) = 0;
        virtual void SetResizeToGrid(bool enabled) = 0;
        virtual void SetGrid(AZ::EntityId gridId) = 0;

        virtual void SetSteppedSizingEnabled(bool sizing) = 0;

        virtual qreal GetCornerRadius() const = 0;
        virtual qreal GetBorderWidth() const = 0;
    };

    using NodeUIRequestBus = AZ::EBus<NodeUIRequests>;
}
