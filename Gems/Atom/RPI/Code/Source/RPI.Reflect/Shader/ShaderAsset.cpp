/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderCommonTypes.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>

#include <Atom/RHI/Factory.h>

#include <AzCore/Interface/Interface.h>
#include <Atom/RPI.Reflect/Shader/IShaderVariantFinder.h>
#include <Atom/RPI.Public/Shader/ShaderReloadDebugTracker.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>

namespace AZ
{
    namespace RPI
    {
        const char* ShaderAsset::DisplayName = "Shader";
        const char* ShaderAsset::Extension = "azshader";
        const char* ShaderAsset::Group = "Shader";

        const ShaderVariantStableId ShaderAsset::RootShaderVariantStableId{ 0 };

        uint32_t ShaderAsset::MakeAssetProductSubId(uint32_t rhiApiUniqueIndex, uint32_t subProductType)
        {
            static constexpr uint32_t SubProductTypeBitPosition = 0;
            static constexpr uint32_t SubProductTypeNumBits = RhiIndexBitPosition - SubProductTypeBitPosition;
            static constexpr uint32_t SubProductTypeMaxValue = (1 << SubProductTypeNumBits) - 1;

            static_assert(RhiIndexMaxValue == RHI::Limits::APIType::PerPlatformApiUniqueIndexMax);
            AZ_Assert(rhiApiUniqueIndex <= RhiIndexMaxValue, "Invalid rhiApiUniqueIndex [%u]", rhiApiUniqueIndex);
            AZ_Assert(subProductType <= SubProductTypeMaxValue, "Invalid subProductType [%u]", subProductType);

            const uint32_t assetProductSubId = (rhiApiUniqueIndex << RhiIndexBitPosition) |
                (subProductType << SubProductTypeBitPosition);
            return assetProductSubId;
        }

