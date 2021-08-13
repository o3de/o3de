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

    class ShaderManagementConsoleDocumentRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Uuid BusIdType;

        //! Get the number of options
        virtual size_t GetShaderOptionCount() const = 0;

        //! Get the descriptor for the shader option at the specified index
        virtual const AZ::RPI::ShaderOptionDescriptor& GetShaderOptionDescriptor(size_t index) const = 0;

        //! Get the number of shader variants
        virtual size_t GetShaderVariantCount() const = 0;

        //! Get the information for the shader variant at the specified index
        virtual const AZ::RPI::ShaderVariantListSourceData::VariantInfo& GetShaderVariantInfo(size_t index) const = 0;
    };

    using ShaderManagementConsoleDocumentRequestBus = AZ::EBus<ShaderManagementConsoleDocumentRequests>;
} // namespace ShaderManagementConsole
