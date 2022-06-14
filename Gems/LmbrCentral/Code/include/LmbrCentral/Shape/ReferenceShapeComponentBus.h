/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>

namespace LmbrCentral
{
    // Type ID for Reference EditorReferenceShapeComponent
    static const char* EditorReferenceShapeComponentTypeId = "{21BC79CA-C2F4-428F-AF2E-B76E233D4254}";

    class ReferenceShapeRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual AZ::EntityId GetShapeEntityId() const = 0;
        virtual void SetShapeEntityId(AZ::EntityId entityId) = 0;
    };

    using ReferenceShapeRequestBus = AZ::EBus<ReferenceShapeRequests>;
}
