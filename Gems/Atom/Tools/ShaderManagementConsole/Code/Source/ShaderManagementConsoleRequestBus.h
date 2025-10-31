/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>
#include <Atom/RPI.Reflect/Material/ShaderCollection.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionTypes.h>

namespace ShaderManagementConsole
{
    //! ShaderManagementConsoleRequestBus provides
    class ShaderManagementConsoleRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Returns a shader file's asset id and relative filepath
        virtual AZ::Data::AssetInfo GetSourceAssetInfo(const AZStd::string& sourceAssetFileName) = 0;

        //! Returns a list of material AssetIds that use the shader file.
        virtual AZStd::vector<AZ::Data::AssetId> FindMaterialAssetsUsingShader (const AZStd::string& shaderFilePath) = 0;

        //! Returns a list of shader items contained within an instantiated material source's shader collection.
        virtual AZStd::vector<AZ::RPI::ShaderCollection::Item> GetMaterialInstanceShaderItems(const AZ::Data::AssetId& assetId) = 0;

        //! Return the id of all built material assets
        virtual AZStd::vector<AZ::Data::AssetId> GetAllMaterialAssetIds() = 0;

        //! A wrapper to call GenerateRelativeSourcePath from python
        virtual AZStd::string GenerateRelativeSourcePath(const AZStd::string& fullShaderPath) = 0;

        //! A convenience function to generate proper ShaderOptionValue instances from scripts
        virtual AZ::RPI::ShaderOptionValue MakeShaderOptionValueFromInt(int value) = 0;

    };
    using ShaderManagementConsoleRequestBus = AZ::EBus<ShaderManagementConsoleRequests>;
} // namespace ShaderManagementConsole
