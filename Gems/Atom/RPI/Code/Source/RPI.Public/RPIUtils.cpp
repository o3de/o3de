/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/RHISystemInterface.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Shader/Shader.h>

#include <AzFramework/Asset/AssetSystemBus.h>

namespace AZ
{
    namespace RPI
    {

        Data::AssetId GetShaderAssetId(const AZStd::string& shaderFilePath, bool isCritical)
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
                if (isCritical)
                {
                    Data::Asset<RPI::ShaderAsset> shaderAsset = RPI::AssetUtils::LoadCriticalAsset<RPI::ShaderAsset>(shaderFilePath);
                    if (shaderAsset.IsReady())
                    {
                        return shaderAsset.GetId();
                    }
                    else
                    {
                        AZ_Error("RPI Utils", false, "Could not load critical shader [%s]", shaderFilePath.c_str());
                    }
                }

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

        Data::Asset<ShaderAsset> FindCriticalShaderAsset(const AZStd::string& shaderFilePath)
        {
            const bool isCritical = true;
            return FindShaderAsset(GetShaderAssetId(shaderFilePath, isCritical), shaderFilePath);
        }

        Data::Instance<Shader> LoadShader(const AZStd::string& shaderFilePath)
        {
            return LoadShader(GetShaderAssetId(shaderFilePath), shaderFilePath);
        }

        Data::Instance<Shader> LoadCriticalShader(const AZStd::string& shaderFilePath)
        {
            const bool isCritical = true;
            return LoadShader(GetShaderAssetId(shaderFilePath, isCritical), shaderFilePath);
        }

        AZ::Data::Instance<RPI::StreamingImage> LoadStreamingTexture(AZStd::string_view path)
        {
            AzFramework::AssetSystem::AssetStatus status = AzFramework::AssetSystem::AssetStatus_Unknown;
            AzFramework::AssetSystemRequestBus::BroadcastResult(
                status, &AzFramework::AssetSystemRequestBus::Events::CompileAssetSync, path);

            // When running with no Asset Processor (for example in release), CompileAssetSync will return AssetStatus_Unknown.
            AZ_Error(
                "RPIUtils",
                status == AzFramework::AssetSystem::AssetStatus_Compiled || status == AzFramework::AssetSystem::AssetStatus_Unknown,
                "Could not compile image at '%s'", path.data());

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

        //! A helper function for GetComputeShaderNumThreads(), to consolidate error messages, etc.
        static bool GetAttributeArgumentByIndex(const Data::Asset<ShaderAsset>& shaderAsset, const AZ::Name& attributeName, const RHI::ShaderStageAttributeArguments& args, const size_t argIndex, uint16_t* value, AZStd::string& errorMsg)
        {
            if (value)
            {
                const auto numArguments = args.size();
                if (numArguments > argIndex)
                {
                    if (args[argIndex].type() == azrtti_typeid<int>())
                    {
                        *value = aznumeric_caster(AZStd::any_cast<int>(args[argIndex]));
                    }
                    else
                    {
                        errorMsg = AZStd::string::format("Was expecting argument '%zu' in attribute '%s' to be of type 'int' from shader asset '%s'", argIndex, attributeName.GetCStr(), shaderAsset.GetHint().c_str());
                        return false;
                    }
                }
                else
                {
                     errorMsg = AZStd::string::format("Was expecting at least '%zu' arguments in attribute '%s' from shader asset '%s'", argIndex + 1, attributeName.GetCStr(), shaderAsset.GetHint().c_str());
                     return false;
                }
            }
            return true;
        }

        AZ::Outcome<void, AZStd::string> GetComputeShaderNumThreads(const Data::Asset<ShaderAsset>& shaderAsset, const AZ::Name& attributeName, uint16_t* numThreadsX, uint16_t* numThreadsY, uint16_t* numThreadsZ)
        {
            // Set default 1, 1, 1 now. In case of errors later this is what the caller will get.
            if (numThreadsX)
            {
                *numThreadsX = 1;
            }
            if (numThreadsY)
            {
                *numThreadsY = 1;
            }
            if (numThreadsZ)
            {
                *numThreadsZ = 1;
            }
            const auto numThreads = shaderAsset->GetAttribute(RHI::ShaderStage::Compute, attributeName);
            if (!numThreads)
            {
                return AZ::Failure(AZStd::string::format("Couldn't find attribute '%s' in shader asset '%s'", attributeName.GetCStr(), shaderAsset.GetHint().c_str()));
            }
            const RHI::ShaderStageAttributeArguments& args = *numThreads;
            AZStd::string errorMsg;
            if (!GetAttributeArgumentByIndex(shaderAsset, attributeName, args, 0, numThreadsX, errorMsg))
            {
                return AZ::Failure(errorMsg);
            }
            if (!GetAttributeArgumentByIndex(shaderAsset, attributeName, args, 1, numThreadsY, errorMsg))
            {
                return AZ::Failure(errorMsg);
            }
            if (!GetAttributeArgumentByIndex(shaderAsset, attributeName, args, 2, numThreadsZ, errorMsg))
            {
                return AZ::Failure(errorMsg);
            }
            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> GetComputeShaderNumThreads(const Data::Asset<ShaderAsset>& shaderAsset, uint16_t* numThreadsX, uint16_t* numThreadsY, uint16_t* numThreadsZ)
        {
            return GetComputeShaderNumThreads(shaderAsset, Name{ "numthreads" }, numThreadsX, numThreadsY, numThreadsZ);
        }

        AZ::Outcome<void, AZStd::string> GetComputeShaderNumThreads(const Data::Asset<ShaderAsset>& shaderAsset, RHI::DispatchDirect& dispatchDirect)
        {
            return GetComputeShaderNumThreads(shaderAsset, &dispatchDirect.m_threadsPerGroupX, &dispatchDirect.m_threadsPerGroupY, &dispatchDirect.m_threadsPerGroupZ);
        }
    }
}
