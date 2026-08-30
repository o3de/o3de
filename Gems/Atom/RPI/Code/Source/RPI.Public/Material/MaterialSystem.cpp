/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Material/MaterialInstanceHandler.h>
#include <Atom/RPI.Public/Material/MaterialSystem.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/LuaMaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom_RPI_Traits_Platform.h>
#include <AzCore/Instance/InstanceDatabase.h>
#include <AzCore/Name/NameDictionary.h>

#ifndef AZ_TRAITS_SCENE_MATERIALS_MAX_SAMPLERS
#define AZ_TRAITS_SCENE_MATERIALS_MAX_SAMPLERS 0
#endif

// enable this if you want debug-prints whenever a material-Instance is registered
// #define DEBUG_MATERIALINSTANCES

namespace AZ::RPI
{
    void MaterialSystem::Reflect(AZ::ReflectContext* context)
    {
        MaterialPropertyValue::Reflect(context);
        MaterialTypeAsset::Reflect(context);
        MaterialAsset::Reflect(context);
        MaterialPropertiesLayout::Reflect(context);
        MaterialFunctor::Reflect(context);
        MaterialNameContext::Reflect(context);
        LuaMaterialFunctor::Reflect(context);
        ReflectMaterialDynamicMetadata(context);
    }

    void MaterialSystem::GetAssetHandlers(AssetHandlerPtrList& assetHandlers)
    {
        assetHandlers.emplace_back(MakeAssetHandler<MaterialTypeAssetHandler>());
        assetHandlers.emplace_back(MakeAssetHandler<MaterialAssetHandler>());
    }

    bool MaterialSystem::LoadMaterialSrgShaderAsset()
    {
        if (!m_sceneMaterialSrgShaderAsset)
        {
            // Load the dummy shader containing the SceneMaterialSrg
            const AZStd::string materialSrgShader = "shaders/scenematerialsrg.azshader";
            m_sceneMaterialSrgShaderAsset =
                AssetUtils::LoadCriticalAsset<RPI::ShaderAsset>(materialSrgShader.data(), RPI::AssetUtils::TraceLevel::Warning);
        }
        if (!m_sceneMaterialSrgShaderAsset)
        {
            AZ_Warning("MaterialSystem", false, "Unable to locate the Material SRG shader asset, try again");
            return false;
        }

        CreateSceneMaterialSrg();

        AZ::Data::AssetBus::Handler::BusConnect(m_sceneMaterialSrgShaderAsset.GetId());
        return true;
    }

    void MaterialSystem::CreateSceneMaterialSrg()
    {
        if (m_sceneMaterialSrgShaderAsset->IsReady())
        {
            m_sceneMaterialSrg = ShaderResourceGroup::Create(m_sceneMaterialSrgShaderAsset, AZ_NAME_LITERAL("SceneMaterialSrg"));

            // get the size of the m_samplers[] array from the SRG layout
            auto samplerIndex = m_sceneMaterialSrg->GetLayout()->FindShaderInputSamplerIndex(AZ_NAME_LITERAL("m_samplers"));
            if (samplerIndex.IsValid())
            {
                auto desc = m_sceneMaterialSrg->GetLayout()->GetShaderInput(samplerIndex);
                [[maybe_unused]] uint32_t maxTextureSamplerStates = desc.m_count;
                AZ_Assert(
                    maxTextureSamplerStates >= m_sceneTextureSamplers.GetMaxNumSamplerStates(),
                    "SceneMaterialSrg::m_samplers[] has size %d, expected size is AZ_TRAITS_SCENE_MATERIALS_MAX_SAMPLERS (%d)",
                    maxTextureSamplerStates,
                    AZ_TRAITS_SCENE_MATERIALS_MAX_SAMPLERS);
                m_sharedSamplerStatesDirty = true;
            }
        }
    }

    //  Data::AssetBus Interface
    void MaterialSystem::OnAssetReloaded([[maybe_unused]] AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        CreateSceneMaterialSrg();
    }

    void MaterialSystem::OnAssetReady([[maybe_unused]] AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        CreateSceneMaterialSrg();
    }

    int32_t MaterialSystem::RegisterMaterialTexture(
        [[maybe_unused]] const int materialTypeIndex,
        [[maybe_unused]] const int materialInstanceIndex,
        [[maybe_unused]] Data::Instance<Image> image)
    {
        int32_t textureIndex{ -1 };
#ifdef AZ_TRAIT_REGISTER_TEXTURES_PER_MATERIAL
        if (!image)
        {
            return textureIndex;
        }

        auto& materialTypeData = m_materialTypeData[materialTypeIndex];
        auto& instanceData = materialTypeData.m_instanceData[materialInstanceIndex];
        if (instanceData.m_materialTextureRegistry)
        {
            textureIndex = instanceData.m_materialTextureRegistry->RegisterMaterialTexture(image);
            // we only need to update the material-textures if we actually register a new texture
            instanceData.m_materialTexturesDirty = true;
        }
#endif
        return textureIndex;
    }

