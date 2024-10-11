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

namespace AZ
{
    namespace RPI
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
                // Load the default vbuffer material asynchronously
                const AZStd::string materialSrgShader = "shaders/scenematerialsrg.azshader";
                m_sceneMaterialSrgShaderAsset =
                    AssetUtils::LoadCriticalAsset<RPI::ShaderAsset>(materialSrgShader.data(), RPI::AssetUtils::TraceLevel::Error);
            }
            if (!m_sceneMaterialSrgShaderAsset)
            {
                // neither the VBufferMaterial nor the fallback - asset-Ids were valid: try again, maybe the assetProcessor is still running
                AZ_Warning("MaterialSystem", false, "Unable to locate the Material SRG shader asset, try again");
                return false;
            }

            PrepareMaterialSrg();

            AZ::Data::AssetBus::Handler::BusConnect(m_sceneMaterialSrgShaderAsset.GetId());
            return true;
        }

        void MaterialSystem::PrepareMaterialSrg()
        {
            if (m_sceneMaterialSrgShaderAsset->IsReady())
            {
                m_sceneMaterialSrg = ShaderResourceGroup::Create(m_sceneMaterialSrgShaderAsset, AZ::Name("SceneMaterialSrg"));
            }
        }

        //  Data::AssetBus Interface
        void MaterialSystem::OnAssetReloaded([[maybe_unused]] AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            PrepareMaterialSrg();
        }

        void MaterialSystem::OnAssetReady([[maybe_unused]] AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            PrepareMaterialSrg();
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

            // Note: the Asset-Id of the Material-type isn't enough, i also need to check the Shader Options
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
                        materialTypeData.m_shaderResourceGroup = m_sceneMaterialSrg;
                    }
                    else
                    {
                        auto srgShaderAsset = materialTypeAsset->GetShaderAssetForMaterialSrg();
                        materialTypeData.m_shaderResourceGroup = ShaderResourceGroup::Create(srgShaderAsset, srgLayout->GetName());
                    }
                }
            }
            else
            {
                materialTypeIndex = materialTypeAssetIterator->second;
            }
            MaterialTypeData& materialTypeData = m_materialTypeData[materialTypeIndex];

            auto materialInstanceIndex = materialTypeData.m_instanceIndices.Aquire();
            materialTypeData.m_instanceData.resize(materialTypeData.m_instanceIndices.Max(), {});
            auto& instanceData = materialTypeData.m_instanceData[materialInstanceIndex];

            instanceData.m_shaderParameter = aznew MaterialShaderParameter(materialTypeData.m_shaderParameterLayout.get());
            instanceData.m_material = material.get();
            instanceData.m_compiledChangeId = Material::DEFAULT_CHANGE_ID;

            MaterialInstanceData result{
                materialTypeIndex, materialInstanceIndex, materialTypeData.m_shaderResourceGroup, instanceData.m_shaderParameter
            };

            AZ_Printf(
                "MaterialSystem",
                "RegisterMaterialInstance: Register Type %d (%s), Instance %d (%s) (max: %d)",
                materialTypeIndex,
                materialTypeData.m_materialTypeAssetHint.c_str(),
                materialInstanceIndex,
                instanceData.m_material->GetAsset().GetHint().c_str(),
                materialTypeData.m_instanceIndices.Max());

            return result;
        }

        void MaterialSystem::ReleaseMaterialInstance(const MaterialInstanceData& materialInstance)
        {
            m_bufferReadIndicesDirty = true;

            MaterialTypeData* materialTypeData = &m_materialTypeData[materialInstance.m_materialTypeId];
            InternalMaterialInstanceData* materialInstanceData = &materialTypeData->m_instanceData[materialInstance.m_materialInstanceId];

            AZ_Printf(
                "MaterialSystem",
                "ReleaseMaterialInstance: Release Type %d(%s), Instance %d (%s) (max: %d)",
                materialInstance.m_materialTypeId,
                materialTypeData->m_materialTypeAssetHint.c_str(),
                materialInstance.m_materialInstanceId,
                materialInstanceData->m_material->GetAsset().GetHint().c_str(),
                materialTypeData->m_instanceIndices.Max());

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
                    result += AZStd::string::format("dev_%d: %d", deviceIndex, static_cast<int>(indices.at(deviceIndex)));
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
                    AZ_Printf("MaterialSystem", "MaterialType [%d]: Empty", materialTypeIndex);
                    continue;
                }
                AZ_Printf(
                    "MaterialSystem",
                    "MaterialType [%d]: %s, ReadIndices = [%s]",
                    materialTypeIndex,
                    materialTypeEntry.m_materialTypeAssetHint.c_str(),
                    readIndices(materialTypeEntry.m_bindlessReadIndices).c_str());
                for (int instanceIndex = 0; instanceIndex < materialTypeEntry.m_instanceData.size(); instanceIndex++)
                {
                    auto& materialInstanceEntry = materialTypeEntry.m_instanceData[instanceIndex];
                    if (!materialInstanceEntry.m_material)
                    {
                        AZ_Printf("MaterialSystem", "    Instance [%d]: Empty", instanceIndex);
                        continue;
                    }
                    AZ_Printf(
                        "MaterialSystem",
                        "    Instance [%d]: %s, Offset %d, size %d bytes",
                        instanceIndex,
                        materialInstanceEntry.m_material->GetAsset().GetHint().c_str(),
                        materialInstanceEntry.m_shaderParameter->GetStructuredBufferDataSize() * instanceIndex,
                        materialInstanceEntry.m_shaderParameter->GetStructuredBufferDataSize());
                }
            }
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
                    if (instanceData.m_material)
                    {
                        shaderParamsSize = instanceData.m_shaderParameter->GetStructuredBufferDataSize();
                        break;
                    }
                }
                AZStd::vector<uint8_t> emptyShaderParams;
                // set the first 8 bits (materialType id and Instance Id) to -1
                emptyShaderParams.resize(8, 255u);
                // and zero out the rest
                emptyShaderParams.resize(shaderParamsSize, 0);
                for (int32_t instanceIndex = 0; instanceIndex < materialTypeEntry.m_instanceIndices.Max(); instanceIndex++)
                {
                    auto& instanceData = materialTypeEntry.m_instanceData[instanceIndex];
                    if (instanceData.m_material)
                    {
                        if (instanceData.m_material->GetCurrentChangeId() != instanceData.m_compiledChangeId)
                        {
                            auto shaderParamsData = instanceData.m_shaderParameter->GetStructuredBufferData();
                            materialTypeEntry.m_parameterBuffer->UpdateData(
                                shaderParamsData, shaderParamsSize, instanceIndex * shaderParamsSize);
                            instanceData.m_compiledChangeId = instanceData.m_material->GetCurrentChangeId();
                        }
                    }
                    // Reset the data to catch wrong access
                    else if (instanceData.m_compiledChangeId == 0)
                    {
                        materialTypeEntry.m_parameterBuffer->UpdateData(
                            emptyShaderParams.data(), shaderParamsSize, instanceIndex * shaderParamsSize);
                        // this will be reset upon aquire, so we can use it to track if we already reset the gpu data
                        instanceData.m_compiledChangeId = 1;
                    }
                }
            }
        }

        void MaterialSystem::PrepareMaterialParameterBuffers()
        {
            auto createBuffer =
                [](const RHI::Format format, const AZStd::string& name, const uint32_t elementSize, const uint32_t numElements)
            {
                AZ::RPI::CommonBufferDescriptor desc;
                desc.m_elementFormat = format;
                desc.m_poolType = AZ::RPI::CommonBufferPoolType::ReadOnly;
                desc.m_elementSize = elementSize;
                desc.m_bufferName = name;
                desc.m_byteCount = static_cast<uint64_t>(desc.m_elementSize) * numElements;
                return AZ::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
            };

            auto createRawBufferView = [](Data::Instance<Buffer>& buffer)
            {
                auto bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, buffer->GetBufferSize());
                return buffer->GetRHIBuffer()->BuildBufferView(bufferViewDescriptor);
            };

            const auto deviceCount{ AZ::RHI::RHISystemInterface::Get()->GetDeviceCount() };
            // multi-device read indices: each MaterialType gets a map with the read-index on each device
            AZStd::vector<AZStd::unordered_map<int, uint32_t>> materialParameterBufferReadIndices;
            materialParameterBufferReadIndices.resize(m_materialTypeData.size());

            for (int materialTypeIndex = 0; materialTypeIndex < m_materialTypeData.size(); materialTypeIndex++)
            {
                auto& materialTypeEntry = m_materialTypeData[materialTypeIndex];
                // The material-Type-Indices and instance-indices stay constant during their lifetime, which means we can get holes in
                // this buffer
                if (!materialTypeEntry.m_valid)
                {
                    for (auto deviceIndex{ 0 }; deviceIndex < deviceCount; ++deviceIndex)
                    {
                        materialParameterBufferReadIndices[materialTypeIndex][deviceIndex] = static_cast<uint32_t>(-1);
                    }
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
                    auto buffername = AZStd::string::format("MaterialParameterBuffer_%d", materialTypeIndex);
                    materialTypeEntry.m_parameterBuffer =
                        createBuffer(AZ::RHI::Format::Unknown, buffername, bufferEntrySize, materialTypeEntry.m_instanceData.size());
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

                materialParameterBufferReadIndices[materialTypeIndex] = materialTypeEntry.m_parameterBufferView->GetBindlessReadIndex();
                materialTypeEntry.m_bindlessReadIndices = materialParameterBufferReadIndices[materialTypeIndex];

                // Register the buffer with the Srg and compile it if the material doesn't use the global material srg
                if (materialTypeEntry.m_shaderResourceGroup != m_sceneMaterialSrg && materialTypeEntry.m_shaderResourceGroup)
                {
                    auto bufferIndex = RHI::ShaderInputNameIndex{ "m_materialParameters" };
                    materialTypeEntry.m_shaderResourceGroup->SetBuffer(bufferIndex, materialTypeEntry.m_parameterBuffer);
                    materialTypeEntry.m_shaderResourceGroup->Compile();
                }
            }
            if (m_sceneMaterialSrg)
            {
                auto indicesBufferSize = static_cast<uint64_t>(sizeof(int32_t) * m_materialTypeData.size());
                if (!m_materialTypeBufferIndicesBuffer)
                {
                    m_materialTypeBufferIndicesBuffer = createBuffer(
                        AZ::RHI::Format::R32_SINT, "MaterialTypeBufferIndicesBuffer", sizeof(int32_t), m_materialTypeData.size());
                }
                if (m_materialTypeBufferIndicesBuffer->GetBufferSize() < indicesBufferSize)
                {
                    m_materialTypeBufferIndicesBuffer->Resize(indicesBufferSize);
                }
                AZStd::unordered_map<int, AZStd::vector<int32_t>> deviceBufferData;
                AZStd::unordered_map<int, const void*> constDeviceBufferData;
                uint64_t bufferDataSize;
                for (auto deviceIndex{ 0 }; deviceIndex < deviceCount; ++deviceIndex)
                {
                    for (auto& deviceReadIndices : materialParameterBufferReadIndices)
                    {
                        deviceBufferData[deviceIndex].push_back(static_cast<int32_t>(deviceReadIndices[deviceIndex]));
                    }
                    constDeviceBufferData[deviceIndex] = deviceBufferData[deviceIndex].data();
                    bufferDataSize = deviceBufferData[deviceIndex].size() * sizeof(int32_t);
                }
                m_materialTypeBufferIndicesBuffer->UpdateData(constDeviceBufferData, bufferDataSize, 0);

                m_sceneMaterialSrg->SetBuffer(m_materialTypeBufferInputIndex, m_materialTypeBufferIndicesBuffer);
                m_sceneMaterialSrg->Compile();
            }
        }

        void MaterialSystem::Compile()
        {
            if (m_bufferReadIndicesDirty)
            {
                PrepareMaterialParameterBuffers();
                m_bufferReadIndicesDirty = false;
                DebugPrintMaterialInstances();
            }
            UpdateChangedMaterialParameters();
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

    } // namespace RPI
} // namespace AZ
