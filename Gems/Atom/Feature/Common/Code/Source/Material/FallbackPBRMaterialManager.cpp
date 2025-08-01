/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/Feature/Material/ConvertEmissiveUnitFunctor.h>
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/std/containers/array.h>
#include <Material/FallbackPBRMaterialManager.h>

namespace AZ::Render
{
    namespace GPU
    {

        // reflection probe data, must match the structure in ReflectionProbeData.azsli
        struct alignas(16) ReflectionProbeData
        {
            AZStd::array<float, 12> m_modelToWorld; // float3x4
            AZStd::array<float, 12> m_modelToWorldInverse; // float3x4
            AZStd::array<float, 3> m_outerObbHalfLengths; // float3
            float m_exposure = 0.0f;
            AZStd::array<float, 3> m_innerObbHalfLengths; // float3
            uint32_t m_useReflectionProbe = 0;
            uint32_t m_useParallaxCorrection = 0;
            AZStd::array<float, 3> m_padding;
        };

        // material data, must match the structure in FallbackPBRMaterialInfo.azsli
        struct alignas(16) MaterialInfo
        {
            AZStd::array<float, 4> m_baseColor = { 0, 0, 0, 0 };

            AZStd::array<float, 4> m_irradianceColor = { 0, 0, 0, 0 };

            AZStd::array<float, 3> m_emissiveColor = { 0, 0, 0 };
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
    } // namespace GPU

    AZ_CVAR(
        bool,
        r_fallbackPBRMaterialEnabled,
        true,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "Enable creation of Fallback PBR material entries for each mesh.");

    class MaterialConversionUtil
    {
    public:
        MaterialConversionUtil(RPI::Material* sourceMaterial)
            : m_material(sourceMaterial)
        {
        }

        template<typename T>
        T GetProperty(const AZ::Name& propertyName, const T defaultValue = T{})
        {
            auto propertyIndex = m_material->FindPropertyIndex(propertyName);
            if (propertyIndex.IsValid())
            {
                return m_material->GetPropertyValue<T>(propertyIndex);
            }
            return defaultValue;
        }

        template<typename T, size_t N>
        T GetProperty(const AZStd::array<AZ::Name, N> propertyNames, const T defaultValue = T{})
        {
            for (const auto& propertyName : propertyNames)
            {
                auto propertyIndex = m_material->FindPropertyIndex(propertyName);
                if (propertyIndex.IsValid())
                {
                    return m_material->GetPropertyValue<T>(propertyIndex);
                }
            }
            return defaultValue;
        }

        template<>
        RHI::Ptr<const RHI::ImageView> GetProperty(const AZ::Name& propertyName, const RHI::Ptr<const RHI::ImageView> defaultValue)
        {
            auto image = GetProperty<Data::Instance<RPI::Image>>(propertyName, nullptr);
            if (image)
            {
                return image->GetImageView();
            }
            return defaultValue;
        }

        template<typename T>
        RHI::Ptr<T> GetFunctor()
        {
            for (auto& functor : m_material->GetAsset()->GetMaterialFunctors())
            {
                auto convertedFunctor = azdynamic_cast<T*>(functor.get());
                if (convertedFunctor != nullptr)
                {
                    return convertedFunctor;
                    // intensity = emissiveFunctor->GetProcessedValue(intensity, unit);
                }
            }
            return nullptr;
        }

        AZ::Name GetEnumValueName(const AZ::Name& propertyName, const AZ::Name defaultValueName)
        {
            auto propertyIndex = m_material->FindPropertyIndex(propertyName);
            if (propertyIndex.IsValid())
            {
                uint32_t enumVal = m_material->GetPropertyValue<uint32_t>(propertyIndex);
                return m_material->GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex)->GetEnumName(enumVal);
            }
            return defaultValueName;
        }

    private:
        AZ::RPI::Material* m_material;
    };

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

        const RHI::Ptr<MaterialEntry>& MaterialManager::GetFallbackPBRMaterialEntry(const MeshInfoHandle handle) const
        {
            if (!m_isEnabled)
            {
                return m_emptyEntry;
            }

            if (m_materialData.size() > handle.GetIndex())
            {
                return m_materialData[handle.GetIndex()];
            }
            return m_emptyEntry;
        }