    void MaterialSystem::ReleaseMaterialTexture(
        [[maybe_unused]] const int materialTypeIndex,
        [[maybe_unused]] const int materialInstanceIndex,
        [[maybe_unused]] int32_t textureIndex)
    {
#ifdef AZ_TRAIT_REGISTER_TEXTURES_PER_MATERIAL
        auto& materialTypeData = m_materialTypeData[materialTypeIndex];
        auto& instanceData = materialTypeData.m_instanceData[materialInstanceIndex];
        if (instanceData.m_materialTextureRegistry)
        {
            instanceData.m_materialTextureRegistry->ReleaseMaterialTexture(textureIndex);
        }
#endif
    }

    AZStd::shared_ptr<SharedSamplerState> MaterialSystem::RegisterTextureSampler(
        const int materialTypeIndex, const int materialInstanceIndex, const RHI::SamplerState& samplerState)
    {
        AZStd::shared_ptr<SharedSamplerState> sharedSampler = nullptr;
        auto& materialTypeData = m_materialTypeData[materialTypeIndex];

        TextureSamplerRegistry* registry;
        if (materialTypeData.m_useSceneMaterialSrg)
        {
            registry = &m_sceneTextureSamplers;
        }
        else
        {
            auto& materialInstanceData = materialTypeData.m_instanceData[materialInstanceIndex];
            registry = materialInstanceData.m_textureSamplers.get();
        }

        auto [sharedSamplerState, registered] = registry->RegisterTextureSampler(samplerState);
        if (materialTypeData.m_useSceneMaterialSrg && registered)
        {
            m_sharedSamplerStatesDirty = true;
        }
        return sharedSamplerState;
    }

    const RHI::SamplerState MaterialSystem::GetRegisteredTextureSampler(
        const int materialTypeIndex, const int materialInstanceIndex, const uint32_t samplerIndex)
    {
        auto& materialTypeData = m_materialTypeData[materialTypeIndex];
        TextureSamplerRegistry* registry;
        if (materialTypeData.m_useSceneMaterialSrg)
        {
            registry = &m_sceneTextureSamplers;
        }
        else
        {
            auto& materialInstanceData = materialTypeData.m_instanceData[materialInstanceIndex];
            registry = materialInstanceData.m_textureSamplers.get();
        }
        auto sharedSamplerState = registry->GetSharedSamplerState(samplerIndex);
        if (sharedSamplerState == nullptr)
        {
            return RHI::SamplerState{};
        }
        return sharedSamplerState->m_samplerState;
    }

