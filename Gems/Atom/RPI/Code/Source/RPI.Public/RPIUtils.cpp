/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/RHISystemInterface.h>

#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Shader/Shader.h>

#include <AzFramework/Asset/AssetSystemBus.h>

namespace AZ
{
    namespace RPI
    {

        Data::AssetId GetShaderAssetId(const AZStd::string& shaderFilePath)
        {
            Data::AssetId shaderAssetId;

            Data::AssetCatalogRequestBus::BroadcastResult(
                shaderAssetId,
                &Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                shaderFilePath.c_str(),
                azrtti_typeid<ShaderAsset>(),
                false
            );

            if (!shaderAssetId.IsValid())
            {
                AZ_Error("RPI Utils", false, "Failed to get asset id for shader [%s]", shaderFilePath.c_str());
            }

            return shaderAssetId;
        }

        Data::Asset<ShaderAsset> FindShaderAsset(Data::AssetId shaderAssetId, [[maybe_unused]] const AZStd::string& shaderFilePath)
        {
            if (!shaderAssetId.IsValid())
            {
                return Data::Asset<ShaderAsset>();
            }

            auto shaderAsset = Data::AssetManager::Instance().GetAsset<ShaderAsset>(shaderAssetId, AZ::Data::AssetLoadBehavior::PreLoad);

            shaderAsset.BlockUntilLoadComplete();

            if (!shaderAsset.IsReady())
            {
                AZ_Error("RPI Utils", false, "Failed to find shader asset [%s] with asset ID [%s]", shaderFilePath.c_str(), shaderAssetId.ToString<AZStd::string>().c_str());
                return Data::Asset<ShaderAsset>();
            }

            return shaderAsset;
        }

        Data::Instance<Shader> LoadShader(Data::AssetId shaderAssetId, const AZStd::string& shaderFilePath)
        {
            auto shaderAsset = FindShaderAsset(shaderAssetId, shaderFilePath);
            if (!shaderAsset)
            {
                return nullptr;
            }

            Data::Instance<Shader> shader = Shader::FindOrCreate(shaderAsset);
            if (!shader)
            {
                AZ_Error("RPI Utils", false, "Failed to find or create a shader instance from shader asset [%s] with asset ID [%s]", shaderFilePath.c_str(), shaderAssetId.ToString<AZStd::string>().c_str());
                return nullptr;
            }

            return shader;
        }

        Data::Asset<ShaderAsset> FindShaderAsset(const AZStd::string& shaderFilePath)
        {
            return FindShaderAsset(GetShaderAssetId(shaderFilePath), shaderFilePath);
        }

        Data::Instance<Shader> LoadShader(const AZStd::string& shaderFilePath)
        {
            return LoadShader(GetShaderAssetId(shaderFilePath), shaderFilePath);
        }

        AZ::Data::Instance<RPI::StreamingImage> LoadStreamingTexture(AZStd::string_view path)
        {
            AzFramework::AssetSystem::AssetStatus status = AzFramework::AssetSystem::AssetStatus_Unknown;
            AzFramework::AssetSystemRequestBus::BroadcastResult(
                status, &AzFramework::AssetSystemRequestBus::Events::CompileAssetSync, path);
            AZ_Error("RPIUtils", status == AzFramework::AssetSystem::AssetStatus_Compiled, "Could not compile image at '%s'", path.data());

            Data::AssetId streamingImageAssetId;
            Data::AssetCatalogRequestBus::BroadcastResult(
                streamingImageAssetId, &Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                path.data(), azrtti_typeid<RPI::StreamingImageAsset>(), false);
            if (!streamingImageAssetId.IsValid())
            {
                AZ_Error("RPI Utils", false, "Failed to get streaming image asset id with path " AZ_STRING_FORMAT, AZ_STRING_ARG(path));
                return AZ::Data::Instance<RPI::StreamingImage>();
            }

            auto streamingImageAsset = Data::AssetManager::Instance().GetAsset<RPI::StreamingImageAsset>(streamingImageAssetId, AZ::Data::AssetLoadBehavior::PreLoad);

            streamingImageAsset.BlockUntilLoadComplete();

            if (!streamingImageAsset.IsReady())
            {
                AZ_Error("RPI Utils", false, "Failed to get streaming image asset: " AZ_STRING_FORMAT, AZ_STRING_ARG(path));
                return AZ::Data::Instance<RPI::StreamingImage>();
            }

            return RPI::StreamingImage::FindOrCreate(streamingImageAsset);
        }
    }
}
