/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderCommonTypes.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>

#include <Atom/RHI/Factory.h>

#include <AzCore/Interface/Interface.h>
#include <Atom/RPI.Reflect/Shader/IShaderVariantFinder.h>
#include <Atom/RPI.Public/Shader/ShaderSystem.h>
#include <Atom/RPI.Public/Shader/ShaderReloadDebugTracker.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>

DECLARE_EBUS_INSTANTIATION_DLL_MULTI_ADDRESS(RPI::ShaderVariantFinderNotification);

namespace AZ
{
    namespace RPI
    {
        static constexpr uint32_t SubProductTypeBitPosition = 0;
        static constexpr uint32_t SubProductTypeNumBits = SupervariantIndexBitPosition - SubProductTypeBitPosition;
        [[maybe_unused]] static constexpr uint32_t SubProductTypeMaxValue = (1 << SubProductTypeNumBits) - 1;

        static_assert(RhiIndexMaxValue == RHI::Limits::APIType::PerPlatformApiUniqueIndexMax);

        uint32_t ShaderAsset::MakeProductAssetSubId(
            uint32_t rhiApiUniqueIndex, uint32_t supervariantIndex, uint32_t subProductType)
        {
            AZ_Assert(rhiApiUniqueIndex <= RhiIndexMaxValue, "Invalid rhiApiUniqueIndex [%u]", rhiApiUniqueIndex);
            AZ_Assert(supervariantIndex <= SupervariantIndexMaxValue, "Invalid supervariantIndex [%u]", supervariantIndex);
            AZ_Assert(subProductType <= SubProductTypeMaxValue, "Invalid subProductType [%u]", subProductType);

            const uint32_t assetProductSubId = (rhiApiUniqueIndex << RhiIndexBitPosition) |
                (supervariantIndex << SupervariantIndexBitPosition) | (subProductType << SubProductTypeBitPosition);
            return assetProductSubId;
        }

        SupervariantIndex ShaderAsset::GetSupervariantIndexFromProductAssetSubId(uint32_t assetProducSubId)
        {
            const uint32_t supervariantIndex = assetProducSubId >> SupervariantIndexBitPosition;
            return SupervariantIndex{supervariantIndex & SupervariantIndexMaxValue};
        }

        SupervariantIndex ShaderAsset::GetSupervariantIndexFromAssetId(const Data::AssetId& assetId)
        {
            return GetSupervariantIndexFromProductAssetSubId(assetId.m_subId);
        }