    // MaterialSrgHandler Interface
    MaterialInstanceData MaterialSystem::RegisterMaterialInstance(const Data::Instance<Material> material)
    {
        if (!m_sceneMaterialSrgShaderAsset)
        {
            LoadMaterialSrgShaderAsset();
        }
        m_bufferReadIndicesDirty = true;

        int32_t materialTypeIndex{ -1 };

        // Two parameter layouts describe the same buffer only if every descriptor agrees on name, GPU type and where it sits. Comparing
        // sizes alone is not enough: renaming a material property reorders the descriptors without changing how many bytes they occupy.
        const auto layoutsDescribeTheSameBuffer =
            [](const MaterialShaderParameterLayout& lhs, const MaterialShaderParameterLayout& rhs)
        {
            const auto lhsDescriptors = lhs.GetDescriptors();
            const auto rhsDescriptors = rhs.GetDescriptors();
            if (lhsDescriptors.size() != rhsDescriptors.size())
            {
                return false;
            }

            for (size_t descriptorIndex = 0; descriptorIndex < lhsDescriptors.size(); ++descriptorIndex)
            {
                const auto& lhsDescriptor = lhsDescriptors[descriptorIndex];
                const auto& rhsDescriptor = rhsDescriptors[descriptorIndex];
                if (lhsDescriptor.m_name != rhsDescriptor.m_name || lhsDescriptor.m_typeName != rhsDescriptor.m_typeName ||
                    lhsDescriptor.m_structuredBufferBinding.m_offset != rhsDescriptor.m_structuredBufferBinding.m_offset ||
                    lhsDescriptor.m_structuredBufferBinding.m_elementSize != rhsDescriptor.m_structuredBufferBinding.m_elementSize ||
                    lhsDescriptor.m_structuredBufferBinding.m_elementCount != rhsDescriptor.m_structuredBufferBinding.m_elementCount)
                {
                    return false;
                }
            }
            return true;
        };

        auto materialAsset = material->GetAsset();
        auto materialTypeAsset = materialAsset->GetMaterialTypeAsset();

        // Note: We store the Material-Parameters in a single SRG, but each object gets it's own draw-item, which holds the shader
        // options, so we don't need to consider them. However, for raytracing or deferred approaches, where one shader shades multiple
        // material-instances, we probably want to different Material-IDs for different Shader Options
        auto materialTypeAssetIterator = m_materialTypeIndicesMap.find(materialTypeAsset.GetId());
        if (materialTypeAssetIterator == m_materialTypeIndicesMap.end())
        {
            materialTypeIndex = m_materialTypeIndices.Aquire();
            m_materialTypeData.resize(m_materialTypeIndices.MaxCount());
            m_materialTypeIndicesMap.insert(AZStd::make_pair(materialTypeAsset.GetId(), materialTypeIndex));
            MaterialTypeData& materialTypeData = m_materialTypeData[materialTypeIndex];

            materialTypeData.m_materialTypeAssetId = materialTypeAsset->GetId();
            materialTypeData.m_materialTypeAssetHint = materialTypeAsset.GetHint();
            materialTypeData.m_valid = true;
            // make sure we hold on to the MaterialShaderParameterLayout somewhere that survives a hot reload
            materialTypeData.m_shaderParameterLayout =
                AZStd::make_unique<MaterialShaderParameterLayout>(materialTypeAsset->GetMaterialShaderParameterLayout());

            auto srgLayout = materialTypeAsset->GetMaterialSrgLayout();

            if (srgLayout)
            {
                if (m_sceneMaterialSrg && m_sceneMaterialSrg->GetLayout()->GetHash() == srgLayout->GetHash())
                {
                    materialTypeData.m_useSceneMaterialSrg = true;
                }
            }
        }
        else
        {
            materialTypeIndex = materialTypeAssetIterator->second;

            // The entry is keyed by material type asset ID, and an asset keeps its ID across a hot reload. The layout captured when the
            // ID was first seen was therefore kept forever, so after any rebuild that changed the parameter layout every instance --
            // including ones created long afterwards -- addressed the buffer with offsets the reloaded shader had stopped using. Renaming
            // a material property in Material Canvas does exactly that: the descriptors are reordered while the total size is unchanged,
            // so values land in the wrong members and the material renders with a texture stretched by someone else's UV scale, or black,
            // for the rest of the process.
            MaterialTypeData& existingMaterialTypeData = m_materialTypeData[materialTypeIndex];
            const auto& reloadedLayout = materialTypeAsset->GetMaterialShaderParameterLayout();
            if (existingMaterialTypeData.m_shaderParameterLayout &&
                !layoutsDescribeTheSameBuffer(*existingMaterialTypeData.m_shaderParameterLayout, reloadedLayout))
            {
                existingMaterialTypeData.m_shaderParameterLayout = AZStd::make_unique<MaterialShaderParameterLayout>(reloadedLayout);
                existingMaterialTypeData.m_layoutGeneration++;
            }
        }
        MaterialTypeData& materialTypeData = m_materialTypeData[materialTypeIndex];

        auto materialInstanceIndex = materialTypeData.m_instanceIndices.Aquire();
        materialTypeData.m_instanceData.resize(materialTypeData.m_instanceIndices.MaxCount());
        auto& instanceData = materialTypeData.m_instanceData[materialInstanceIndex];

        instanceData.m_material = material.get();
        instanceData.m_compiledChangeId = Material::DEFAULT_CHANGE_ID;
        instanceData.m_layoutGeneration = materialTypeData.m_layoutGeneration;

        if (!materialTypeData.m_useSceneMaterialSrg)
        {
            auto srgLayout = materialTypeAsset->GetMaterialSrgLayout();
            if (srgLayout)
            {
                auto srgShaderAsset = materialTypeAsset->GetShaderAssetForMaterialSrg();
                instanceData.m_shaderResourceGroup = ShaderResourceGroup::Create(srgShaderAsset, srgLayout->GetName());

                // get the size of the m_samplers[] array from the SRG layout
                auto samplerIndex = instanceData.m_shaderResourceGroup->GetLayout()->FindShaderInputSamplerIndex(AZ::Name{ "m_samplers" });
                if (samplerIndex.IsValid())
                {
                    auto desc = instanceData.m_shaderResourceGroup->GetLayout()->GetShaderInput(samplerIndex);
                    auto defaultSampler =
                        RHI::SamplerState::Create(RHI::FilterMode::Linear, RHI::FilterMode::Linear, RHI::AddressMode::Wrap);
                    defaultSampler.m_anisotropyMax = 16;
                    defaultSampler.m_anisotropyEnable = true;
                    instanceData.m_textureSamplers = AZStd::make_unique<TextureSamplerRegistry>();
                    instanceData.m_textureSamplers->Init(desc.m_count, defaultSampler);
                }

#ifdef AZ_TRAIT_REGISTER_TEXTURES_PER_MATERIAL
                // get the size of the m_samplers[] array from the SRG layout
                auto materialTexturesIndex =
                    instanceData.m_shaderResourceGroup->GetLayout()->FindShaderInputImageIndex(AZ::Name{ "m_textures" });
                if (materialTexturesIndex.IsValid() && m_nullTexture)
                {
                    auto desc = instanceData.m_shaderResourceGroup->GetLayout()->GetShaderInput(materialTexturesIndex);
                    instanceData.m_materialTextureRegistry = AZStd::make_unique<MaterialTextureRegistry>();
                    instanceData.m_materialTextureRegistry->Init(desc.m_count, m_nullTexture);
                }
#endif
            }
        }
        else
        {
            instanceData.m_shaderResourceGroup = m_sceneMaterialSrg;
        }
        if (instanceData.m_shaderResourceGroup)
        {
            instanceData.m_shaderParameter = aznew MaterialShaderParameter(
                materialTypeIndex,
                materialInstanceIndex,
                materialTypeData.m_shaderParameterLayout.get(),
                instanceData.m_shaderResourceGroup);
        }
        else
        {
            // the material has no SRG at all, and also no shader parameters
            instanceData.m_shaderParameter = {};
        }

        MaterialInstanceData result{ materialTypeIndex,
                                     materialInstanceIndex,
                                     materialTypeData.m_useSceneMaterialSrg,
                                     instanceData.m_shaderResourceGroup,
                                     instanceData.m_shaderParameter };

#ifdef DEBUG_MATERIALINSTANCES
        AZ_Printf(
            "MaterialSystem",
            "RegisterMaterialInstance: Register Type %d (%s), Instance %d (%s) (max: %d)",
            materialTypeIndex,
            materialTypeData.m_materialTypeAssetHint.c_str(),
            materialInstanceIndex,
            instanceData.m_material->GetAsset().GetHint().c_str(),
            materialTypeData.m_instanceIndices.MaxCount());
#endif

        return result;
    }

