/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AtomCore/Instance/InstanceId.h>

namespace MaterialEditor
{
    //! MaterialDocumentFactoryRequestBus provides a factory interface for creating and destroying material documents (in memory)
    class MaterialDocumentFactoryRequests
        : public AZ::EBusTraits
    {
    public:
        // Only a single handler is allowed
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Create a material document object
        //! @return Uuid of new material document, or null Uuid if failed
        virtual AZ::Uuid CreateDocument() = 0;

        //! Destroy a material document object with the specified id
        //! @return true if Uuid was found and removed, otherwise false
        virtual bool DestroyDocument(const AZ::Uuid& documentId) = 0;
    };

    using MaterialDocumentFactoryRequestBus = AZ::EBus<MaterialDocumentFactoryRequests>;

} // namespace MaterialEditor
