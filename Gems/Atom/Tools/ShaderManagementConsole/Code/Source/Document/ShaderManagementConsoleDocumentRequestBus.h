/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Edit/Shader/ShaderSourceData.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>
#include <AzCore/Asset/AssetCommon.h>

namespace ShaderManagementConsole
{
    class ShaderManagementConsoleDocumentRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Uuid BusIdType;

        //! Set the shader variant list source data on the document.
        //! This function can be used to edit and update the data contained within the document.
        //! Functions can be added to this bus for more fine grained editing of shader variant list data.
        virtual void SetShaderVariantListSourceData(const AZ::RPI::ShaderVariantListSourceData& shaderVariantListSourceData) = 0;

        //! Get the shader variant list source data from the document.
        virtual const AZ::RPI::ShaderVariantListSourceData& GetShaderVariantListSourceData() const = 0;

        //! Get the number of shader options stored in the shader asset.
        //! Note that the shader asset can contain more descriptors than are stored in the shader variant list source data.
        virtual size_t GetShaderOptionDescriptorCount() const = 0;

        //! Get the shader option descriptor from the shader asset.
        //! Note that the shader asset can contain more descriptors than are stored in the shader variant list source data.
        virtual const AZ::RPI::ShaderOptionDescriptor& GetShaderOptionDescriptor(size_t index) const = 0;
    };

    using ShaderManagementConsoleDocumentRequestBus = AZ::EBus<ShaderManagementConsoleDocumentRequests>;
} // namespace ShaderManagementConsole