    void MaterialSystem::ReleaseMaterialInstance(const MaterialInstanceData& materialInstance)
    {
        m_bufferReadIndicesDirty = true;

        MaterialTypeData* materialTypeData = &m_materialTypeData[materialInstance.m_materialTypeId];
#ifdef DEBUG_MATERIALINSTANCES
        InternalMaterialInstanceData* materialInstanceData = &materialTypeData->m_instanceData[materialInstance.m_materialInstanceId];

        AZ_Printf(
            "MaterialSystem",
            "ReleaseMaterialInstance: Release Type %d(%s), Instance %d (%s) (max: %d)",
            materialInstance.m_materialTypeId,
            materialTypeData->m_materialTypeAssetHint.c_str(),
            materialInstance.m_materialInstanceId,
            materialInstanceData->m_material->GetAsset().GetHint().c_str(),
            materialTypeData->m_instanceIndices.MaxCount());
#endif

        materialTypeData->m_instanceData[materialInstance.m_materialInstanceId] = {};
        materialTypeData->m_instanceIndices.Release(materialInstance.m_materialInstanceId);
        if (materialTypeData->m_instanceIndices.IsFullyReleased()) // no more instances of this type
        {
            m_materialTypeIndices.Release(materialInstance.m_materialTypeId);
            m_materialTypeIndicesMap.erase(materialTypeData->m_materialTypeAssetId);
            m_materialTypeData[materialInstance.m_materialTypeId] = {};
            m_materialTypeData[materialInstance.m_materialTypeId].m_valid = false;
            materialTypeData = nullptr;
        }
        if (m_materialTypeIndices.IsFullyReleased()) // no more types in general
        {
            m_materialTypeData.clear();
            m_materialTypeIndices.Reset();
            m_materialTypeIndicesMap.clear();
        }
    }

    void MaterialSystem::DebugPrintMaterialInstances()
    {
#ifdef DEBUG_MATERIALINSTANCES
        auto readIndices = [](const AZStd::unordered_map<int, uint32_t>& indices)
        {
            AZStd::string result;
            if (indices.empty())
            {
                return result;
            }
            const auto deviceCount{ AZ::RHI::RHISystemInterface::Get()->GetDeviceCount() };
            result.reserve(20ull * deviceCount);
            for (auto deviceIndex{ 0 }; deviceIndex < deviceCount; ++deviceIndex)
            {
                if (!result.empty())
                {
                    result += ", ";
                }
                result += AZStd::string::format("device %d: %d", deviceIndex, static_cast<int>(indices.at(deviceIndex)));
            }
            return result;
        };

        for (int materialTypeIndex = 0; materialTypeIndex < m_materialTypeData.size(); materialTypeIndex++)
        {
            auto& materialTypeEntry = m_materialTypeData[materialTypeIndex];
            // The material-Type-Indices and instance-indices stay constant during their lifetime, which means we can get holes in
            // this buffer
            if (!materialTypeEntry.m_valid)
            {
                AZ_Printf("MaterialSystem", " [%d] MaterialType Empty", materialTypeIndex);
                continue;
            }

            AZ_Printf(
                "MaterialSystem",
                "[%d] MaterialType %s, %s, device bindless read indices = [%s]",
                materialTypeIndex,
                materialTypeEntry.m_materialTypeAssetHint.c_str(),
                materialTypeEntry.m_useSceneMaterialSrg ? "uses SceneMaterialSrg" : "uses custom MaterialSrg",
                readIndices(materialTypeEntry.m_bindlessReadIndices).c_str());

            for (int instanceIndex = 0; instanceIndex < materialTypeEntry.m_instanceData.size(); instanceIndex++)
            {
                auto& materialInstanceEntry = materialTypeEntry.m_instanceData[instanceIndex];
                if (!materialInstanceEntry.m_material)
                {
                    AZ_Printf("MaterialSystem", "    [%d] Instance Empty", instanceIndex);
                    continue;
                }
                if (materialInstanceEntry.m_shaderParameter)
                {
                    AZ_Printf(
                        "MaterialSystem",
                        "    [%d] Instance %s (Offset %d, size %d)",
                        instanceIndex,
                        materialInstanceEntry.m_material->GetAsset().GetHint().c_str(),
                        materialInstanceEntry.m_shaderParameter->GetStructuredBufferDataSize() * instanceIndex,
                        materialInstanceEntry.m_shaderParameter->GetStructuredBufferDataSize());
                }
                else
                {
                    AZ_Printf(
                        "MaterialSystem",
                        "    [%d] Instance %s (no parameters)",
                        instanceIndex,
                        materialInstanceEntry.m_material->GetAsset().GetHint().c_str());
                }
            }
        }
#endif
    }