        void MaterialManager::Activate(RPI::Scene* scene)
        {
            UpdateFallbackPBRMaterialBuffer();

            if (auto* console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
            {
                console->GetCvarValue("r_fallbackPBRMaterialEnabled", m_isEnabled);
            }

            // We need to register the buffer in the SceneSrg even if we are disabled
            m_updateSceneSrgHandler = RPI::Scene::PrepareSceneSrgEvent::Handler(
                [this](RPI::ShaderResourceGroup* sceneSrg)
                {
                    sceneSrg->SetBufferView(m_fallbackPBRMaterialIndex, GetFallbackPBRMaterialBuffer()->GetBufferView());
                });
            scene->ConnectEvent(m_updateSceneSrgHandler);

            m_rpfp = scene->GetFeatureProcessor<ReflectionProbeFeatureProcessorInterface>();

            MeshInfoNotificationBus::Handler::BusConnect(scene->GetId());
        }

        void MaterialManager::Deactivate()
        {
            MeshInfoNotificationBus::Handler::BusDisconnect();
        }

        void MaterialManager::ConvertMaterial(RPI::Material* material, MaterialParameters& convertedMaterial)
        {
            if (material == nullptr)
            {
                return;
            }
            MaterialConversionUtil util{ material };

            convertedMaterial.m_baseColor = util.GetProperty<AZ::Color>(AZ_NAME_LITERAL("baseColor.color"));
            convertedMaterial.m_baseColor *= util.GetProperty<float>(AZ_NAME_LITERAL("baseColor.factor"), 1.0f);

            convertedMaterial.m_metallicFactor = util.GetProperty<float>(AZ_NAME_LITERAL("metallic.factor"), 0.5f);
            convertedMaterial.m_roughnessFactor = util.GetProperty<float>(AZ_NAME_LITERAL("roughness.factor"), 0.5f);

            bool emissiveEnable = util.GetProperty<bool>(AZ_NAME_LITERAL("emissive.enable"), false);
            if (emissiveEnable)
            {
                convertedMaterial.m_emissiveColor = util.GetProperty<AZ::Color>(AZ_NAME_LITERAL("emissive.color"));
                float intensity = util.GetProperty<float>(AZ_NAME_LITERAL("emissive.intensity"), 1.0f);
                uint32_t unit = util.GetProperty<uint32_t>(AZ_NAME_LITERAL("emissive.unit"), 0);
                auto emissiveFunctor = util.GetFunctor<ConvertEmissiveUnitFunctor>();
                if (emissiveFunctor)
                {
                    intensity = emissiveFunctor->GetProcessedValue(intensity, unit);
                    convertedMaterial.m_emissiveColor *= intensity;
                }
                else
                {
                    AZ_WarningOnce(
                        "MaterialManager",
                        false,
                        "Could not find ConvertEmissiveUnitFunctor for material %s",
                        material->GetAsset()->GetId().ToFixedString().c_str());
                }
            }

            convertedMaterial.m_baseColorImageView =
                util.GetProperty<RHI::Ptr<const RHI::ImageView>>(AZ_NAME_LITERAL("baseColor.textureMap"));
            convertedMaterial.m_normalImageView = util.GetProperty<RHI::Ptr<const RHI::ImageView>>(AZ_NAME_LITERAL("normal.textureMap"));
            convertedMaterial.m_metallicImageView =
                util.GetProperty<RHI::Ptr<const RHI::ImageView>>(AZ_NAME_LITERAL("metallic.textureMap"));
            convertedMaterial.m_roughnessImageView =
                util.GetProperty<RHI::Ptr<const RHI::ImageView>>(AZ_NAME_LITERAL("roughness.textureMap"));
            convertedMaterial.m_emissiveImageView =
                util.GetProperty<RHI::Ptr<const RHI::ImageView>>(AZ_NAME_LITERAL("emissive.textureMap"));

            auto irradianceColorSource =
                util.GetEnumValueName(AZ_NAME_LITERAL("irradiance.irradianceColorSource"), AZ_NAME_LITERAL("Manual"));
            if (irradianceColorSource == AZ_NAME_LITERAL("Manual"))
            {
                auto propertyNames = AZStd::array{ AZ_NAME_LITERAL("irradiance.manualColor"), AZ_NAME_LITERAL("irradiance.color") };
                convertedMaterial.m_irradianceColor = util.GetProperty<AZ::Color>(propertyNames, AZ::Colors::White);
            }
            else if (irradianceColorSource == AZ_NAME_LITERAL("BaseColorTint"))
            {
                convertedMaterial.m_irradianceColor = convertedMaterial.m_baseColor;
            }
            else if (irradianceColorSource == AZ_NAME_LITERAL("BaseColor"))
            {
                // if we can't find the useTexture - switch, assume we want to use a texture if it's assigned
                bool useTexture = util.GetProperty(AZ_NAME_LITERAL("baseColor.useTexture"), true);
                Data::Instance<RPI::Image> baseColorImage =
                    util.GetProperty<Data::Instance<RPI::Image>>(AZ_NAME_LITERAL("baseColor.textureMap"));
                if (useTexture && baseColorImage)
                {
                    auto baseColorStreamingImg = azdynamic_cast<RPI::StreamingImage*>(baseColorImage.get());
                    if (baseColorStreamingImg)
                    {
                        // Note: there are quite a few hidden assumptions in using the average
                        // texture color. For instance, (1) it assumes that every texel in the
                        // texture actually gets mapped to the surface (or non-mapped regions are
                        // colored with a meaningful 'average' color, or have zero opacity); (2) it
                        // assumes that the mapping from uv space to the mesh surface is
                        // (approximately) area-preserving to get a properly weighted average; and
                        // mostly, (3) it assumes that a single 'average color' is a meaningful
                        // characterisation of the full material.
                        Color avgColor = baseColorStreamingImg->GetAverageColor();

                        // We do a simple 'multiply' blend with the base color
                        // Note: other blend modes are currently not supported
                        convertedMaterial.m_irradianceColor = avgColor * convertedMaterial.m_baseColor;
                    }
                    else
                    {
                        AZ_Warning(
                            "MeshFeatureProcessor",
                            false,
                            "Using BaseColor as irradianceColorSource "
                            "is currently only supported for textures of type StreamingImage");
                        // Default to the flat base color
                        convertedMaterial.m_irradianceColor = convertedMaterial.m_baseColor;
                    }
                }
                else
                {
                    // No texture, simply copy the baseColor
                    convertedMaterial.m_irradianceColor = convertedMaterial.m_baseColor;
                }
            }
            else
            {
                AZ_Warning(
                    "MaterialManager",
                    false,
                    "Unknown irradianceColorSource value: %s, defaulting to white",
                    irradianceColorSource.GetCStr());
                convertedMaterial.m_irradianceColor = AZ::Colors::White;
            }

            // Overall scale factor
            float irradianceScale = util.GetProperty(AZ_NAME_LITERAL("irradiance.factor"), 1.0f);
            convertedMaterial.m_irradianceColor *= irradianceScale;

            // set the transparency from the material opacity factor
            auto opacityMode = util.GetEnumValueName(AZ_NAME_LITERAL("opacity.mode"), AZ_NAME_LITERAL("Opaque"));
            if (opacityMode != AZ_NAME_LITERAL("Opaque"))
            {
                convertedMaterial.m_irradianceColor.SetA(util.GetProperty(AZ_NAME_LITERAL("opacity.factor"), 1.0f));
            }
        }

        void MaterialManager::OnAcquireMeshInfoEntry(const MeshInfoHandle meshInfoHandle)
        {
            if (!m_isEnabled)
            {
                return;
            }
            if (m_materialData.size() <= meshInfoHandle.GetIndex())
            {
                m_materialData.resize(meshInfoHandle.GetIndex() + 1, nullptr);
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
                return;
            }

            auto& entry = m_materialData[meshInfoHandle.GetIndex()];

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
            if (m_materialData.size() <= meshInfoHandle.GetIndex())
            {
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
            if (m_materialData.size() > handle.GetIndex() && m_materialData[handle.GetIndex()] != nullptr)
            {
                m_bufferNeedsUpdate = updateFunction(m_materialData[handle.GetIndex()].get());
            }
        }

        void MaterialManager::UpdateReflectionProbes(
            const TransformServiceFeatureProcessorInterface::ObjectId& objectId, const Aabb& aabbWS)
        {
            if (!m_rpfp)
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