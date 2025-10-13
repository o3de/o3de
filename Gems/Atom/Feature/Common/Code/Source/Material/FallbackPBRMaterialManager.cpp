/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/Material/FallbackPBRMaterial.h>
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RPI.Public/Image/ImageSystem.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <Material/FallbackPBRMaterialManager.h>


namespace AZ::Render
{
    namespace GPU
    {
        // Note: we can't include FallbackPBRMaterialInfo.azsli or ReflectionProbeData.azsli here, since it contains
        // hlsl code that won't easily compile in c++

        // must match the structure in ReflectionProbeData.azsli
        using float3 = AZStd::array<float, 3>;
        using float4 = AZStd::array<float, 4>;
        using float3x4 = AZStd::array<float, 12>;
        struct ReflectionProbeData
        {
            float3x4 m_modelToWorld; // float3x4
            float3x4 m_modelToWorldInverse; // float3x4
            float3 m_outerObbHalfLengths; // float3
            float m_exposure = 0.0f;
            float3 m_innerObbHalfLengths; // float3
            uint32_t m_useReflectionProbe = 0;
            uint32_t m_useParallaxCorrection = 0;
            float3 m_padding;
        };

        // must match the structure in FallbackPBRMaterialInfo.azsli
        struct MaterialInfo
        {
            float4 m_baseColor = { 0, 0, 0, 0 };
            float4 m_irradianceColor = { 0, 0, 0, 0 };
            float3 m_emissiveColor = { 0, 0, 0 };
            float m_metallicFactor{ 0 };

            float m_roughnessFactor{ 0 };
            int m_baseColorImage{ -1 };
            int m_normalImage{ -1 };
            int m_metallicImage{ -1 };

            int m_roughnessImage{ -1 };
            int m_emissiveImage{ -1 };
            uint32_t m_reflectionProbeCubeMapIndex{ 0 };
            uint32_t m_pad;

            ReflectionProbeData m_reflectionProbeData;
        };

        static_assert(sizeof(ReflectionProbeData) % 16 == 0, "GPU struct ReflectionProbeData does not align to 16 bytes");
        static_assert(sizeof(MaterialInfo) % 16 == 0, "GPU struct MaterialInfo does not align to 16 bytes");
    } // namespace GPU

    // Disabling this will disable all update - functions of the FallbackPBR::MaterialManager, and SceneSrg::m_fallbackPBRMaterial will
    // be a buffer with a single empty entry. This does not modify the shaders though, so be careful about accessing the SceneSrg.
    AZ_CVAR(
        bool,
        r_fallbackPBRMaterialEnabled,
        true,
        nullptr,
        AZ::ConsoleFunctorFlags::NeedsReload,
        "Enable creation of Fallback PBR material entries for each mesh.");

    namespace FallbackPBR
    {

        MaterialManager::MaterialManager()
            : m_materialDataBuffer{ "FallbackPBR::MaterialInfo",
                                    RPI::CommonBufferPoolType::ReadOnly,
                                    static_cast<uint32_t>(sizeof(GPU::MaterialInfo)) }
        {
        }

        const Data::Instance<RPI::Buffer>& MaterialManager::GetFallbackPBRMaterialBuffer() const
        {
            return m_materialDataBuffer.GetCurrentBuffer();
        }

        const RHI::Ptr<MaterialEntry> MaterialManager::GetFallbackPBRMaterialEntry(const MeshInfoHandle handle)
        {
            RHI::Ptr<MaterialEntry> entry{};
            if (!m_isEnabled)
            {
                return entry;
            }

            {
                AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
                if (m_materialData.size() > handle.GetIndex())
                {
                    entry = m_materialData[handle.GetIndex()];
                }
            }
            return entry;
        }

        void MaterialManager::InitNullCubeMap()
        {
            auto imageSystem = AZ::RPI::ImageSystemInterface::Get();
            AZ_Assert(imageSystem, "Can't access RPI::ImageSystem.");
            if (imageSystem)
            {
                auto pool = imageSystem->GetSystemAttachmentPool();
                auto bindFlags = RHI::GetImageBindFlags(RHI::ScopeAttachmentUsage::Shader, RHI::ScopeAttachmentAccess::Read);
                // create a 8x8 cubemap image with the RGBA values (0, 0, 0, 1) to use a similar behaviour as the robustness2 extension when
                // reading a null texture.
                auto nullImageDesc = RHI::ImageDescriptor::CreateCubemap(bindFlags, 8, RHI::Format::R8G8B8A8_UNORM);
                auto imageViewDesc = RHI::ImageViewDescriptor::CreateCubemap();
                RHI::ClearValue nullImageClearValue = RHI::ClearValue::CreateVector4Uint(0, 0, 0, 1);
                m_nullCubeMapTexture = AZ::RPI::AttachmentImage::Create(
                    *pool.get(), nullImageDesc, AZ_NAME_LITERAL("ReflectionCubeMapNullTexture"), &nullImageClearValue, &imageViewDesc);
            }
        }

