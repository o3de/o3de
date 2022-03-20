/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>

namespace MaterialCanvas
{
    class MaterialCanvasDocumentRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Uuid BusIdType;

        virtual GraphCanvas::GraphId GetGraphId() const = 0;
    };

    using MaterialCanvasDocumentRequestBus = AZ::EBus<MaterialCanvasDocumentRequests>;
} // namespace MaterialCanvas