    void MaterialSystem::UpdateChangedMaterialParameters()
    {
        for (auto& materialTypeEntry : m_materialTypeData)
        {
            if (!materialTypeEntry.m_valid)
            {
                continue;
            }
            size_t shaderParamsSize = 0;
            for (auto& instanceData : materialTypeEntry.m_instanceData)
            {
                if (instanceData.m_material && instanceData.m_shaderParameter)
                {
                    shaderParamsSize = instanceData.m_shaderParameter->GetStructuredBufferDataSize();
                    break;
                }
            }
            if (materialTypeEntry.m_useSceneMaterialSrg)
            {
                AZ_Assert(shaderParamsSize > 0, "MaterialSystem: Material uses SceneMaterialSrg, but has no Shader Parameters");
            }
            for (int32_t instanceIndex = 0; instanceIndex < materialTypeEntry.m_instanceIndices.MaxCount(); instanceIndex++)
            {
                auto& instanceData = materialTypeEntry.m_instanceData[instanceIndex];
                if (instanceData.m_material && instanceData.m_material->GetCurrentChangeId() != instanceData.m_compiledChangeId)
                {
                    if (materialTypeEntry.m_useSceneMaterialSrg)
                    {
                        // The parameter buffer is addressed as a flat array of identically sized structs: the offset below is
                        // instanceIndex * shaderParamsSize, and shaderParamsSize was sampled from whichever instance happened to be
                        // found first. That holds only while every live instance of this material type shares one parameter layout.
                        //
                        // It stops holding when a material type is rebuilt with a different layout while instances of the previous one
                        // are still registered. Material Canvas does exactly that on every structural edit: adding a material input
                        // node adds a property, the generated MaterialParameters struct grows, and the viewport is still holding the
                        // unique instances it created before the rebuild. Copying shaderParamsSize bytes out of an instance whose own
                        // struct is smaller reads off the end of it, which surfaces as an access violation inside Buffer::UpdateData
                        // with no indication of where it came from.
                        //
                        // Skipping the mismatched instance leaves it showing stale parameters until whatever owns it recreates it,
                        // which is recoverable and self correcting. The change ID is still advanced so this reports once per material
                        // change rather than once per frame, since an instance built against the old layout will never match again.
                        if (!instanceData.m_shaderParameter)
                        {
                            instanceData.m_compiledChangeId = instanceData.m_material->GetCurrentChangeId();
                            continue;
                        }

                        // Built against a layout this material type no longer uses. Its MaterialShaderParameter still holds the old
                        // offsets, so writing it would scatter values across the wrong members rather than merely being stale. Whoever
                        // owns the instance replaces it shortly after a reload, and the replacement carries the current generation.
                        if (instanceData.m_layoutGeneration != materialTypeEntry.m_layoutGeneration)
                        {
                            // Identified by the material type, not by the instance. Reaching through m_material for its asset hint is
                            // what the neighbouring Register and Release logs do, and it is safe there because the material is known
                            // good at those points. It is not safe here: this branch exists precisely because the instance disagrees
                            // with the material type about the parameter layout, and the reason it disagrees is that its material type
                            // asset is being reloaded underneath it. Formatting a string read out of an asset reference that is mid
                            // swap turned a recoverable, self correcting condition into an access violation inside vsnprintf.
                            //
                            // m_materialTypeAssetHint is a plain string owned by the material type entry, names the same thing for
                            // diagnostic purposes, and is not touched by a reload in flight.
                            AZ_Warning(
                                "MaterialSystem",
                                false,
                                "Material instance %d of material type '%s' was created against parameter layout generation %u, but "
                                "this material type is now on generation %u. Its update is being skipped until the instance is "
                                "recreated.",
                                instanceIndex,
                                materialTypeEntry.m_materialTypeAssetHint.c_str(),
                                instanceData.m_layoutGeneration,
                                materialTypeEntry.m_layoutGeneration);
                            instanceData.m_compiledChangeId = instanceData.m_material->GetCurrentChangeId();
                            continue;
                        }

                        const size_t instanceParamsSize = instanceData.m_shaderParameter->GetStructuredBufferDataSize();
                        if (instanceParamsSize != shaderParamsSize)
                        {
                            AZ_Warning(
                                "MaterialSystem",
                                false,
                                "Material instance %d of material type '%s' has a %zu byte parameter struct where this material type's "
                                "buffer is strided for %zu. Its update is being skipped: it was created against an earlier version of "
                                "the material type and will correct itself once the instance is recreated.",
                                instanceIndex,
                                materialTypeEntry.m_materialTypeAssetHint.c_str(),
                                instanceParamsSize,
                                shaderParamsSize);
                            instanceData.m_compiledChangeId = instanceData.m_material->GetCurrentChangeId();
                            continue;
                        }

                        auto shaderParamsData = instanceData.m_shaderParameter->GetStructuredBufferData();
                        materialTypeEntry.m_parameterBuffer->UpdateData(
                            shaderParamsData, shaderParamsSize, instanceIndex * shaderParamsSize);
                        instanceData.m_compiledChangeId = instanceData.m_material->GetCurrentChangeId();
                        // we are only changing the data of a buffer registered in the SceneMaterialSrg, no need to compile it
                    }
                    else if (instanceData.m_shaderResourceGroup)
                    {
                        // The material doesn't use the SceneMaterialSrg: make sure the custom SRG still gets compiled

#ifdef AZ_TRAIT_REGISTER_TEXTURES_PER_MATERIAL
                        if (instanceData.m_materialTexturesDirty && instanceData.m_materialTextureRegistry)
                        {
                            auto texturesIndex = instanceData.m_shaderResourceGroup->FindShaderInputImageIndex(AZ::Name{ "m_textures" });
                            if (texturesIndex.IsValid())
                            {
                                AZStd::vector<const RHI::ImageView*> imageViews =
                                    instanceData.m_materialTextureRegistry->CollectTextureViews();
                                instanceData.m_shaderResourceGroup->SetImageViewArray(texturesIndex, imageViews);
                            }
                            instanceData.m_materialTexturesDirty = false;
                        }
#endif
                        // register the sampler array if the material requires it
                        auto nullTextureIndex =
                            instanceData.m_shaderResourceGroup->FindShaderInputConstantIndex(AZ::Name{ "m_nullTextureIndex" });
                        if (nullTextureIndex.IsValid() && m_nullTexture)
                        {
#ifdef AZ_TRAIT_REGISTER_TEXTURES_PER_MATERIAL
                            instanceData.m_shaderResourceGroup->SetConstant(
                                nullTextureIndex, instanceData.m_materialTextureRegistry->GetNullTextureIndex());
#else
                            instanceData.m_shaderResourceGroup->SetConstant(
                                nullTextureIndex, m_nullTexture->GetImageView()->GetBindlessReadIndex());
#endif
                        }

                        // register the sampler array if the material requires it
                        auto samplerIndex = instanceData.m_shaderResourceGroup->FindShaderInputSamplerIndex(AZ::Name{ "m_samplers" });
                        if (samplerIndex.IsValid() && instanceData.m_textureSamplers)
                        {
                            auto samplerStates = instanceData.m_textureSamplers->CollectSamplerStates();
                            instanceData.m_shaderResourceGroup->SetSamplerArray(
                                samplerIndex, { samplerStates.begin(), samplerStates.end() });
                        }

                        instanceData.m_shaderResourceGroup->Compile();
                        instanceData.m_compiledChangeId = instanceData.m_material->GetCurrentChangeId();
                    }
                }
            }
        }
    }

