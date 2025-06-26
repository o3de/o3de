/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Material/MaterialInstanceHandler.h>
#include <Atom/RPI.Public/Material/MaterialSystem.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/LuaMaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <AtomCore/Instance/InstanceDatabase.h>
#include <AzCore/Name/NameDictionary.h>

#include <Atom_RPI_Traits_Platform.h>

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
                    maxTextureSamplerStates == m_sceneTextureSamplers.GetMaxNumSamplerStates(),
                    "SceneMaterialSrg::m_samplers[] has size %d, expected size is AZ_TRAITS_SCENE_MATERIALS_MAX_SAMPLERS (%d)",
                    maxTextureSamplerStates,
                    AZ_TRAITS_SCENE_MATERIALS_MAX_SAMPLERS);
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

        auto materialAsset = material->GetAsset();
        auto materialTypeAsset = materialAsset->GetMaterialTypeAsset();

        // Note: We store the Material-Parameters in a single SRG, but each object gets it's own draw-item, which holds the shader
        // options, so we don't need to consider them. However, for raytracing or deferred approaches, where one shader shades multiple
        // material-instances, we probably want to different Material-IDs for different Shader Options
        auto materialTypeAssetIterator = m_materialTypeIndicesMap.find(materialTypeAsset.GetId());
        if (materialTypeAssetIterator == m_materialTypeIndicesMap.end())
        {
            materialTypeIndex = m_materialTypeIndices.Aquire();
            m_materialTypeData.resize(m_materialTypeIndices.Max());
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
        }
        MaterialTypeData& materialTypeData = m_materialTypeData[materialTypeIndex];

        auto materialInstanceIndex = materialTypeData.m_instanceIndices.Aquire();
        materialTypeData.m_instanceData.resize(materialTypeData.m_instanceIndices.Max());
        auto& instanceData = materialTypeData.m_instanceData[materialInstanceIndex];

        instanceData.m_material = material.get();
        instanceData.m_compiledChangeId = Material::DEFAULT_CHANGE_ID;

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
                    instanceData.m_textureSamplers = AZStd::make_unique<TextureSamplerRegistry>();
                    instanceData.m_textureSamplers->Init(desc.m_count, defaultSampler);
                }

#ifdef AZ_TRAIT_REGISTER_TEXTURES_PER_MATERIAL
                // get the size of the m_samplers[] array from the SRG layout
                auto materialTexturesIndex =
                    instanceData.m_shaderResourceGroup->GetLayout()->FindShaderInputImageIndex(AZ::Name{ "m_textures" });
                if (materialTexturesIndex.IsValid())
                {
                    auto desc = instanceData.m_shaderResourceGroup->GetLayout()->GetShaderInput(materialTexturesIndex);
                    instanceData.m_materialTextureRegistry = AZStd::make_unique<MaterialTextureRegistry>();
                    instanceData.m_materialTextureRegistry->Init(desc.m_count);
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
            materialTypeData.m_instanceIndices.Max());
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
            materialTypeData->m_instanceIndices.Max());
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
            for (int32_t instanceIndex = 0; instanceIndex < materialTypeEntry.m_instanceIndices.Max(); instanceIndex++)
            {
                auto& instanceData = materialTypeEntry.m_instanceData[instanceIndex];
                if (instanceData.m_material && instanceData.m_material->GetCurrentChangeId() != instanceData.m_compiledChangeId)
                {
                    if (materialTypeEntry.m_useSceneMaterialSrg)
                    {
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
            auto bufferSize = bufferEntrySize * materialTypeEntry.m_instanceIndices.Max();

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

    void MaterialSystem::UpdateSceneMaterialSrg()
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
                deviceBufferData[deviceIndex].resize(m_materialTypeIndices.Max(), -1);
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
            auto indicesBufferSize = static_cast<uint64_t>(sizeof(int32_t) * m_materialTypeIndices.Max());
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
        }
    }

    void MaterialSystem::Compile()
    {
        bool compileSceneMaterialSrg = false;
        if (m_sharedSamplerStatesDirty)
        {
            m_sharedSamplerStatesDirty = false;
            compileSceneMaterialSrg = UpdateSharedSamplerStates();
        }

        if (m_bufferReadIndicesDirty)
        {
            PrepareMaterialParameterBuffers();
            UpdateSceneMaterialSrg();
            m_bufferReadIndicesDirty = false;
#ifdef DEBUG_MATERIALINSTANCES
            DebugPrintMaterialInstances();
#endif
            compileSceneMaterialSrg = true;
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

        auto defaultSampler = RHI::SamplerState::Create(RHI::FilterMode::Linear, RHI::FilterMode::Linear, RHI::AddressMode::Wrap);
        defaultSampler.m_anisotropyMax = 16;
        m_sceneTextureSamplers.Init(AZ_TRAITS_SCENE_MATERIALS_MAX_SAMPLERS, defaultSampler);
    }

    void MaterialSystem::Shutdown()
    {
        if (m_sceneMaterialSrgShaderAsset)
        {
            AZ::Data::AssetBus::Handler::BusDisconnect(m_sceneMaterialSrgShaderAsset.GetId());
            m_sceneMaterialSrgShaderAsset->Release();
        }
        MaterialInstanceHandlerInterface::Unregister(this);
        Data::InstanceDatabase<Material>::Destroy();
    }

} // namespace AZ::RPI