        void MaterialManager::Activate(RPI::Scene* scene)
        {
            if (auto* console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
            {
                console->GetCvarValue("r_fallbackPBRMaterialEnabled", m_isEnabled);
            }

            InitNullCubeMap();

            UpdateFallbackPBRMaterialBuffer();

            // We need to register the buffer in the SceneSrg even if we are disabled
            m_updateSceneSrgHandler = RPI::Scene::PrepareSceneSrgEvent::Handler(
                [this](RPI::ShaderResourceGroup* sceneSrg)
                {
                    sceneSrg->SetBufferView(m_fallbackPBRMaterialIndex, GetFallbackPBRMaterialBuffer()->GetBufferView());
                    sceneSrg->SetConstant(m_nullCubeMapIndex, m_nullCubeMapTexture->GetImageView()->GetBindlessReadIndex());
                });
            scene->ConnectEvent(m_updateSceneSrgHandler);

            m_rpfp = scene->GetFeatureProcessor<ReflectionProbeFeatureProcessorInterface>();

            if (m_isEnabled == false)
            {
                return;
            }

            MeshInfoNotificationBus::Handler::BusConnect(scene->GetId());
        }

        void MaterialManager::Deactivate()
        {
            MeshInfoNotificationBus::Handler::BusDisconnect();
        }

        void MaterialManager::OnAcquireMeshInfoEntry(const MeshInfoHandle meshInfoHandle)
        {
            if (!m_isEnabled)
            {
                return;
            }
            AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
            if (m_materialData.size() <= meshInfoHandle.GetIndex())
            {
                // Allocate several entries so we can avoid reallocation both on the CPU and GPU
                constexpr static int minEntries = 32;
                const auto numEntries = AlignUpToPowerOfTwo(AZStd::max(meshInfoHandle.GetIndex() + 1, minEntries));
                m_materialData.resize(numEntries, nullptr);
            }
            m_materialData[meshInfoHandle.GetIndex()] = aznew FallbackPBR::MaterialEntry{};
        }

        void MaterialManager::OnPopulateMeshInfoEntry(
            const MeshInfoHandle meshInfoHandle, ModelDataInstanceInterface* modelData, const size_t lodIndex, const size_t lodMeshIndex)
        {
            if (!m_isEnabled)
            {
                return;
            }
            if (m_materialData.size() <= meshInfoHandle.GetIndex())
            {
                AZ_Assert(m_materialData.size() > meshInfoHandle.GetIndex(), "OnPopulateMeshInfoEntry() called with invalid index");
                return;
            }
            RHI::Ptr<MaterialEntry> entry;
            {
                AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
                entry = m_materialData[meshInfoHandle.GetIndex()];
            }

            const auto& model = modelData->GetModel();
            const auto& modelLod = model->GetLods()[lodIndex];
            const auto& mesh = modelLod->GetMeshes()[lodMeshIndex];
            // Determine if there is a custom material specified for this mesh
            const AZ::Render::CustomMaterialId customMaterialId(aznumeric_cast<AZ::u64>(lodMeshIndex), mesh.m_materialSlotStableId);
            const auto& customMaterialInfo = modelData->GetCustomMaterialWithFallback(customMaterialId);
            const auto& material = customMaterialInfo.m_material ? customMaterialInfo.m_material : mesh.m_material;

            entry->m_objectId = modelData->GetObjectId();
            entry->m_material = material;
            entry->m_materialChangeId = RPI::Material::DEFAULT_CHANGE_ID;
        }

        void MaterialManager::OnReleaseMeshInfoEntry(const MeshInfoHandle meshInfoHandle)
        {
            if (!m_isEnabled)
            {
                return;
            }
            AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
            if (m_materialData.size() <= meshInfoHandle.GetIndex())
            {
                AZ_Assert(m_materialData.size() > meshInfoHandle.GetIndex(), "OnReleaseMeshInfoEntry() called with invalid index");
                return;
            }
            auto objectId = m_materialData[meshInfoHandle.GetIndex()]->m_objectId;
            m_materialData[meshInfoHandle.GetIndex()] = nullptr;

            // check if this is the last mesh with this object-id, and delete the ReflectionProbe - data for this entry
            // TODO: maybe don't do this with a loop over all objects
            if (m_reflectionProbeData.contains(objectId))
            {
                bool deleteReflectionProbeData = true;
                for (auto& entry : m_materialData)
                {
                    if (entry && entry->m_objectId == objectId)
                    {
                        deleteReflectionProbeData = false;
                        break;
                    }
                }
                if (deleteReflectionProbeData)
                {
                    m_reflectionProbeData.erase(objectId);
                }
            }

            if (meshInfoHandle.GetIndex() + 1 == m_materialData.size())
            {
                int deleteBackEntries = 0;
                for (int index = meshInfoHandle.GetIndex(); index >= 0; index--)
                {
                    if (m_materialData[index] == nullptr)
                    {
                        deleteBackEntries++;
                    }
                    else
                    {
                        break;
                    }
                }
                m_materialData.resize(m_materialData.size() - deleteBackEntries);
            }
            m_bufferNeedsUpdate = true;
        }

        void MaterialManager::UpdateFallbackPBRMaterialEntry(
            const MeshInfoHandle handle, AZStd::function<bool(MaterialEntry*)> updateFunction)
        {
            if (!m_isEnabled)
            {
                return;
            }
            RHI::Ptr<MaterialEntry> entry;
            {
                AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
                if (m_materialData.size() > handle.GetIndex() && m_materialData[handle.GetIndex()] != nullptr)
                {
                    // make a copy of the smart pointer to make sure the entry isn't deleted during the update function
                    entry = m_materialData[handle.GetIndex()];
                }
                else
                {
                    AZ_Assert(false, "UpdateFallbackPBRMaterialEntry() called with invalid index");
                }
            }
            if (entry)
            {
                m_bufferNeedsUpdate |= updateFunction(entry.get());
            }
        }

        void MaterialManager::UpdateReflectionProbes(
            const TransformServiceFeatureProcessorInterface::ObjectId& objectId, const Aabb& aabbWS)
        {
            if (!m_isEnabled || !m_rpfp)
            {
                return;
            }

            bool hasReflectionProbeData = m_reflectionProbeData.contains(objectId);

            ReflectionProbeHandleVector reflectionProbeHandles;
            m_rpfp->FindReflectionProbes(aabbWS, reflectionProbeHandles);

            if (reflectionProbeHandles.empty())
            {
                if (hasReflectionProbeData)
                {
                    m_reflectionProbeData.erase(objectId);
                    m_bufferNeedsUpdate = true;
                }
            }
            else
            {
                if (!hasReflectionProbeData)
                {
                    m_reflectionProbeData.emplace(AZStd::make_pair(objectId, ReflectionProbe{}));
                }
                ReflectionProbe& reflectionProbe = m_reflectionProbeData.at(objectId);

                // take the last handle from the list, which will be the smallest (most influential) probe
                ReflectionProbeHandle handle = reflectionProbeHandles.back();
                reflectionProbe.m_modelToWorld = m_rpfp->GetTransform(handle);
                reflectionProbe.m_outerObbHalfLengths = m_rpfp->GetOuterObbWs(handle).GetHalfLengths();
                reflectionProbe.m_innerObbHalfLengths = m_rpfp->GetInnerObbWs(handle).GetHalfLengths();
                reflectionProbe.m_useParallaxCorrection = m_rpfp->GetUseParallaxCorrection(handle);
                reflectionProbe.m_exposure = m_rpfp->GetRenderExposure(handle);
                reflectionProbe.m_reflectionProbeCubeMap = m_rpfp->GetCubeMap(handle);

                m_bufferNeedsUpdate = true;
            }
        }

        void MaterialManager::UpdateFallbackPBRMaterial()
        {
            for (int index = 0; index < static_cast<int>(m_materialData.size()); ++index)
            {
                const auto& entry = m_materialData[index];
                if (entry && entry->m_material)
                {
                    if (entry->m_material->GetCurrentChangeId() != entry->m_materialChangeId)
                    {
                        ConvertMaterial(entry->m_material.get(), entry->m_materialParameters);
                        entry->m_materialChangeId = entry->m_material->GetCurrentChangeId();
                        m_bufferNeedsUpdate = true;
                    }
                }
            }
        }

        void MaterialManager::UpdateFallbackPBRMaterialBuffer()
        {
            AZStd::unordered_map<int, AZStd::vector<GPU::MaterialInfo>> multiDeviceMaterialData;
            AZStd::unordered_map<int, const void*> updateDataHelper;

            const GPU::MaterialInfo emptyMaterial{};

            const auto deviceCount{ AZ::RHI::RHISystemInterface::Get()->GetDeviceCount() };
            // Initialize at least one entry for each device so we don't have dangling buffers
            const auto numEntries = AZStd::max(m_materialData.size(), size_t{ 1 });

            for (auto deviceIndex{ 0 }; deviceIndex < deviceCount; ++deviceIndex)
            {
                // reserve the memory that will be uploaded to the GPU
                multiDeviceMaterialData[deviceIndex].resize(numEntries, emptyMaterial);
                updateDataHelper[deviceIndex] = multiDeviceMaterialData[deviceIndex].data();
            }

            // write the (multidevice) ReadIndices into a (single-device) meshInfo - entry
            auto SetMaterialParameters = [](GPU::MaterialInfo& out, const MaterialParameters& params, const int deviceIndex)
            {
                params.m_baseColor.StoreToFloat4(out.m_baseColor.data());
                params.m_emissiveColor.StoreToFloat4(out.m_emissiveColor.data());
                params.m_irradianceColor.StoreToFloat4(out.m_irradianceColor.data());
                out.m_metallicFactor = params.m_metallicFactor;
                out.m_roughnessFactor = params.m_roughnessFactor;

                auto GetDeviceBindlessReadIndex = [&deviceIndex](const RHI::Ptr<const RHI::ImageView>& imageView)
                {
                    if (imageView != nullptr)
                    {
                        return static_cast<int32_t>(imageView->GetDeviceImageView(deviceIndex)->GetBindlessReadIndex());
                    }
                    return -1;
                };

                out.m_baseColorImage = GetDeviceBindlessReadIndex(params.m_baseColorImageView);
                out.m_normalImage = GetDeviceBindlessReadIndex(params.m_normalImageView);
                out.m_metallicImage = GetDeviceBindlessReadIndex(params.m_metallicImageView);
                out.m_roughnessImage = GetDeviceBindlessReadIndex(params.m_roughnessImageView);
                out.m_emissiveImage = GetDeviceBindlessReadIndex(params.m_emissiveImageView);

                return out;
            };

            auto SetReflectionProbeData = [](GPU::MaterialInfo& out, const ReflectionProbe* reflectionProbe, const int deviceIndex)
            {
                // add reflection probe data
                if (reflectionProbe->m_reflectionProbeCubeMap.get())
                {
                    Matrix3x4 reflectionProbeModelToWorld3x4 = Matrix3x4::CreateFromTransform(reflectionProbe->m_modelToWorld);
                    out.m_reflectionProbeCubeMapIndex =
                        reflectionProbe->m_reflectionProbeCubeMap->GetImageView()->GetDeviceImageView(deviceIndex)->GetBindlessReadIndex();
                    if (out.m_reflectionProbeCubeMapIndex != RHI::DeviceImageView::InvalidBindlessIndex)
                    {
                        reflectionProbeModelToWorld3x4.StoreToRowMajorFloat12(out.m_reflectionProbeData.m_modelToWorld.data());
                        reflectionProbeModelToWorld3x4.GetInverseFull().StoreToRowMajorFloat12(
                            out.m_reflectionProbeData.m_modelToWorldInverse.data());
                        reflectionProbe->m_outerObbHalfLengths.StoreToFloat3(out.m_reflectionProbeData.m_outerObbHalfLengths.data());
                        reflectionProbe->m_innerObbHalfLengths.StoreToFloat3(out.m_reflectionProbeData.m_innerObbHalfLengths.data());
                        out.m_reflectionProbeData.m_useReflectionProbe = true;
                        out.m_reflectionProbeData.m_useParallaxCorrection = reflectionProbe->m_useParallaxCorrection;
                        out.m_reflectionProbeData.m_exposure = reflectionProbe->m_exposure;
                    }
                }
            };

            for (int meshInfoIndex = 0; meshInfoIndex < numEntries; meshInfoIndex++)
            {
                if (m_materialData.empty() || m_materialData[meshInfoIndex] == nullptr)
                {
                    continue;
                }
                auto& entry = m_materialData[meshInfoIndex];
                ReflectionProbe* reflectionProbe = nullptr;
                if (m_reflectionProbeData.contains(entry->m_objectId))
                {
                    reflectionProbe = &m_reflectionProbeData.at(entry->m_objectId);
                }
                for (auto deviceIndex{ 0 }; deviceIndex < deviceCount; ++deviceIndex)
                {
                    auto& gpuData = multiDeviceMaterialData[deviceIndex][meshInfoIndex];
                    SetMaterialParameters(gpuData, entry->m_materialParameters, deviceIndex);
                    if (reflectionProbe)
                    {
                        SetReflectionProbeData(gpuData, reflectionProbe, deviceIndex);
                    }
                    else
                    {
                        gpuData.m_reflectionProbeData = GPU::ReflectionProbeData{};
                        gpuData.m_reflectionProbeCubeMapIndex = RHI::DeviceImageView::InvalidBindlessIndex;
                    }
                }
            }
            // copy the data to the GPU
            m_materialDataBuffer.AdvanceCurrentBufferAndUpdateData(updateDataHelper, numEntries * sizeof(GPU::MaterialInfo));
        }

        void MaterialManager::Update()
        {
            if (!m_isEnabled)
            {
                return;
            }
            UpdateFallbackPBRMaterial();
            if (m_bufferNeedsUpdate)
            {
                UpdateFallbackPBRMaterialBuffer();
                m_bufferNeedsUpdate = false;
            }
        }
    } // namespace FallbackPBR
} // namespace AZ::Render