        void ShaderAsset::Supervariant::Reflect(AZ::ReflectContext* context)
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
                    ->Field("UseSpecializationConstants", &Supervariant::m_useSpecializationConstants)
                    ;
            }
        }

        void ShaderAsset::ShaderApiDataContainer::Reflect(AZ::ReflectContext* context)
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

        void ShaderAsset::Reflect(ReflectContext* context)
        {
            Supervariant::Reflect(context);

            ShaderApiDataContainer::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderAsset>()
                    ->Version(2)
                    ->Field("name", &ShaderAsset::m_name)
                    ->Field("pipelineStateType", &ShaderAsset::m_pipelineStateType)
                    ->Field("shaderOptionGroupLayout", &ShaderAsset::m_shaderOptionGroupLayout)
                    ->Field("defaultShaderOptionValueOverrides", &ShaderAsset::m_defaultShaderOptionValueOverrides)
                    ->Field("drawListName", &ShaderAsset::m_drawListName)
                    ->Field("perAPIShaderData", &ShaderAsset::m_perAPIShaderData)
                    ;
            }
        }

        ShaderAsset::~ShaderAsset()
        {
            ShaderVariantFinderNotificationBus::Handler::BusDisconnect();
            AssetInitBus::Handler::BusDisconnect();
        }

        const Name& ShaderAsset::GetName() const
        {
            return m_name;
        }

        RHI::PipelineStateType ShaderAsset::GetPipelineStateType() const
        {
            return m_pipelineStateType;
        }

        const ShaderOptionGroupLayout* ShaderAsset::GetShaderOptionGroupLayout() const
        {
            AZ_Assert(m_shaderOptionGroupLayout, "m_shaderOptionGroupLayout is null");
            return m_shaderOptionGroupLayout.get();
        }

        ShaderOptionGroup ShaderAsset::GetDefaultShaderOptions() const
        {
            // The m_shaderOptionGroupLayout has default values for each shader option, these come from shader source code.
            // The ShaderAsset can override these with its own default values, these come from the .shader file.
            ShaderOptionGroup shaderOptionGroup{m_shaderOptionGroupLayout, m_defaultShaderOptionValueOverrides};
            shaderOptionGroup.SetUnspecifiedToDefaultValues();
            return shaderOptionGroup;
        }

        const Name& ShaderAsset::GetDrawListName() const
        {
            return m_drawListName;
        }

        void ShaderAsset::SetReady()
        {
            m_status = AssetStatus::Ready;
        }

        SupervariantIndex ShaderAsset::GetSupervariantIndex(const AZ::Name& supervariantName) const
        {
            SupervariantIndex supervariantIndex = InvalidSupervariantIndex;

            // check for an RPI ShaderSystem supervariant
            RPI::ShaderSystemInterface* shaderSystemInterface = ShaderSystemInterface::Get();
            if (shaderSystemInterface && !shaderSystemInterface->GetSupervariantName().IsEmpty())
            {
                // search for the combined requested name and system supervariant name
                // Note: the shader may not support this supervariant, if it doesn't we will
                // fallback to the requested name below
                AZStd::string combinedName = supervariantName.GetCStr();
                combinedName.append(shaderSystemInterface->GetSupervariantName().GetCStr());
                supervariantIndex = GetSupervariantIndexInternal(AZ::Name(combinedName));
            }

            if (supervariantIndex == InvalidSupervariantIndex)
            {
                // search for the requested name
                supervariantIndex = GetSupervariantIndexInternal(supervariantName);
            }

            return supervariantIndex;
        }

        const AZ::Name& ShaderAsset::GetSupervariantName(SupervariantIndex supervariantIndex) const
        {
            const auto& supervariants = GetCurrentShaderApiData().m_supervariants;
            if (supervariantIndex.GetIndex() >= supervariants.size())
            {
                // Index 0 always exists, because the default supervariant always exists.
                return supervariants[0].m_name;
            }
            return supervariants[supervariantIndex.GetIndex()].m_name;
        }

        Data::Asset<ShaderVariantAsset> ShaderAsset::GetVariantAsset(
            const ShaderVariantId& shaderVariantId, SupervariantIndex supervariantIndex)
        {
            auto variantFinder = AZ::Interface<IShaderVariantFinder>::Get();
            AZ_Assert(variantFinder, "The IShaderVariantFinder doesn't exist");

            Data::Asset<ShaderAsset> thisAsset(this, Data::AssetLoadBehavior::Default);
            Data::Asset<ShaderVariantAsset> shaderVariantAsset =
                variantFinder->GetShaderVariantAssetByVariantId(thisAsset, shaderVariantId, supervariantIndex);
            if (!shaderVariantAsset && !IsFullySpecialized(supervariantIndex))
            {
                variantFinder->QueueLoadShaderVariantAssetByVariantId(thisAsset, shaderVariantId, supervariantIndex);
            }
            return shaderVariantAsset;
        }

        ShaderVariantSearchResult ShaderAsset::FindVariantStableId(const ShaderVariantId& shaderVariantId)
        {
            uint32_t dynamicOptionCount = aznumeric_cast<uint32_t>(GetShaderOptionGroupLayout()->GetShaderOptions().size());
            ShaderVariantSearchResult variantSearchResult{RootShaderVariantStableId,  dynamicOptionCount };

            if (!dynamicOptionCount || m_isFullySpecialized)
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

        Data::Asset<ShaderVariantAsset> ShaderAsset::GetVariantAsset(
            ShaderVariantStableId shaderVariantStableId, SupervariantIndex supervariantIndex) const
        {
            if (!shaderVariantStableId.IsValid() ||
                shaderVariantStableId == RootShaderVariantStableId ||
                IsFullySpecialized(supervariantIndex))
            {
                return GetRootVariantAsset(supervariantIndex);
            }

            auto variantFinder = AZ::Interface<IShaderVariantFinder>::Get();
            AZ_Assert(variantFinder, "No Variant Finder For shaderAsset with name [%s] and stableId [%u]", GetName().GetCStr(), shaderVariantStableId.GetIndex());
            Data::Asset<ShaderVariantAsset> variant =
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
                    variantFinder->QueueLoadShaderVariantAsset(variantTreeAssetId, shaderVariantStableId, GetSupervariantName(supervariantIndex));
                }
                return GetRootVariantAsset(supervariantIndex);
            }
            return variant;
        }

        Data::Asset<ShaderVariantAsset> ShaderAsset::GetRootVariantAsset(SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            if (!supervariant)
            {
                return Data::Asset<ShaderVariantAsset>();
            }
            return supervariant->m_rootShaderVariantAsset;
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& ShaderAsset::FindShaderResourceGroupLayout(
            const Name& shaderResourceGroupName, SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            if (!supervariant)
            {
                return RHI::NullSrgLayout;
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

            return RHI::NullSrgLayout;
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& ShaderAsset::FindShaderResourceGroupLayout(const Name& shaderResourceGroupName) const
        {
            SupervariantIndex supervariantIndex = DefaultSupervariantIndex;

            // check for an RPI ShaderSystem specified supervariant
            // Note: we can just search for the system supervariant name, since the default supervariant is "", so no need to append
            RPI::ShaderSystemInterface* shaderSystemInterface = ShaderSystemInterface::Get();
            if (shaderSystemInterface && !shaderSystemInterface->GetSupervariantName().IsEmpty())
            {
                SupervariantIndex systemSupervariantIndex = GetSupervariantIndexInternal(shaderSystemInterface->GetSupervariantName());
                if (systemSupervariantIndex.IsValid())
                {
                    supervariantIndex = systemSupervariantIndex;
                }
            }

            return FindShaderResourceGroupLayout(shaderResourceGroupName, supervariantIndex);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& ShaderAsset::FindShaderResourceGroupLayout(
            uint32_t bindingSlot, SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            if (!supervariant)
            {
                return RHI::NullSrgLayout;
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

            return RHI::NullSrgLayout;
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& ShaderAsset::FindShaderResourceGroupLayout(uint32_t bindingSlot) const
        {
            SupervariantIndex supervariantIndex = DefaultSupervariantIndex;

            // check for an RPI ShaderSystem specified supervariant
            // Note: we can just search for the system supervariant name, since the default supervariant is "", so no need to append
            RPI::ShaderSystemInterface* shaderSystemInterface = ShaderSystemInterface::Get();
            if (shaderSystemInterface && !shaderSystemInterface->GetSupervariantName().IsEmpty())
            {
                SupervariantIndex systemSupervariantIndex = GetSupervariantIndexInternal(shaderSystemInterface->GetSupervariantName());
                if (systemSupervariantIndex.IsValid())
                {
                    supervariantIndex = systemSupervariantIndex;
                }
            }

            return FindShaderResourceGroupLayout(bindingSlot, supervariantIndex);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& ShaderAsset::FindFallbackShaderResourceGroupLayout(
            SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            if (!supervariant)
            {
                return RHI::NullSrgLayout;
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

            return RHI::NullSrgLayout;
        }

        AZStd::span<const RHI::Ptr<RHI::ShaderResourceGroupLayout>> ShaderAsset::GetShaderResourceGroupLayouts(
            SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            if (!supervariant)
            {
                return {};
            }
            return supervariant->m_srgLayoutList;
        }

        
        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& ShaderAsset::GetDrawSrgLayout(SupervariantIndex supervariantIndex) const
        {
            return FindShaderResourceGroupLayout(SrgBindingSlot::Draw, supervariantIndex);
        }

        const ShaderInputContract& ShaderAsset::GetInputContract(SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            return supervariant->m_inputContract;
        }

        const ShaderOutputContract& ShaderAsset::GetOutputContract(SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            return supervariant->m_outputContract;
        }

        const RHI::RenderStates& ShaderAsset::GetRenderStates(SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            return supervariant->m_renderStates;
        }

        const RHI::PipelineLayoutDescriptor* ShaderAsset::GetPipelineLayoutDescriptor(SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            if (!supervariant)
            {
                return nullptr;
            }
            AZ_Assert(supervariant->m_pipelineLayoutDescriptor, "m_pipelineLayoutDescriptor is null");
            return supervariant->m_pipelineLayoutDescriptor.get();
        }

        AZStd::optional<RHI::ShaderStageAttributeArguments> ShaderAsset::GetAttribute(const RHI::ShaderStage& shaderStage, const Name& attributeName,
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

        bool ShaderAsset::UseSpecializationConstants(SupervariantIndex supervariantIndex) const
        {
            auto supervariant = GetSupervariant(supervariantIndex);
            if (!supervariant)
            {
                return false;
            }

            return supervariant->m_useSpecializationConstants;
        }

        bool ShaderAsset::IsFullySpecialized(SupervariantIndex supervariantIndex) const
        {            
            return UseSpecializationConstants(supervariantIndex) && m_shaderOptionGroupLayout->IsFullySpecialized();
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

        ShaderAsset::Supervariant* ShaderAsset::GetSupervariant(SupervariantIndex supervariantIndex)
        {
            auto& supervariants = GetCurrentShaderApiData().m_supervariants;
            auto index = supervariantIndex.GetIndex();
            if (index >= supervariants.size())
            {
                AZ_Error(
                    "ShaderAsset", false, "Supervariant index = %u is invalid because there are only %zu supervariants", index,
                    supervariants.size());
                return nullptr;
            }

            return &supervariants[index];
        }

        const ShaderAsset::Supervariant* ShaderAsset::GetSupervariant(SupervariantIndex supervariantIndex) const
        {
            const auto& supervariants = GetCurrentShaderApiData().m_supervariants;
            auto index = supervariantIndex.GetIndex();
            if (index >= supervariants.size())
            {
                AZ_Error(
                    "ShaderAsset", false, "Supervariant index = %u is invalid because there are only %zu supervariants", index,
                    supervariants.size());
                return nullptr;
            }

            return &supervariants[index];
        }

        SupervariantIndex ShaderAsset::GetSupervariantIndexInternal(AZ::Name supervariantName) const
        {
            const auto& supervariants = GetCurrentShaderApiData().m_supervariants;
            const uint32_t supervariantCount = static_cast<uint32_t>(supervariants.size());
            for (uint32_t index = 0; index < supervariantCount; ++index)
            {
                if (supervariants[index].m_name == supervariantName)
                {
                    return SupervariantIndex{ index };
                }
            }
            return InvalidSupervariantIndex;
        }

        bool ShaderAsset::SelectShaderApiData()
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

            m_isFullySpecialized = m_shaderOptionGroupLayout->IsFullySpecialized();
            // Common finalize check
            for (const auto& shaderApiData : m_perAPIShaderData)
            {
                const auto& supervariants = shaderApiData.m_supervariants;
                for (const auto& supervariant : supervariants)
                {
                    m_isFullySpecialized &= supervariant.m_useSpecializationConstants;
                    bool beTrue = supervariant.m_attributeMaps.size() == RHI::ShaderStageCount;
                    if (!beTrue)
                    {
                        AZ_Error("ShaderAsset", false, "Unexpected number of shader stages at supervariant with name [%s]!", supervariant.m_name.GetCStr());
                        return false;
                    }
                }
            }

            return true;
        }

        bool ShaderAsset::UpdateRootShaderVariantAsset(Data::Asset<ShaderVariantAsset> shaderVariantAsset)
        {
            auto& supervariants = GetCurrentShaderApiData().m_supervariants;
            for (auto& supervariant : supervariants)
            {
                if (supervariant.m_rootShaderVariantAsset.GetId() == shaderVariantAsset.GetId())
                {
                    supervariant.m_rootShaderVariantAsset = shaderVariantAsset;
                    return true;
                }
            }
            return false;
        }

        bool ShaderAsset::PostLoadInit()
        {
            ShaderVariantFinderNotificationBus::Handler::BusConnect(GetId());
            AssetInitBus::Handler::BusDisconnect();
            return true;
        }

        ///////////////////////////////////////////////////////////////////
        /// ShaderVariantFinderNotificationBus overrides
        void ShaderAsset::OnShaderVariantTreeAssetReady(Data::Asset<ShaderVariantTreeAsset> shaderVariantTreeAsset, bool isError)
        {
            ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->ShaderAsset::OnShaderVariantTreeAssetReady %s", this, shaderVariantTreeAsset.GetHint().c_str());

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
                ShaderAsset* shaderAsset = asset.GetAs<ShaderAsset>();

                // The shader API selection must occur immediately ofter loading, on the same thread, rather than
                // deferring to AssetInitBus::PostLoadInit. Many functions in the ShaderAsset class are invalid
                // until after SelectShaderApiData() is called and some client code may need to access data in
                // the ShaderAsset before then.
                if (!shaderAsset->SelectShaderApiData())
                {
                    return Data::AssetHandler::LoadResult::Error;
                }

                shaderAsset->AssetInitBus::Handler::BusConnect();

                return Data::AssetHandler::LoadResult::LoadComplete;
            }

            return Data::AssetHandler::LoadResult::Error;
        }
        
        ///////////////////////////////////////////////////////////////////////

    } // namespace RPI
} // namespace AZ
