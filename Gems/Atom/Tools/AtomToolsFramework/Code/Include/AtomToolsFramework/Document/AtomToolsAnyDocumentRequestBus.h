/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/any.h>

namespace AtomToolsFramework
{
    // AtomToolsAnyDocumentRequests declares an interface for a general purpose document class that can be used to serialize and edit
    // objects using standard serialize and edit context reflection within atom tools.
    class AtomToolsAnyDocumentRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Uuid BusIdType;

        // Get the object containing the content.
        virtual const AZStd::any& GetContent() const = 0;
    };

    using AtomToolsAnyDocumentRequestBus = AZ::EBus<AtomToolsAnyDocumentRequests>;
} // namespace AtomToolsFramework
