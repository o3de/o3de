/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Reflect/Shader/ShaderAsset2.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>

#include <Atom/RHI/Factory.h>

#include <AzCore/Interface/Interface.h>
#include <Atom/RPI.Reflect/Shader/IShaderVariantFinder2.h>
#include <Atom/RPI.Public/Shader/ShaderReloadDebugTracker.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus2.h>

namespace AZ
{
    namespace RPI
    {
        const ShaderVariantStableId ShaderAsset2::RootShaderVariantStableId{0};

        static constexpr uint32_t SubProductTypeBitPosition = 0;
        static constexpr uint32_t SubProductTypeNumBits = SupervariantIndexBitPosition - SubProductTypeBitPosition;
        static constexpr uint32_t SubProductTypeMaxValue = (1 << SubProductTypeNumBits) - 1;

        static_assert(RhiIndexMaxValue == RHI::Limits::APIType::PerPlatformApiUniqueIndexMax);

        uint32_t ShaderAsset2::MakeProductAssetSubId(
            uint32_t rhiApiUniqueIndex, uint32_t supervariantIndex, uint32_t subProductType)
        {
            AZ_Assert(rhiApiUniqueIndex <= RhiIndexMaxValue, "Invalid rhiApiUniqueIndex [%u]", rhiApiUniqueIndex);
            AZ_Assert(supervariantIndex <= SupervariantIndexMaxValue, "Invalid supervariantIndex [%u]", supervariantIndex);
            AZ_Assert(subProductType <= SubProductTypeMaxValue, "Invalid subProductType [%u]", subProductType);

            const uint32_t assetProductSubId = (rhiApiUniqueIndex << RhiIndexBitPosition) |
                (supervariantIndex << SupervariantIndexBitPosition) | (subProductType << SubProductTypeBitPosition);
            return assetProductSubId;
        }

        SupervariantIndex ShaderAsset2::GetSupervariantIndexFromProductAssetSubId(uint32_t assetProducSubId)
        {
            const uint32_t supervariantIndex = assetProducSubId >> SupervariantIndexBitPosition;
            return SupervariantIndex{supervariantIndex & SupervariantIndexMaxValue};
        }

        SupervariantIndex ShaderAsset2::GetSupervariantIndexFromAssetId(const Data::AssetId& assetId)
        {
            return GetSupervariantIndexFromProductAssetSubId(assetId.m_subId);
        }

