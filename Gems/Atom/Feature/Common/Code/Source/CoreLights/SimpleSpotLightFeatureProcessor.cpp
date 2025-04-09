/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/SimpleSpotLightFeatureProcessor.h>
#include <CoreLights/SpotLightUtils.h>
#include <Mesh/MeshFeatureProcessor.h>
#include <Atom/Feature/CoreLights/CoreLightsConstants.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RPI.Public/ColorManagement/TransformColor.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Math/Vector3.h>
#include <numeric>

//! If modified, ensure that r_maxVisibleSpotLights is equal to or lower than ENABLE_SIMPLE_SPOTLIGHTS_CAP which is the limit set by the shader on GPU.
AZ_CVAR(int, r_maxVisibleSpotLights, -1, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Maximum number of visible spot lights to use when culling is not available. -1 means no limit");

namespace AZ
{
    namespace Render
    {
        void SimpleSpotLightFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto * serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<SimpleSpotLightFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        SimpleSpotLightFeatureProcessor::SimpleSpotLightFeatureProcessor()
            : SimpleSpotLightFeatureProcessorInterface()
        {
        }

        void SimpleSpotLightFeatureProcessor::Activate()
        {
            GpuBufferHandler::Descriptor desc;
            desc.m_bufferName = "SimpleSpotLightBuffer";
            desc.m_bufferSrgName = "m_simpleSpotLights";
            desc.m_elementCountSrgName = "m_simpleSpotLightCount";
            desc.m_elementSize = sizeof(SimpleSpotLightData);
            desc.m_srgLayout = RPI::RPISystemInterface::Get()->GetViewSrgLayout().get();

            m_lightBufferHandler = GpuBufferHandler(desc);
            m_shadowFeatureProcessor = GetParentScene()->GetFeatureProcessor<ProjectedShadowFeatureProcessor>();
            MeshFeatureProcessor* meshFeatureProcessor = GetParentScene()->GetFeatureProcessor<MeshFeatureProcessor>();
            if (meshFeatureProcessor)
            {
                m_lightMeshFlag = meshFeatureProcessor->GetShaderOptionFlagRegistry()->AcquireTag(AZ::Name("o_enableSimpleSpotLights"));
                m_shadowMeshFlag = meshFeatureProcessor->GetShaderOptionFlagRegistry()->AcquireTag(AZ::Name("o_enableSimpleSpotLightShadows"));
            }
            EnableSceneNotification();
        }

        void SimpleSpotLightFeatureProcessor::Deactivate()
        {
            DisableSceneNotification();
            m_lightData.Clear();
            m_lightBufferHandler.Release();
            for (auto& handler : m_visibleSpotLightsBufferHandlers)
            {
                handler.Release();
            }
            m_visibleSpotLightsBufferHandlers.clear();
        }

        SimpleSpotLightFeatureProcessor::LightHandle SimpleSpotLightFeatureProcessor::AcquireLight()
        {
            uint16_t id = m_lightData.GetFreeSlotIndex();

            if (id == IndexedDataVector<SimpleSpotLightData>::NoFreeSlot)
            {
                return LightHandle::Null;
            }
            else
            {
                m_deviceBufferNeedsUpdate = true;
                m_goboArrayChanged = true;
                return LightHandle(id);
            }
        }

        bool SimpleSpotLightFeatureProcessor::ReleaseLight(LightHandle& handle)
        {
            if (handle.IsValid())
            {
                SpotLightUtils::ShadowId shadowId = SpotLightUtils::ShadowId(m_lightData.GetData<0>(handle.GetIndex()).m_shadowIndex);
                if (shadowId.IsValid())
                {
                    m_shadowFeatureProcessor->ReleaseShadow(shadowId);
                }
                m_lightData.RemoveIndex(handle.GetIndex());
                m_deviceBufferNeedsUpdate = true;
                m_goboArrayChanged = true;
                handle.Reset();
                return true;
            }
            return false;
        }

        SimpleSpotLightFeatureProcessor::LightHandle SimpleSpotLightFeatureProcessor::CloneLight(LightHandle sourceLightHandle)
        {
            AZ_Assert(sourceLightHandle.IsValid(), "Invalid LightHandle passed to SimpleSpotLightFeatureProcessor::CloneLight().");

            LightHandle handle = AcquireLight();
            if (handle.IsValid())
            {
                // Get a reference to the new light
                auto& light = m_lightData.GetData<0>(handle.GetIndex());
                // Copy data from the source light on top of it.
                light = m_lightData.GetData<0>(sourceLightHandle.GetIndex());
                m_lightData.GetData<1>(handle.GetIndex()) = m_lightData.GetData<1>(sourceLightHandle.GetIndex());

                SpotLightUtils::ShadowId shadowId = SpotLightUtils::ShadowId(light.m_shadowIndex);
                if (shadowId.IsValid())
                {
                    // Since the source light has a valid shadow, a new shadow must be generated for the cloned light.
                    ProjectedShadowFeatureProcessorInterface::ProjectedShadowDescriptor originalDesc =
                        m_shadowFeatureProcessor->GetShadowProperties(shadowId);
                    SpotLightUtils::ShadowId cloneShadow = m_shadowFeatureProcessor->AcquireShadow();
                    light.m_shadowIndex = cloneShadow.GetIndex();
                    m_shadowFeatureProcessor->SetShadowProperties(cloneShadow, originalDesc);
                }
                m_deviceBufferNeedsUpdate = true;
            }
            return handle;
        }

        void SimpleSpotLightFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "SimpleSpotLightFeatureProcessor: Simulate");
            AZ_UNUSED(packet);

            if (m_deviceBufferNeedsUpdate)
            {
                if (m_goboArrayChanged)
                {
                    // collect all gobo textures and assign index for each spot light
                    AZStd::unordered_map<AZ::Data::Instance<AZ::RPI::Image>, uint32_t> gobosTextures;
                    m_goboTextures.clear();

                    m_lightData.ForEach([&](uint16_t idx) -> bool
                        {
                            auto& lightData = m_lightData.GetData<0>(idx);
                            auto image = m_lightData.GetData<1>(idx).m_goboTexture;
                            if (image)
                            {
                                if (gobosTextures.find(image) == gobosTextures.end())
                                {
                                    uint32_t index = aznumeric_cast<uint32_t>(gobosTextures.size());
                                    gobosTextures[image] = index;
                                    m_goboTextures.push_back(image);
                                }
                                lightData.m_goboTextureIndex = gobosTextures[image];
                            }
                            else
                            {
                                lightData.m_goboTextureIndex = MaxGoboTextureCount;
                            }
                            return true; // continue to next light
                        });

                    m_goboArrayChanged = false;
                }

                m_lightBufferHandler.UpdateBuffer(m_lightData.GetDataVector<0>());
                m_deviceBufferNeedsUpdate = false;
            }

            if (r_enablePerMeshShaderOptionFlags)
            {
                const uint32_t lightAndShadow = m_lightMeshFlag.GetIndex() | m_shadowMeshFlag.GetIndex();
                const uint32_t lightOnly = m_lightMeshFlag.GetIndex();

                AZStd::vector<MeshCommon::BoundsVariant> lightsWithShadow;
                AZStd::vector<MeshCommon::BoundsVariant> lightsWithoutShadow;

                m_lightData.ForEach([&](uint16_t idx) -> bool
                    {
                        const auto& lightData = m_lightData.GetData<0>(idx);
                        const auto& extraData = m_lightData.GetData<1>(idx);

                        SpotLightUtils::ShadowId shadowId = SpotLightUtils::ShadowId(lightData.m_shadowIndex);
                        if (shadowId.IsValid())
                        {
                            lightsWithShadow.push_back(extraData.m_boundsVariant);
                        }
                        else
                        {
                            lightsWithoutShadow.push_back(extraData.m_boundsVariant);
                        }
                        return true; // continue to next light
                    });

                MeshCommon::MarkMeshesWithFlag(GetParentScene(), AZStd::span(lightsWithoutShadow), lightOnly);
                MeshCommon::MarkMeshesWithFlag(GetParentScene(), AZStd::span(lightsWithShadow), lightAndShadow);
            }
        }

        void SimpleSpotLightFeatureProcessor::Render(const SimpleSpotLightFeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "SimpleSpotLightFeatureProcessor: Render");
            m_visibleSpotLightsBufferUsedCount = 0;

            uint64_t count = m_goboTextures.size();
            if (count > MaxGoboTextureCount)
            {
                AZ_WarningOnce("SimpleSpotLight", false, "There are more than %d (MaxGoboTextureCount) gobo textures used in the level.", MaxGoboTextureCount);
                count = MaxGoboTextureCount;
            }

            for (const RPI::ViewPtr& view : packet.m_views)
            {
                m_lightBufferHandler.UpdateSrg(view->GetShaderResourceGroup().get());
                if (count > 0)
                {
                    view->GetShaderResourceGroup()->SetImageArray(m_goboTexturesIndex, AZStd::span(m_goboTextures.begin(), m_goboTextures.begin() + count));
                    
                }
                CullLights(view);
            }
        }

        void SimpleSpotLightFeatureProcessor::SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightRgbIntensity)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimpleSpotLightFeatureProcessor::SetRgbIntensity().");

            auto transformedColor = AZ::RPI::TransformColor(lightRgbIntensity, AZ::RPI::ColorSpaceId::LinearSRGB, AZ::RPI::ColorSpaceId::ACEScg);

            AZStd::array<float, 3>& rgbIntensity = m_lightData.GetData<0>(handle.GetIndex()).m_rgbIntensity;
            rgbIntensity[0] = transformedColor.GetR();
            rgbIntensity[1] = transformedColor.GetG();
            rgbIntensity[2] = transformedColor.GetB();

            m_deviceBufferNeedsUpdate = true;
        }

        void SimpleSpotLightFeatureProcessor::SetTransform(LightHandle handle, const AZ::Transform& transform)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimpleSpotLightFeatureProcessor::SetTransform().");

            m_lightData.GetData<1>(handle.GetIndex()).m_transform = transform;

            auto newDirection = transform.GetBasisZ();
            auto newPosition = transform.GetTranslation();

            auto& lightData = m_lightData.GetData<0>(handle.GetIndex());

            AZStd::array<float, 3>& direction = lightData.m_direction;
            newDirection.GetNormalized().StoreToFloat3(direction.data());
            AZStd::array<float, 3>& position = lightData.m_position;
            newPosition.StoreToFloat3(position.data());

            UpdateBounds(handle);
            UpdateShadow(handle);
            UpdateViewProjectionMatrix(handle);

            m_deviceBufferNeedsUpdate = true;
        }

        void SimpleSpotLightFeatureProcessor::SetConeAngles(LightHandle handle, float innerRadians, float outerRadians)
        {
            auto& light = m_lightData.GetData<0>(handle.GetIndex());
            SpotLightUtils::ValidateAndSetConeAngles(light, innerRadians, outerRadians);

            UpdateShadow(handle);
            UpdateViewProjectionMatrix(handle);

            m_deviceBufferNeedsUpdate = true;
        }

        void SimpleSpotLightFeatureProcessor::SetAttenuationRadius(LightHandle handle, float attenuationRadius)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimpleSpotLightFeatureProcessor::SetAttenuationRadius().");

            attenuationRadius = AZStd::max<float>(attenuationRadius, 0.001f); // prevent divide by zero.
            auto& light = m_lightData.GetData<0>(handle.GetIndex());
            light.m_invAttenuationRadiusSquared = 1.0f / (attenuationRadius * attenuationRadius);

            UpdateBounds(handle);
            UpdateViewProjectionMatrix(handle);

            // Update the shadow near far planes if necessary
            SpotLightUtils::ShadowId shadowId = SpotLightUtils::ShadowId(light.m_shadowIndex);
            if (shadowId.IsValid())
            {
                m_shadowFeatureProcessor->SetNearFarPlanes(SpotLightUtils::ShadowId(light.m_shadowIndex), 0, attenuationRadius);
            }
            m_deviceBufferNeedsUpdate = true;
        }

        void SimpleSpotLightFeatureProcessor::SetAffectsGI(LightHandle handle, bool affectsGI)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimpleSpotLightFeatureProcessor::SetAffectsGI().");

            m_lightData.GetData<0>(handle.GetIndex()).m_affectsGI = affectsGI;
            m_deviceBufferNeedsUpdate = true;
        }

        void SimpleSpotLightFeatureProcessor::SetAffectsGIFactor(LightHandle handle, float affectsGIFactor)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimpleSpotLightFeatureProcessor::SetAffectsGIFactor().");

            m_lightData.GetData<0>(handle.GetIndex()).m_affectsGIFactor = affectsGIFactor;
            m_deviceBufferNeedsUpdate = true;
        }

        void SimpleSpotLightFeatureProcessor::SetGoboTexture(LightHandle handle, AZ::Data::Instance<AZ::RPI::Image> goboTexture)
        {
            m_lightData.GetData<1>(handle.GetIndex()).m_goboTexture = goboTexture;
            m_deviceBufferNeedsUpdate = true;
            m_goboArrayChanged = true;
        }

        void SimpleSpotLightFeatureProcessor::SetShadowsEnabled(LightHandle handle, bool enabled)
        {
            auto& light = m_lightData.GetData<0>(handle.GetIndex());
            SpotLightUtils::ShadowId shadowId = SpotLightUtils::ShadowId(light.m_shadowIndex);
            if (shadowId.IsValid() && enabled == false)
            {
                // Disable shadows
                m_shadowFeatureProcessor->ReleaseShadow(shadowId);
                shadowId.Reset();
                light.m_shadowIndex = shadowId.GetIndex();
                m_deviceBufferNeedsUpdate = true;
            }
            else if (shadowId.IsNull() && enabled == true)
            {
                // Enable shadows
                light.m_shadowIndex = m_shadowFeatureProcessor->AcquireShadow().GetIndex();

                // It's possible the cone angles aren't set, or are too wide for casting shadows. This makes sure they're set to reasonable
                // limits. This function expects radians, so the cos stored in the actual data needs to be undone.
                SpotLightUtils::ValidateAndSetConeAngles(light, acosf(light.m_cosInnerConeAngle), acosf(light.m_cosOuterConeAngle));

                UpdateShadow(handle);
                m_deviceBufferNeedsUpdate = true;
            }
        }

        void SimpleSpotLightFeatureProcessor::SetLightingChannelMask(LightHandle handle, uint32_t lightingChannelMask)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimpleSpotLightFeatureProcessor::SetLightingChannelMask().");

            m_lightData.GetData<0>(handle.GetIndex()).m_lightingChannelMask = lightingChannelMask;
            m_deviceBufferNeedsUpdate = true;
        }

        const Data::Instance<RPI::Buffer> SimpleSpotLightFeatureProcessor::GetLightBuffer() const
        {
            return m_lightBufferHandler.GetBuffer();
        }

        uint32_t SimpleSpotLightFeatureProcessor::GetLightCount() const
        {
            return m_lightBufferHandler.GetElementCount();
        }

        void SimpleSpotLightFeatureProcessor::UpdateBounds(LightHandle handle)
        {
            const SimpleSpotLightData& data = m_lightData.GetData<0>(handle.GetIndex());
            m_lightData.GetData<1>(handle.GetIndex()).m_boundsVariant = SpotLightUtils::BuildBounds(data);
        }

        void SimpleSpotLightFeatureProcessor::UpdateViewProjectionMatrix(LightHandle handle)
        {
            auto& lightData = m_lightData.GetData<0>(handle.GetIndex());

            AZ::Matrix4x4 viewToClipMatrix;
            float halfFov = acosf(lightData.m_cosOuterConeAngle);
            float attenuationRadius = LightCommon::GetRadiusFromInvRadiusSquared(lightData.m_invAttenuationRadiusSquared);
            attenuationRadius = AZStd::max(0.02f, attenuationRadius);
            MakePerspectiveFovMatrixRH(viewToClipMatrix, halfFov * 2.0f, 1.0f, 0.01f, attenuationRadius, false);

            auto& extraData = m_lightData.GetData<1>(handle.GetIndex());
            AZ::Matrix4x4 worldMatrix = AZ::Matrix4x4::CreateFromTransform(extraData.m_transform).GetInverseFast();
            AZ::Matrix4x4 worldToClip = viewToClipMatrix * worldMatrix;
            worldToClip.StoreToRowMajorFloat16(lightData.m_viewProjectionMatrix.data());
        }

        void SimpleSpotLightFeatureProcessor::UpdateShadow(LightHandle handle)
        {
            const auto& lightData = m_lightData.GetData<0>(handle.GetIndex());
            SpotLightUtils::ShadowId shadowId = SpotLightUtils::ShadowId(lightData.m_shadowIndex);
            if (shadowId.IsNull())
            {
                // Early out if shadows are disabled.
                return;
            }

            ProjectedShadowFeatureProcessorInterface::ProjectedShadowDescriptor desc =
                m_shadowFeatureProcessor->GetShadowProperties(shadowId);

            SpotLightUtils::UpdateShadowDescriptor(lightData, desc);
            m_shadowFeatureProcessor->SetShadowProperties(shadowId, desc);
        }

        template<typename Functor, typename ParamType>
        void SimpleSpotLightFeatureProcessor::SetShadowSetting(LightHandle handle, Functor&& functor, ParamType&& param)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimpleSpotLightFeatureProcessor::SetShadowSetting().");

            auto& light = m_lightData.GetData<0>(handle.GetIndex());
            SpotLightUtils::ShadowId shadowId = SpotLightUtils::ShadowId(light.m_shadowIndex);

            AZ_Assert(shadowId.IsValid(), "Attempting to set a shadow property when shadows are not enabled.");
            if (shadowId.IsValid())
            {
                AZStd::invoke(AZStd::forward<Functor>(functor), m_shadowFeatureProcessor, shadowId, AZStd::forward<ParamType>(param));
            }
        }

        void SimpleSpotLightFeatureProcessor::SetShadowBias(LightHandle handle, float bias)
        {
            SetShadowSetting(handle, &ProjectedShadowFeatureProcessor::SetShadowBias, bias);
        }

        void SimpleSpotLightFeatureProcessor::SetNormalShadowBias(LightHandle handle, float bias)
        {
            SetShadowSetting(handle, &ProjectedShadowFeatureProcessor::SetNormalShadowBias, bias);
        }

        void SimpleSpotLightFeatureProcessor::SetShadowmapMaxResolution(LightHandle handle, ShadowmapSize shadowmapSize)
        {
            SetShadowSetting(handle, &ProjectedShadowFeatureProcessor::SetShadowmapMaxResolution, shadowmapSize);
        }

        void SimpleSpotLightFeatureProcessor::SetShadowFilterMethod(LightHandle handle, ShadowFilterMethod method)
        {
            SetShadowSetting(handle, &ProjectedShadowFeatureProcessor::SetShadowFilterMethod, method);
        }

        void SimpleSpotLightFeatureProcessor::SetFilteringSampleCount(LightHandle handle, uint16_t count)
        {
            SetShadowSetting(handle, &ProjectedShadowFeatureProcessor::SetFilteringSampleCount, count);
        }

        void SimpleSpotLightFeatureProcessor::SetEsmExponent(LightHandle handle, float exponent)
        {
            SetShadowSetting(handle, &ProjectedShadowFeatureProcessor::SetEsmExponent, exponent);
        }

        void SimpleSpotLightFeatureProcessor::SetUseCachedShadows(LightHandle handle, bool useCachedShadows)
        {
            SetShadowSetting(handle, &ProjectedShadowFeatureProcessor::SetUseCachedShadows, useCachedShadows);
        }

        void SimpleSpotLightFeatureProcessor::OnRenderPipelinePersistentViewChanged(
            RPI::RenderPipeline* renderPipeline,
            [[maybe_unused]] RPI::PipelineViewTag viewTag,
            RPI::ViewPtr newView,
            RPI::ViewPtr previousView)
        {
            Render::LightCommon::CacheCPUCulledPipelineInfo(
                renderPipeline,
                newView, previousView, m_cpuCulledPipelinesPerView);
        }

        void SimpleSpotLightFeatureProcessor::CullLights(const RPI::ViewPtr& view)
        {
            if (!AZ::RHI::CheckBitsAll(view->GetUsageFlags(), RPI::View::UsageFlags::UsageCamera) ||
                !Render::LightCommon::NeedsCPUCulling(view, m_cpuCulledPipelinesPerView))
            {
                return;
            }

            const auto& dataVector = m_lightData.GetDataVector<0>();
            size_t numVisibleLights =
                r_maxVisibleSpotLights < 0 ? dataVector.size() : AZStd::min(dataVector.size(), static_cast<size_t>(r_maxVisibleSpotLights));
            AZStd::vector<uint32_t> sortedLights(dataVector.size());
            // Initialize with all the simple spot light indices
            std::iota(sortedLights.begin(), sortedLights.end(), 0);
            // Only sort if we are going to limit the number of visible decals
            if (numVisibleLights < dataVector.size())
            {
                AZ::Vector3 viewPos = view->GetViewToWorldMatrix().GetTranslation();
                AZStd::sort(
                    sortedLights.begin(),
                    sortedLights.end(),
                    [&dataVector, &viewPos](uint32_t lhs, uint32_t rhs)
                    {
                        float d1 = (AZ::Vector3::CreateFromFloat3(dataVector[lhs].m_position.data()) - viewPos).GetLengthSq();
                        float d2 = (AZ::Vector3::CreateFromFloat3(dataVector[rhs].m_position.data()) - viewPos).GetLengthSq();
                        return d1 < d2;
                    });
            }

            const AZ::Frustum viewFrustum = AZ::Frustum::CreateFromMatrixColumnMajor(view->GetWorldToClipMatrix());
            AZStd::vector<uint32_t> visibilityBuffer;
            visibilityBuffer.reserve(numVisibleLights);
            for (uint32_t i = 0; i < sortedLights.size() && visibilityBuffer.size() < numVisibleLights; ++i)
            {
                uint32_t dataIndex = sortedLights[i];
                const auto& lightData = dataVector[dataIndex];
                float radius = LightCommon::GetRadiusFromInvRadiusSquared(abs(lightData.m_invAttenuationRadiusSquared));
                AZ::Vector3 position = AZ::Vector3::CreateFromFloat3(lightData.m_position.data());
                
                // Do the actual culling per light and only add the indices for the visible ones.
                // We cull based on a sphere around the whole spot light as that is easier and faster and good enough.
                // This can be improved by doing frustum-frustum and frustum-hemisphere intersection.
                if (AZ::ShapeIntersection::Overlaps(viewFrustum, AZ::Sphere(position, radius)))
                {
                    visibilityBuffer.push_back(dataIndex);
                }
            }

            //Create the appropriate buffer handlers for the visibility data
            Render::LightCommon::UpdateVisibleBuffers(
                "SimpleSpotLightVisibilityBuffer",
                "m_visibleSimpleSpotLightIndices",
                "m_visibleSimpleSpotLightCount",
                m_visibleSpotLightsBufferUsedCount,
                m_visibleSpotLightsBufferHandlers);

            // Update buffer and View SRG
            GpuBufferHandler& bufferHandler = m_visibleSpotLightsBufferHandlers[m_visibleSpotLightsBufferUsedCount++];
            bufferHandler.UpdateBuffer(visibilityBuffer);
            bufferHandler.UpdateSrg(view->GetShaderResourceGroup().get());
        }
    } // namespace Render
} // namespace AZ