    void MaterialSystem::PrepareMaterialParameterBuffers()
    {
        auto createMaterialParameterBuffer = [](const int materialTypeIndex, const size_t elementSize, const size_t numElements)
        {
            AZ::RPI::CommonBufferDescriptor desc;
            desc.m_elementFormat = AZ::RHI::Format::Unknown;
            desc.m_poolType = AZ::RPI::CommonBufferPoolType::ReadOnly;
            desc.m_elementSize = static_cast<uint32_t>(elementSize);
            desc.m_bufferName = AZStd::string::format("MaterialParameterBuffer_%d", materialTypeIndex);
            desc.m_byteCount = desc.m_elementSize * numElements;
            return AZ::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        };

        auto createRawBufferView = [](Data::Instance<Buffer>& buffer)
        {
            auto bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, static_cast<uint32_t>(buffer->GetBufferSize()));
            return buffer->GetRHIBuffer()->GetBufferView(bufferViewDescriptor);
        };

        for (int materialTypeIndex = 0; materialTypeIndex < m_materialTypeData.size(); materialTypeIndex++)
        {
            auto& materialTypeEntry = m_materialTypeData[materialTypeIndex];
            // The material-Type-Indices and instance-indices stay constant during their lifetime, which means we can get holes in
            // this buffer
            if (!materialTypeEntry.m_valid)
            {
                continue;
            }
            if (!materialTypeEntry.m_useSceneMaterialSrg)
            {
                continue;
            }
            // find the first valid shader parameter entry to determine the size of the MaterialParameter-Struct
            size_t bufferEntrySize = 0;
            for (auto& instanceData : materialTypeEntry.m_instanceData)
            {
                if (instanceData.m_shaderParameter)
                {
                    bufferEntrySize = instanceData.m_shaderParameter->GetStructuredBufferDataSize();
                    break;
                }
            }
            auto bufferSize = bufferEntrySize * materialTypeEntry.m_instanceIndices.MaxCount();

            // create or resize the MaterialParameter-buffer for this material-type
            if (materialTypeEntry.m_parameterBuffer == nullptr)
            {
                materialTypeEntry.m_parameterBuffer =
                    createMaterialParameterBuffer(materialTypeIndex, bufferEntrySize, materialTypeEntry.m_instanceData.size());
                materialTypeEntry.m_parameterBufferView = createRawBufferView(materialTypeEntry.m_parameterBuffer);
            }
            else if (materialTypeEntry.m_parameterBuffer->GetBufferSize() < bufferSize)
            {
                materialTypeEntry.m_parameterBuffer->Resize(bufferSize);
                materialTypeEntry.m_parameterBufferView = createRawBufferView(materialTypeEntry.m_parameterBuffer);
                // we need to re-upload the data after a resize
                for (auto& instanceData : materialTypeEntry.m_instanceData)
                {
                    instanceData.m_compiledChangeId = Material::DEFAULT_CHANGE_ID;
                }
            }
            materialTypeEntry.m_bindlessReadIndices = materialTypeEntry.m_parameterBufferView->GetBindlessReadIndex();
        }
    }

    bool MaterialSystem::UpdateSharedSamplerStates()
    {
        if (m_sceneMaterialSrg)
        {
            auto samplerIndex = m_sceneMaterialSrg->FindShaderInputSamplerIndex(AZ::Name{ "m_samplers" });
            if (samplerIndex.IsValid())
            {
                auto samplerStates = m_sceneTextureSamplers.CollectSamplerStates();
                if (!samplerStates.empty())
                {
                    m_sceneMaterialSrg->SetSamplerArray(samplerIndex, { samplerStates.begin(), samplerStates.end() });
                    return true;
                }
            }
        }
        return false;
    }

    bool MaterialSystem::UpdateSceneMaterialSrg()
    {
        auto createBuffer = [](const size_t numElements)
        {
            AZ::RPI::CommonBufferDescriptor desc;
            desc.m_elementFormat = AZ::RHI::Format::R32_UINT;
            desc.m_poolType = AZ::RPI::CommonBufferPoolType::ReadOnly;
            desc.m_elementSize = static_cast<uint32_t>(sizeof(uint32_t));
            desc.m_bufferName = "MaterialTypeBufferIndicesBuffer";
            desc.m_byteCount = desc.m_elementSize * numElements;
            return AZ::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        };

        if (m_sceneMaterialSrg)
        {
            const auto deviceCount{ AZ::RHI::RHISystemInterface::Get()->GetDeviceCount() };
            // init the data to upload: Material-Types without parameter buffer get a -1 read index
            AZStd::unordered_map<int, AZStd::vector<int32_t>> deviceBufferData;
            for (auto deviceIndex{ 0 }; deviceIndex < deviceCount; ++deviceIndex)
            {
                deviceBufferData[deviceIndex].resize(m_materialTypeIndices.MaxCount(), -1);
            }

            // collect the per-device read indices of the material parameter buffers
            for (auto materialTypeIndex{ 0 }; materialTypeIndex < m_materialTypeData.size(); materialTypeIndex++)
            {
                auto& materialTypeData = m_materialTypeData[materialTypeIndex];
                if (!materialTypeData.m_valid || !materialTypeData.m_useSceneMaterialSrg)
                {
                    continue;
                }
                for (const auto& [device, readIndex] : materialTypeData.m_bindlessReadIndices)
                {
                    deviceBufferData[device][materialTypeIndex] = static_cast<int32_t>(readIndex);
                }
            }

            // prepare / resize the GPU buffer
            auto indicesBufferSize = static_cast<uint64_t>(sizeof(int32_t) * m_materialTypeIndices.MaxCount());
            if (!m_materialTypeBufferIndicesBuffer)
            {
                m_materialTypeBufferIndicesBuffer = createBuffer(m_materialTypeData.size());
            }
            if (m_materialTypeBufferIndicesBuffer->GetBufferSize() < indicesBufferSize)
            {
                m_materialTypeBufferIndicesBuffer->Resize(indicesBufferSize);
            }

            // convert the map of vectors to a map of const void*
            AZStd::unordered_map<int, const void*> constDeviceBufferData;
            for (auto deviceIndex{ 0 }; deviceIndex < deviceCount; ++deviceIndex)
            {
                constDeviceBufferData[deviceIndex] = deviceBufferData[deviceIndex].data();
            }
            // upload the GPU data, with different data for each device
            m_materialTypeBufferIndicesBuffer->UpdateData(constDeviceBufferData, indicesBufferSize, 0);

            // register the buffer in the SRG and compile it
            m_sceneMaterialSrg->SetBuffer(m_materialTypeBufferInputIndex, m_materialTypeBufferIndicesBuffer);

            if (m_nullTexture)
            {
                // Register the bindless read index of the Null-Texture
                m_sceneMaterialSrg->SetConstant(m_nullTextureIndexInputIndex, m_nullTexture->GetImageView()->GetBindlessReadIndex());
            }
            return true;
        }
        return false;
    }

    void MaterialSystem::Compile()
    {
        bool compileSceneMaterialSrg = false;
        if (m_sharedSamplerStatesDirty)
        {
            if (UpdateSharedSamplerStates())
            {
                // make sure we try again if we didn't update the samplers successfully
                m_sharedSamplerStatesDirty = false;
                compileSceneMaterialSrg = true;
            }
        }

        if (m_bufferReadIndicesDirty)
        {
            PrepareMaterialParameterBuffers();
            if (UpdateSceneMaterialSrg())
            {
                // make sure we try again if we didn't update the SceneMaterialSrg successfully
                m_bufferReadIndicesDirty = false;
                compileSceneMaterialSrg = true;
            }
#ifdef DEBUG_MATERIALINSTANCES
            DebugPrintMaterialInstances();
#endif
        }
        UpdateChangedMaterialParameters();
        if (m_sceneMaterialSrg && compileSceneMaterialSrg)
        {
            m_sceneMaterialSrg->Compile();
        }
    }

    void MaterialSystem::Init()
    {
        MaterialInstanceHandlerInterface::Register(this);

        AZ::Data::InstanceHandler<Material> handler;
        handler.m_createFunction = [](Data::AssetData* materialAsset)
        {
            return Material::CreateInternal(*(azrtti_cast<MaterialAsset*>(materialAsset)));
        };
        Data::InstanceDatabase<Material>::Create(azrtti_typeid<MaterialAsset>(), handler);

        {
            auto defaultSampler = RHI::SamplerState::Create(RHI::FilterMode::Linear, RHI::FilterMode::Linear, RHI::AddressMode::Wrap);
            defaultSampler.m_anisotropyMax = 16;
            defaultSampler.m_anisotropyEnable = true;
            m_sceneTextureSamplers.Init(AZ_TRAITS_SCENE_MATERIALS_MAX_SAMPLERS, defaultSampler);
        }

        auto imageSystem = AZ::RPI::ImageSystemInterface::Get();
        AZ_Assert(imageSystem, "ImageSystem needs to be initialized before the MaterialSystem.");
        if (imageSystem)
        {
            auto pool = imageSystem->GetSystemAttachmentPool();
            auto bindFlags = RHI::GetImageBindFlags(RHI::ScopeAttachmentUsage::Shader, RHI::ScopeAttachmentAccess::Read);
            // create a 8x8 image with the RGBA values (0, 0, 0, 1) to use a similar behaviour as the robustness2 extension when reading a
            // null texture.
            auto nullImageDesc = RHI::ImageDescriptor::Create2D(bindFlags, 8, 8, RHI::Format::R8G8B8A8_UNORM);
            auto imageViewDesc = RHI::ImageViewDescriptor::Create(RHI::Format::R8G8B8A8_UNORM, 0, 0);
            RHI::ClearValue nullImageClearValue = RHI::ClearValue::CreateVector4Uint(0, 0, 0, 1);
            m_nullTexture = AZ::RPI::AttachmentImage::Create(
                *pool.get(), nullImageDesc, AZ_NAME_LITERAL("MaterialNullTexture"), &nullImageClearValue, &imageViewDesc);
        }
        m_initialized = true;
    }

    void MaterialSystem::Shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        if (m_sceneMaterialSrgShaderAsset)
        {
            AZ::Data::AssetBus::Handler::BusDisconnect(m_sceneMaterialSrgShaderAsset.GetId());
            m_sceneMaterialSrgShaderAsset.Reset();
        }
        if (m_sceneMaterialSrg)
        {
            m_sceneMaterialSrg.reset();
        }
        if (m_materialTypeBufferIndicesBuffer)
        {
            m_materialTypeBufferIndicesBuffer.reset();
        }
        m_materialTypeData.clear();
        if (m_nullTexture)
        {
            m_nullTexture.reset();
        }

        MaterialInstanceHandlerInterface::Unregister(this);
        Data::InstanceDatabase<Material>::Destroy();

        m_initialized = false;
    }

} // namespace AZ::RPI
