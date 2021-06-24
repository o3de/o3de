/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Reflect/Shader/ShaderVariantAsset2.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>

#include <Atom/RPI.Reflect/Shader/ShaderCommonTypes.h>
#include <Atom/RHI.Reflect/ShaderStageFunction.h>
#include <Atom/RHI.Reflect/Limits.h>

namespace AZ
{
    namespace RPI
    {
        uint32_t ShaderVariantAsset2::MakeAssetProductSubId(
            uint32_t rhiApiUniqueIndex, uint32_t supervariantIndex, ShaderVariantStableId variantStableId, uint32_t subProductType)
        {
            static constexpr uint32_t SubProductTypeBitPosition = 17;
            static constexpr uint32_t SubProductTypeNumBits = SupervariantIndexBitPosition - SubProductTypeBitPosition;
            static constexpr uint32_t SubProductTypeMaxValue = (1 << SubProductTypeNumBits) - 1;

            static constexpr uint32_t StableIdBitPosition = 0;
            static constexpr uint32_t StableIdNumBits = SubProductTypeBitPosition - StableIdBitPosition;
            static constexpr uint32_t StableIdMaxValue = (1 << StableIdNumBits) - 1;

            static_assert(RhiIndexMaxValue == RHI::Limits::APIType::PerPlatformApiUniqueIndexMax);

            // The 2 Most significant bits encode the the RHI::API unique index.
            AZ_Assert(rhiApiUniqueIndex <= RhiIndexMaxValue, "Invalid rhiApiUniqueIndex [%u]", rhiApiUniqueIndex);
            AZ_Assert(supervariantIndex <= SupervariantIndexMaxValue, "Invalid supervariantIndex [%u]", supervariantIndex);
            AZ_Assert(subProductType <= SubProductTypeMaxValue, "Invalid subProductType [%u]", subProductType);
            AZ_Assert(variantStableId.GetIndex() <= StableIdMaxValue, "Invalid variantStableId [%u]", variantStableId.GetIndex());

            const uint32_t assetProductSubId = (rhiApiUniqueIndex << RhiIndexBitPosition) |
                (supervariantIndex << SupervariantIndexBitPosition) | (subProductType << SubProductTypeBitPosition) |
                (variantStableId.GetIndex() << StableIdBitPosition);
            return assetProductSubId;
        }

        void ShaderVariantAsset2::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderVariantAsset2, AZ::Data::AssetData>()
                    ->Version(1)
                    ->Field("StableId", &ShaderVariantAsset2::m_stableId)
                    ->Field("ShaderVariantId", &ShaderVariantAsset2::m_shaderVariantId)
                    ->Field("IsFullyBaked", &ShaderVariantAsset2::m_isFullyBaked)
                    ->Field("FunctionsByStage", &ShaderVariantAsset2::m_functionsByStage)
                    ->Field("BuildTimestamp", &ShaderVariantAsset2::m_buildTimestamp)
                    ;
            }
        }

        AZStd::sys_time_t ShaderVariantAsset2::GetBuildTimestamp() const
        {
            return m_buildTimestamp;
        }

        const RHI::ShaderStageFunction* ShaderVariantAsset2::GetShaderStageFunction(RHI::ShaderStage shaderStage) const
        {
            return m_functionsByStage[static_cast<size_t>(shaderStage)].get();
        }

        bool ShaderVariantAsset2::IsFullyBaked() const
        {
            return m_isFullyBaked;
        }

        void ShaderVariantAsset2::SetReady()
        {
            m_status = AssetStatus::Ready;
        }

        bool ShaderVariantAsset2::FinalizeAfterLoad()
        {
            return true;
        }

        ShaderVariantAssetHandler2::LoadResult ShaderVariantAssetHandler2::LoadAssetData(const Data::Asset<Data::AssetData>& asset, AZStd::shared_ptr<Data::AssetDataStream> stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            if (Base::LoadAssetData(asset, stream, assetLoadFilterCB) == LoadResult::LoadComplete)
            {
                return PostLoadInit(asset) ? LoadResult::LoadComplete : LoadResult::Error;
            }
            return LoadResult::Error;
        }

        bool ShaderVariantAssetHandler2::PostLoadInit(const Data::Asset<Data::AssetData>& asset)
        {
            if (ShaderVariantAsset2* shaderVariantAsset = asset.GetAs<ShaderVariantAsset2>())
            {
                if (!shaderVariantAsset->FinalizeAfterLoad())
                {
                    AZ_Error("ShaderVariantAssetHandler", false, "Shader asset failed to finalize.");
                    return false;
                }
                return true;
            }
            return false;
        }   
    } // namespace RPI
} // namespace AZ