        void ShaderAsset2::Supervariant::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<Supervariant>()
                    ->Version(1)
                    ->Field("Name", &Supervariant::m_name)
                    ->Field("SrgLayoutList", &Supervariant::m_srgLayoutList)
                    ->Field("PipelineLayout", &Supervariant::m_pipelineLayoutDescriptor)
                    ->Field("InputContract", &Supervariant::m_inputContract)
                    ->Field("OutputContract", &Supervariant::m_outputContract)
                    ->Field("RenderStates", &Supervariant::m_renderStates)
                    ->Field("AttributeMapList", &Supervariant::m_attributeMaps)
                    ->Field("RootVariantAsset", &Supervariant::m_rootShaderVariantAsset)
                    ;
            }
        }

        void ShaderAsset2::ShaderApiDataContainer::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderApiDataContainer>()
                    ->Version(1)
                    ->Field("APIType", &ShaderApiDataContainer::m_APIType)
                    ->Field("Supervariants", &ShaderApiDataContainer::m_supervariants)
                    ;
            }
        }

        void ShaderAsset2::Reflect(ReflectContext* context)
        {
            Supervariant::Reflect(context);

            ShaderApiDataContainer::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderAsset2>()
                    ->Version(1)
                    ->Field("name", &ShaderAsset2::m_name)
                    ->Field("pipelineStateType", &ShaderAsset2::m_pipelineStateType)
                    ->Field("shaderOptionGroupLayout", &ShaderAsset2::m_shaderOptionGroupLayout)
                    ->Field("drawListName", &ShaderAsset2::m_drawListName)
                    ->Field("shaderAssetBuildTimestamp", &ShaderAsset2::m_shaderAssetBuildTimestamp)
                    ->Field("perAPIShaderData", &ShaderAsset2::m_perAPIShaderData)
                    ;
            }
        }

        ShaderAsset2::~ShaderAsset2()
        {
            Data::AssetBus::Handler::BusDisconnect();
            ShaderVariantFinderNotificationBus2::Handler::BusDisconnect();
        }

        const Name& ShaderAsset2::GetName() const
        {
            return m_name;
        }

        RHI::PipelineStateType ShaderAsset2::GetPipelineStateType() const
        {
            return m_pipelineStateType;
        }

        const ShaderOptionGroupLayout* ShaderAsset2::GetShaderOptionGroupLayout() const
        {
            AZ_Assert(m_shaderOptionGroupLayout, "m_shaderOptionGroupLayout is null");
            return m_shaderOptionGroupLayout.get();
        }

        const Name& ShaderAsset2::GetDrawListName() const
        {
            return m_drawListName;
        }

        AZStd::sys_time_t ShaderAsset2::GetShaderAssetBuildTimestamp() const
        {
            return m_shaderAssetBuildTimestamp;
        }

        void ShaderAsset2::SetReady()
        {
            m_status = AssetStatus::Ready;
        }


        SupervariantIndex ShaderAsset2::GetSupervariantIndex(const AZ::Name& supervariantName) const
        {
            const auto& supervariants = GetCurrentShaderApiData().m_supervariants;
            const uint32_t supervariantCount = supervariants.size();
            for (uint32_t index = 0; index < supervariantCount; ++index)
            {
                if (supervariants[index].m_name == supervariantName)
                {
                    return SupervariantIndex{index};
                }
            }
            return InvalidSupervariantIndex;
        }


        Data::Asset<ShaderVariantAsset2> ShaderAsset2::GetVariant(
            const ShaderVariantId& shaderVariantId, SupervariantIndex supervariantIndex)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);

            auto variantFinder = AZ::Interface<IShaderVariantFinder2>::Get();
            AZ_Assert(variantFinder, "The IShaderVariantFinder doesn't exist");

            Data::Asset<ShaderAsset2> thisAsset(this, Data::AssetLoadBehavior::Default);
            Data::Asset<ShaderVariantAsset2> shaderVariantAsset =
                variantFinder->GetShaderVariantAssetByVariantId(thisAsset, shaderVariantId, supervariantIndex);
            if (!shaderVariantAsset)
            {
                variantFinder->QueueLoadShaderVariantAssetByVariantId(thisAsset, shaderVariantId, supervariantIndex);
            }
            return shaderVariantAsset;
        }

        ShaderVariantSearchResult ShaderAsset2::FindVariantStableId(const ShaderVariantId& shaderVariantId)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);

            uint32_t dynamicOptionCount = aznumeric_cast<uint32_t>(GetShaderOptionGroupLayout()->GetShaderOptions().size());
            ShaderVariantSearchResult variantSearchResult{RootShaderVariantStableId,  dynamicOptionCount };

            if (!dynamicOptionCount)
            {
                // The shader has no options at all. There's nothing to search.
                return variantSearchResult;
            }

            auto variantFinder = AZ::Interface<IShaderVariantFinder2>::Get();
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

        Data::Asset<ShaderVariantAsset2> ShaderAsset2::GetVariant(
            ShaderVariantStableId shaderVariantStableId, SupervariantIndex supervariantIndex) const
        {
            if (!shaderVariantStableId.IsValid() || shaderVariantStableId == RootShaderVariantStableId)
            {
                return GetRootVariant(supervariantIndex);
            }

            auto variantFinder = AZ::Interface<IShaderVariantFinder2>::Get();
            AZ_Assert(variantFinder, "No Variant Finder For shaderAsset with name [%s] and stableId [%u]", GetName().GetCStr(), shaderVariantStableId.GetIndex());
            Data::Asset<ShaderVariantAsset2> variant =
                variantFinder->GetShaderVariantAsset(m_shaderVariantTree.GetId(), shaderVariantStableId, supervariantIndex);
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
                    variantFinder->QueueLoadShaderVariantAsset(variantTreeAssetId, shaderVariantStableId, supervariantIndex);
                }
                return GetRootVariant(supervariantIndex);
            }
            else if (variant->GetBuildTimestamp() >= m_shaderAssetBuildTimestamp)
            {
                return variant;
            }
            else
            {
                // When rebuilding shaders we may be in a state where the ShaderAsset2 and root ShaderVariantAsset have been rebuilt and reloaded, but some (or all)
                // shader variants haven't been built yet. Since we want to use the latest version of the shader code, ignore the old variants and fall back to the newer root variant instead.
                AZ_Warning("ShaderAsset2", false, "ShaderAsset2 and ShaderVariantAsset are out of sync; defaulting to root shader variant. (This is common while reloading shaders).");
                return GetRootVariant(supervariantIndex);
            }
        }

        Data::Asset<ShaderVariantAsset2> ShaderAsset2::GetRootVariant(SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            if (!supervariant)
            {
                return Data::Asset<ShaderVariantAsset2>();
            }
            return supervariant->m_rootShaderVariantAsset;
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout> ShaderAsset2::FindShaderResourceGroupLayout(
            const Name& shaderResourceGroupName, SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            if (!supervariant)
            {
                return nullptr;
            }
            const auto& srgLayoutList = supervariant->m_srgLayoutList;
            const auto findIt = AZStd::find_if(srgLayoutList.begin(), srgLayoutList.end(), [&](const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
                {
                    return layout->GetName() == shaderResourceGroupName;
                });

            if (findIt != srgLayoutList.end())
            {
                return *findIt;
            }

            return nullptr;
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout> ShaderAsset2::FindShaderResourceGroupLayout(
            uint32_t bindingSlot, SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            if (!supervariant)
            {
                return nullptr;
            }
            const auto& srgLayoutList = supervariant->m_srgLayoutList;
            const auto findIt =
                AZStd::find_if(srgLayoutList.begin(), srgLayoutList.end(), [&](const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
                {
                    return layout && layout->GetBindingSlot() == bindingSlot;
                });

            if (findIt != srgLayoutList.end())
            {
                return *findIt;
            }

            return nullptr;
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout> ShaderAsset2::FindFallbackShaderResourceGroupLayout(
            SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            if (!supervariant)
            {
                return nullptr;
            }
            const auto& srgLayoutList = supervariant->m_srgLayoutList;
            const auto findIt =
                AZStd::find_if(srgLayoutList.begin(), srgLayoutList.end(), [&](const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
                {
                    return layout && layout->HasShaderVariantKeyFallbackEntry();
                });

            if (findIt != srgLayoutList.end())
            {
                return *findIt;
            }

            return nullptr;
        }

        AZStd::array_view<RHI::Ptr<RHI::ShaderResourceGroupLayout>> ShaderAsset2::GetShaderResourceGroupLayouts(
            SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            if (!supervariant)
            {
                return {};
            }
            return supervariant->m_srgLayoutList;
        }

        
        const RHI::Ptr<RHI::ShaderResourceGroupLayout> ShaderAsset2::GetDrawSrgLayout(SupervariantIndex supervariantIndex) const
        {
            return FindShaderResourceGroupLayout(SrgBindingSlot::Draw, supervariantIndex);
        }

        const ShaderInputContract& ShaderAsset2::GetInputContract(SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            return supervariant->m_inputContract;
        }

        const ShaderOutputContract& ShaderAsset2::GetOutputContract(SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            return supervariant->m_outputContract;
        }

        const RHI::RenderStates& ShaderAsset2::GetRenderStates(SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            return supervariant->m_renderStates;
        }

        const RHI::PipelineLayoutDescriptor* ShaderAsset2::GetPipelineLayoutDescriptor(SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            if (!supervariant)
            {
                return nullptr;
            }
            AZ_Assert(supervariant->m_pipelineLayoutDescriptor, "m_pipelineLayoutDescriptor is null");
            return supervariant->m_pipelineLayoutDescriptor.get();
        }

        AZStd::optional<RHI::ShaderStageAttributeArguments> ShaderAsset2::GetAttribute(const RHI::ShaderStage& shaderStage, const Name& attributeName,
            SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            if (!supervariant)
            {
                return AZStd::nullopt;
            }
            const auto stageIndex = static_cast<uint32_t>(shaderStage);
            AZ_Assert(stageIndex < RHI::ShaderStageCount, "Invalid shader stage specified!");

            const auto& attributeMaps = supervariant->m_attributeMaps;
            const auto& attrPair = attributeMaps[stageIndex].find(attributeName);
            if (attrPair == attributeMaps[stageIndex].end())
            {
                return AZStd::nullopt;
            }

            return attrPair->second;
        }

        ShaderAsset2::ShaderApiDataContainer& ShaderAsset2::GetCurrentShaderApiData()
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

        const ShaderAsset2::ShaderApiDataContainer& ShaderAsset2::GetCurrentShaderApiData() const
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

        ShaderAsset2::Supervariant* ShaderAsset2::GetSupervariant(SupervariantIndex supervariantIndex)
        {
            auto& supervariants = GetCurrentShaderApiData().m_supervariants;
            auto index = supervariantIndex.GetIndex();
            if (index >= supervariants.size())
            {
                AZ_Error(
                    "ShaderAsset2", false, "Supervariant index = %u is invalid because there are only %zu supervariants", index,
                    supervariants.size());
                return nullptr;
            }

            return &supervariants[index];
        }

        const ShaderAsset2::Supervariant* ShaderAsset2::GetSupervariant(SupervariantIndex supervariantIndex) const
        {
            const auto& supervariants = GetCurrentShaderApiData().m_supervariants;
            auto index = supervariantIndex.GetIndex();
            if (index >= supervariants.size())
            {
                AZ_Error(
                    "ShaderAsset2", false, "Supervariant index = %u is invalid because there are only %zu supervariants", index,
                    supervariants.size());
                return nullptr;
            }

            return &supervariants[index];
        }

        bool ShaderAsset2::FinalizeAfterLoad()
        {
            // Use the current RHI that is active to select which shader data to use.
            // We don't assert if the Factory is not available because this method could be called during build time,
            // when no Factory is available. Some assets (like the material asset) need to load the ShaderAsset2 
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
                    AZ_Error("ShaderAsset2", false, "Could not find shader for API %s in shader %s", RHI::Factory::Get().GetName().GetCStr(), GetName().GetCStr());
                    return false;
                }
            }

            // Common finalize check
            for (const auto& shaderApiData : m_perAPIShaderData)
            {
                const auto& supervariants = shaderApiData.m_supervariants;
                for (const auto& supervariant : supervariants)
                {
                    bool beTrue = supervariant.m_attributeMaps.size() == RHI::ShaderStageCount;
                    if (!beTrue)
                    {
                        AZ_Error("ShaderAsset2", false, "Unexpected number of shader stages at supervariant with name [%s]!", supervariant.m_name.GetCStr());
                        return false;
                    }
                }
            }

            // Once the ShaderAsset2 is loaded, it is necessary to listen for changes in the Root Variant Asset.
            Data::AssetBus::Handler::BusConnect(GetRootVariant().GetId());
            ShaderVariantFinderNotificationBus2::Handler::BusConnect(GetId());

            return true;
        }

        ///////////////////////////////////////////////////////////////////////
        // AssetBus overrides...
        void ShaderAsset2::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
            ShaderReloadDebugTracker::ScopedSection reloadSection("ShaderAsset2::OnAssetReloaded %s", asset.GetHint().c_str());

            Data::Asset<ShaderVariantAsset2> shaderVariantAsset = { asset.GetAs<ShaderVariantAsset2>(), AZ::Data::AssetLoadBehavior::PreLoad };
            AZ_Assert(shaderVariantAsset->GetStableId() == RootShaderVariantStableId,
                "Was expecting to update the root variant");
            SupervariantIndex supervariantIndex = GetSupervariantIndexFromAssetId(asset.GetId());
            GetCurrentShaderApiData().m_supervariants[supervariantIndex.GetIndex()].m_rootShaderVariantAsset = asset;

            ShaderReloadNotificationBus2::Event(GetId(), &ShaderReloadNotificationBus2::Events::OnShaderAssetReinitialized, Data::Asset<ShaderAsset2>{ this, AZ::Data::AssetLoadBehavior::PreLoad } );
        }
        ///////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////
        /// ShaderVariantFinderNotificationBus2 overrides
        void ShaderAsset2::OnShaderVariantTreeAssetReady(Data::Asset<ShaderVariantTreeAsset> shaderVariantTreeAsset, bool isError)
        {
            ShaderReloadDebugTracker::ScopedSection reloadSection("ShaderAsset2::OnShaderVariantTreeAssetReady %s", shaderVariantTreeAsset.GetHint().c_str());

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
            ShaderReloadNotificationBus2::Event(GetId(), &ShaderReloadNotificationBus2::Events::OnShaderAssetReinitialized, Data::Asset<ShaderAsset2>{ this, AZ::Data::AssetLoadBehavior::PreLoad });
        }

        ///////////////////////////////////////////////////////////////////


        ///////////////////////////////////////////////////////////////////////
        // ShaderAssetHandler

        Data::AssetHandler::LoadResult ShaderAssetHandler2::LoadAssetData(
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

        Data::AssetHandler::LoadResult ShaderAssetHandler2::PostLoadInit(const Data::Asset<Data::AssetData>& asset)
        {
            if (ShaderAsset2* shaderAsset = asset.GetAs<ShaderAsset2>())
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