        void ShaderAsset::ShaderApiDataContainer::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderApiDataContainer>()
                    ->Version(5)
                    ->Field("APIType", &ShaderApiDataContainer::m_APIType)
                    ->Field("PipelineLayoutDescriptor", &ShaderApiDataContainer::m_pipelineLayoutDescriptor)
                    ->Field("RootShaderVariantAsset", &ShaderApiDataContainer::m_rootShaderVariantAsset)
                    ->Field("AttributeMaps", &ShaderApiDataContainer::m_attributeMaps)
                    ;
            }
        }

        void ShaderAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderAsset>()
                    ->Version(6)
                    ->Field("name", &ShaderAsset::m_name)
                    ->Field("pipelineStateType", &ShaderAsset::m_pipelineStateType)
                    ->Field("shaderResourceGroupAssets", &ShaderAsset::m_shaderResourceGroupAssets)
                    ->Field("shaderOptionGroupLayout", &ShaderAsset::m_shaderOptionGroupLayout)
                    ->Field("perAPIShaderData", &ShaderAsset::m_perAPIShaderData)
                    ->Field("drawListName", &ShaderAsset::m_drawListName)
                    ->Field("shaderAssetBuildTimestamp", &ShaderAsset::m_shaderAssetBuildTimestamp)
                    ;
            }

            ShaderApiDataContainer::Reflect(context);
        }

        ShaderAsset::~ShaderAsset()
        {
            Data::AssetBus::Handler::BusDisconnect();
            ShaderVariantFinderNotificationBus::Handler::BusDisconnect();
        }

        const Name& ShaderAsset::GetName() const
        {
            return m_name;
        }

        Data::Asset<ShaderVariantAsset> ShaderAsset::GetVariant(const ShaderVariantId& shaderVariantId)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);

            auto variantFinder = AZ::Interface<IShaderVariantFinder>::Get();
            AZ_Assert(variantFinder, "The IShaderVariantFinder doesn't exist");

            Data::Asset<ShaderVariantAsset> shaderVariantAsset = variantFinder->GetShaderVariantAssetByVariantId(
                Data::Asset<ShaderAsset>(this, Data::AssetLoadBehavior::Default), shaderVariantId);
            if (!shaderVariantAsset)
            {
                variantFinder->QueueLoadShaderVariantAssetByVariantId(
                    Data::Asset<ShaderAsset>(this, Data::AssetLoadBehavior::Default), shaderVariantId);
            }
            return shaderVariantAsset;
        }

        ShaderVariantSearchResult ShaderAsset::FindVariantStableId(const ShaderVariantId& shaderVariantId)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);

            uint32_t dynamicOptionCount = aznumeric_cast<uint32_t>(GetShaderOptionGroupLayout()->GetShaderOptions().size());
            ShaderVariantSearchResult variantSearchResult{RootShaderVariantStableId,  dynamicOptionCount };

            if (!dynamicOptionCount)
            {
                // The shader has no options at all. There's nothing to search.
                return variantSearchResult;
            }

            auto variantFinder = AZ::Interface<IShaderVariantFinder>::Get();
            AZ_Assert(variantFinder, "The IShaderVariantFinder doesn't exist");

            {
                AZStd::shared_lock<decltype(m_variantTreeMutex)> lock(m_variantTreeMutex);
                if (m_shaderVariantTree)
                {
                    return m_shaderVariantTree->FindVariantStableId(GetShaderOptionGroupLayout(), shaderVariantId);
                }
            }

            AZStd::unique_lock<decltype(m_variantTreeMutex)> lock(m_variantTreeMutex);
            if (!m_shaderVariantTree)
            {
                m_shaderVariantTree = variantFinder->GetShaderVariantTreeAsset(GetId());
                if (!m_shaderVariantTree)
                {
                    if (!m_shaderVariantTreeLoadWasRequested)
                    {
                        variantFinder->QueueLoadShaderVariantTreeAsset(GetId());
                        m_shaderVariantTreeLoadWasRequested = true;
                    }

                    // The variant tree could be under construction or simply doesn't exist at all.
                    return variantSearchResult;
                }
            }
            return m_shaderVariantTree->FindVariantStableId(GetShaderOptionGroupLayout(), shaderVariantId);
        }

        Data::Asset<ShaderVariantAsset> ShaderAsset::GetVariant(ShaderVariantStableId shaderVariantStableId) const
        {
            if (!shaderVariantStableId.IsValid() || shaderVariantStableId == RootShaderVariantStableId)
            {
                return GetRootVariant();
            }

            auto variantFinder = AZ::Interface<IShaderVariantFinder>::Get();
            AZ_Assert(variantFinder, "No Variant Finder For shaderAsset with name [%s] and stableId [%u]", GetName().GetCStr(), shaderVariantStableId.GetIndex());
            Data::Asset<ShaderVariantAsset> variant = variantFinder->GetShaderVariantAsset(m_shaderVariantTree.GetId(), shaderVariantStableId);
            if (!variant.IsReady())
            {
                // Enqueue a request to load the variant, next time around the caller will get the asset.
                Data::AssetId variantTreeAssetId;
                {
                    AZStd::shared_lock<decltype(m_variantTreeMutex)> lock(m_variantTreeMutex);
                    if (m_shaderVariantTree)
                    {
                        variantTreeAssetId = m_shaderVariantTree.GetId();
                    }
                }
                if (variantTreeAssetId.IsValid())
                {
                    variantFinder->QueueLoadShaderVariantAsset(variantTreeAssetId, shaderVariantStableId);
                }
                return GetRootVariant();
            }
            else if (variant->GetShaderAssetBuildTimestamp() == m_shaderAssetBuildTimestamp)
            {
                return variant;
            }
            else
            {
                // When rebuilding shaders we may be in a state where the ShaderAsset and root ShaderVariantAsset have been rebuilt and reloaded, but some (or all)
                // shader variants haven't been built yet. Since we want to use the latest version of the shader code, ignore the old variants and fall back to the newer root variant instead.
                AZ_Warning("ShaderAsset", false, "ShaderAsset and ShaderVariantAsset are out of sync; defaulting to root shader variant. (This is common while reloading shaders).");
                return GetRootVariant();
            }
        }

        Data::Asset<ShaderVariantAsset> ShaderAsset::GetRootVariant() const
        {
            return GetCurrentShaderApiData().m_rootShaderVariantAsset;
        }

        const Data::Asset<ShaderResourceGroupAsset>& ShaderAsset::FindShaderResourceGroupAsset(const Name& shaderResourceGroupName) const
        {
            const auto findIt = AZStd::find_if(m_shaderResourceGroupAssets.begin(), m_shaderResourceGroupAssets.end(), [&](const Data::Asset<ShaderResourceGroupAsset>& asset)
                {
                    return asset->GetName() == shaderResourceGroupName;
                });

            if (findIt != m_shaderResourceGroupAssets.end())
            {
                return *findIt;
            }

            static const Data::Asset<ShaderResourceGroupAsset> s_nullAsset;
            return s_nullAsset;
        }

        const Data::Asset<ShaderResourceGroupAsset>& ShaderAsset::FindShaderResourceGroupAsset(uint32_t bindingSlot) const
        {
            const auto findIt = AZStd::find_if(m_shaderResourceGroupAssets.begin(), m_shaderResourceGroupAssets.end(), [&](const Data::Asset<ShaderResourceGroupAsset>& asset)
                {
                    const auto layout = asset->GetLayout();
                    return layout && layout->GetBindingSlot() == bindingSlot;
                });

            if (findIt != m_shaderResourceGroupAssets.end())
            {
                return *findIt;
            }

            static const Data::Asset<ShaderResourceGroupAsset> s_nullAsset;
            return s_nullAsset;
        }

        const Data::Asset<ShaderResourceGroupAsset>& ShaderAsset::FindFallbackShaderResourceGroupAsset() const
        {
            const auto findIt = AZStd::find_if(m_shaderResourceGroupAssets.begin(), m_shaderResourceGroupAssets.end(), [&](const Data::Asset<ShaderResourceGroupAsset>& asset)
                {
                    const auto layout = asset->GetLayout();
                    return layout && layout->HasShaderVariantKeyFallbackEntry();
                });

            if (findIt != m_shaderResourceGroupAssets.end())
            {
                return *findIt;
            }

            static const Data::Asset<ShaderResourceGroupAsset> s_nullAsset;
            return s_nullAsset;
        }

        AZStd::array_view<Data::Asset<ShaderResourceGroupAsset>> ShaderAsset::GetShaderResourceGroupAssets() const
        {
            return m_shaderResourceGroupAssets;
        }

        RHI::PipelineStateType ShaderAsset::GetPipelineStateType() const
        {
            return m_pipelineStateType;
        }

        const RHI::PipelineLayoutDescriptor* ShaderAsset::GetPipelineLayoutDescriptor() const
        {
            AZ_Assert(GetCurrentShaderApiData().m_pipelineLayoutDescriptor, "m_pipelineLayoutDescriptor is null");
            return GetCurrentShaderApiData().m_pipelineLayoutDescriptor.get();
        }

        const ShaderOptionGroupLayout* ShaderAsset::GetShaderOptionGroupLayout() const
        {
            AZ_Assert(m_shaderOptionGroupLayout, "m_shaderOptionGroupLayout is null");
            return m_shaderOptionGroupLayout.get();
        }

        const AZ::Data::Asset<ShaderResourceGroupAsset>& ShaderAsset::GetDrawSrgAsset() const
        {
            return FindShaderResourceGroupAsset(SrgBindingSlot::Draw);
        }

        AZStd::optional<RHI::ShaderStageAttributeArguments> ShaderAsset::GetAttribute(const RHI::ShaderStage& shaderStage, const Name& attributeName) const
        {
            const auto stageIndex = static_cast<uint32_t>(shaderStage);
            AZ_Assert(stageIndex < RHI::ShaderStageCount, "Invalid shader stage specified!");

            const auto& attributeMaps = GetCurrentShaderApiData().m_attributeMaps;
            const auto& attrPair = attributeMaps[stageIndex].find(attributeName);
            if (attrPair == attributeMaps[stageIndex].end())
            {
                return AZStd::nullopt;
            }

            return attrPair->second;
        }

        const Name& ShaderAsset::GetDrawListName() const
        {
            return m_drawListName;
        }

        AZStd::sys_time_t ShaderAsset::GetShaderAssetBuildTimestamp() const
        {
            return m_shaderAssetBuildTimestamp;
        }

        void ShaderAsset::SetReady()
        {
            m_status = AssetStatus::Ready;
        }

        ShaderAsset::ShaderApiDataContainer& ShaderAsset::GetCurrentShaderApiData()
        {
            const size_t perApiShaderDataCount = m_perAPIShaderData.size();
            AZ_Assert(perApiShaderDataCount > 0, "Invalid m_perAPIShaderData");

            if (m_currentAPITypeIndex < perApiShaderDataCount)
            {
                return m_perAPIShaderData[m_currentAPITypeIndex];
            }

            // We may only endup here when running in a Builder context.
            return m_perAPIShaderData[0];
        }

        const ShaderAsset::ShaderApiDataContainer& ShaderAsset::GetCurrentShaderApiData() const
        {
            const size_t perApiShaderDataCount = m_perAPIShaderData.size();
            AZ_Assert(perApiShaderDataCount > 0, "Invalid m_perAPIShaderData");

            if (m_currentAPITypeIndex < perApiShaderDataCount)
            {
                return m_perAPIShaderData[m_currentAPITypeIndex];
            }

            // We may only endup here when running in a Builder context.
            return m_perAPIShaderData[0];
        }

        bool ShaderAsset::FinalizeAfterLoad()
        {
            // Use the current RHI that is active to select which shader data to use.
            // We don't assert if the Factory is not available because this method could be called during build time,
            // when no Factory is available. Some assets (like the material asset) need to load the ShaderAsset 
            // in order to get some non API specific data (like a ShaderResourceGroup) during their build
            // process. If they try to access any RHI API specific data, an assert will be trigger because the
            // correct API index will not set.
            if (RHI::Factory::IsReady())
            {
                auto rhiType = RHI::Factory::Get().GetType();
                auto findIt = AZStd::find_if(m_perAPIShaderData.begin(), m_perAPIShaderData.end(), [&rhiType](const auto& shaderData)
                    {
                        return shaderData.m_APIType == rhiType;
                    });

                if (findIt != m_perAPIShaderData.end())
                {
                    m_currentAPITypeIndex = AZStd::distance(m_perAPIShaderData.begin(), findIt);
                }
                else
                {
                    AZ_Error("ShaderAsset", false, "Could not find shader for API %s in shader %s", RHI::Factory::Get().GetName().GetCStr(), GetName().GetCStr());
                    return false;
                }
            }

            // Common finalize check
            for (const auto& shaderApiData : m_perAPIShaderData)
            {
                bool beTrue = shaderApiData.m_attributeMaps.size() == RHI::ShaderStageCount;
                if (!beTrue)
                {
                    AZ_Error("ShaderAsset", false, "Unexpected number of shader stages!");
                    return false;
                }
            }

            // Once the ShaderAsset is loaded, it is necessary to listen for changes in the Root Variant Asset.
            Data::AssetBus::Handler::BusConnect(GetRootVariant().GetId());
            ShaderVariantFinderNotificationBus::Handler::BusConnect(GetId());

            return true;
        }

        ///////////////////////////////////////////////////////////////////////
        // AssetBus overrides...
        void ShaderAsset::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
            ShaderReloadDebugTracker::ScopedSection reloadSection("ShaderAsset::OnAssetReloaded %s", asset.GetHint().c_str());

            Data::Asset<ShaderVariantAsset> shaderVariantAsset = { asset.GetAs<ShaderVariantAsset>(), AZ::Data::AssetLoadBehavior::PreLoad };
            AZ_Assert(shaderVariantAsset->GetStableId() == RootShaderVariantStableId,
                "Was expecting to update the root variant");
            GetCurrentShaderApiData().m_rootShaderVariantAsset = asset;

            ShaderReloadNotificationBus::Event(GetId(), &ShaderReloadNotificationBus::Events::OnShaderAssetReinitialized, Data::Asset<ShaderAsset>{ this, AZ::Data::AssetLoadBehavior::PreLoad } );
        }
        ///////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////
        /// ShaderVariantFinderNotificationBus overrides
        void ShaderAsset::OnShaderVariantTreeAssetReady(Data::Asset<ShaderVariantTreeAsset> shaderVariantTreeAsset, bool isError)
        {
            ShaderReloadDebugTracker::ScopedSection reloadSection("ShaderAsset::OnShaderVariantTreeAssetReady %s", shaderVariantTreeAsset.GetHint().c_str());

            AZStd::unique_lock<decltype(m_variantTreeMutex)> lock(m_variantTreeMutex);
            if (isError)
            {
                m_shaderVariantTree = {}; //This will force to attempt to reload later.
                m_shaderVariantTreeLoadWasRequested = false;
            }
            else
            {
                m_shaderVariantTree = shaderVariantTreeAsset;
            }
            lock.unlock();
            ShaderReloadNotificationBus::Event(GetId(), &ShaderReloadNotificationBus::Events::OnShaderAssetReinitialized, Data::Asset<ShaderAsset>{ this, AZ::Data::AssetLoadBehavior::PreLoad });
        }

        ///////////////////////////////////////////////////////////////////


        ///////////////////////////////////////////////////////////////////////
        // ShaderAssetHandler

        Data::AssetHandler::LoadResult ShaderAssetHandler::LoadAssetData(
            const Data::Asset<Data::AssetData>& asset,
            AZStd::shared_ptr<Data::AssetDataStream> stream,
            const Data::AssetFilterCB& assetLoadFilterCB)
        {
            if (Base::LoadAssetData(asset, stream, assetLoadFilterCB) == Data::AssetHandler::LoadResult::LoadComplete)
            {
                return PostLoadInit(asset);
            }
            return Data::AssetHandler::LoadResult::Error;
        }

        Data::AssetHandler::LoadResult ShaderAssetHandler::PostLoadInit(const Data::Asset<Data::AssetData>& asset)
        {
            if (ShaderAsset* shaderAsset = asset.GetAs<ShaderAsset>())
            {
                if (!shaderAsset->FinalizeAfterLoad())
                {
                    AZ_Error("ShaderAssetHandler", false, "Shader asset failed to finalize.");
                    return Data::AssetHandler::LoadResult::Error;
                }
                return Data::AssetHandler::LoadResult::LoadComplete;
            }
            return Data::AssetHandler::LoadResult::Error;
        }

        ///////////////////////////////////////////////////////////////////////

    } // namespace RPI
} // namespace AZ
