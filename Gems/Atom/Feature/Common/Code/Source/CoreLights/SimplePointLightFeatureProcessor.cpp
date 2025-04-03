/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/SimplePointLightFeatureProcessor.h>
#include <CoreLights/LightCommon.h>
#include <CoreLights/SpotLightUtils.h>
#include <Mesh/MeshFeatureProcessor.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RPI.Public/ColorManagement/TransformColor.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <numeric>

//! If modified, ensure that r_maxVisiblePointLights is equal to or lower than ENABLE_SIMPLE_POINTLIGHTS_CAP which is the limit set by the shader on GPU.
AZ_CVAR(int, r_maxVisiblePointLights, -1, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Maximum number of visible point lights to use when culling is not available. -1 means no limit");

namespace AZ
{
    namespace Render
    {
        void SimplePointLightFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto * serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<SimplePointLightFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        SimplePointLightFeatureProcessor::SimplePointLightFeatureProcessor()
            : SimplePointLightFeatureProcessorInterface()
        {
        }

        void SimplePointLightFeatureProcessor::Activate()
        {
            GpuBufferHandler::Descriptor desc;
            desc.m_bufferName = "SimplePointLightBuffer";
            desc.m_bufferSrgName = "m_simplePointLights";
            desc.m_elementCountSrgName = "m_simplePointLightCount";
            desc.m_elementSize = sizeof(SimplePointLightData);
            desc.m_srgLayout = RPI::RPISystemInterface::Get()->GetViewSrgLayout().get();

            m_lightBufferHandler = GpuBufferHandler(desc);
            
            MeshFeatureProcessor* meshFeatureProcessor = GetParentScene()->GetFeatureProcessor<MeshFeatureProcessor>();
            if (meshFeatureProcessor)
            {
                m_lightMeshFlag = meshFeatureProcessor->GetShaderOptionFlagRegistry()->AcquireTag(AZ::Name("o_enableSimplePointLights"));
            }
            EnableSceneNotification();
        }

        void SimplePointLightFeatureProcessor::Deactivate()
        {
            DisableSceneNotification();
            m_lightData.Clear();
            m_lightBufferHandler.Release();
            for (auto& handler : m_visiblePointLightsBufferHandlers)
            {
                handler.Release();
            }
            m_visiblePointLightsBufferHandlers.clear();
        }

        SimplePointLightFeatureProcessor::LightHandle SimplePointLightFeatureProcessor::AcquireLight()
        {
            uint16_t id = m_lightData.GetFreeSlotIndex();

            if (id == IndexedDataVector<SimplePointLightData>::NoFreeSlot)
            {
                return LightHandle::Null;
            }
            else
            {
                m_deviceBufferNeedsUpdate = true;
                return LightHandle(id);
            }
        }

        bool SimplePointLightFeatureProcessor::ReleaseLight(LightHandle& handle)
        {
            if (handle.IsValid())
            {
                m_lightData.RemoveIndex(handle.GetIndex());
                m_deviceBufferNeedsUpdate = true;
                handle.Reset();
                return true;
            }
            return false;
        }

        SimplePointLightFeatureProcessor::LightHandle SimplePointLightFeatureProcessor::CloneLight(LightHandle sourceLightHandle)
        {
            AZ_Assert(sourceLightHandle.IsValid(), "Invalid LightHandle passed to SimplePointLightFeatureProcessor::CloneLight().");

            LightHandle handle = AcquireLight();
            if (handle.IsValid())
            {
                //  Get a reference to the new light
                auto& light = m_lightData.GetData<0>(handle.GetIndex());
                // Copy data from the source light on top of it.
                light = m_lightData.GetData<0>(sourceLightHandle.GetIndex());
                m_lightData.GetData<1>(handle.GetIndex()) = m_lightData.GetData<1>(sourceLightHandle.GetIndex());

                m_deviceBufferNeedsUpdate = true;
            }
            return handle;
        }

        void SimplePointLightFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "SimplePointLightFeatureProcessor: Simulate");
            AZ_UNUSED(packet);

            if (m_deviceBufferNeedsUpdate)
            {
                m_lightBufferHandler.UpdateBuffer(m_lightData.GetDataVector<0>());
                m_deviceBufferNeedsUpdate = false;
            }
            
            if (r_enablePerMeshShaderOptionFlags)
            {
                auto noShadow = [&](const AZ::Sphere& sphere) -> bool
                {
                    LightHandle::IndexType index = m_lightData.GetIndexForData<1>(&sphere);
                    return index != IndexedDataVector<int>::NoFreeSlot;
                };

                // Mark meshes that have simple point lights
                MeshCommon::MarkMeshesWithFlag(GetParentScene(), AZStd::span(m_lightData.GetDataVector<1>()), m_lightMeshFlag.GetIndex(), noShadow);
            }
        }

        void SimplePointLightFeatureProcessor::Render(const SimplePointLightFeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "SimplePointLightFeatureProcessor: Render");
            m_visiblePointLightsBufferUsedCount = 0;
            for (const RPI::ViewPtr& view : packet.m_views)
            {
                m_lightBufferHandler.UpdateSrg(view->GetShaderResourceGroup().get());
                CullLights(view);
            }
        }

        void SimplePointLightFeatureProcessor::SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightRgbIntensity)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimplePointLightFeatureProcessor::SetRgbIntensity().");

            auto transformedColor = AZ::RPI::TransformColor(lightRgbIntensity, AZ::RPI::ColorSpaceId::LinearSRGB, AZ::RPI::ColorSpaceId::ACEScg);

            AZStd::array<float, 3>& rgbIntensity = m_lightData.GetData<0>(handle.GetIndex()).m_rgbIntensity;
            rgbIntensity[0] = transformedColor.GetR();
            rgbIntensity[1] = transformedColor.GetG();
            rgbIntensity[2] = transformedColor.GetB();

            m_deviceBufferNeedsUpdate = true;
        }

        void SimplePointLightFeatureProcessor::SetPosition(LightHandle handle, const AZ::Vector3& lightPosition)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimplePointLightFeatureProcessor::SetPosition().");

            AZStd::array<float, 3>& position = m_lightData.GetData<0>(handle.GetIndex()).m_position;
            lightPosition.StoreToFloat3(position.data());
            m_lightData.GetData<1>(handle.GetIndex()).SetCenter(lightPosition);
            m_deviceBufferNeedsUpdate = true;
        }

        void SimplePointLightFeatureProcessor::SetAttenuationRadius(LightHandle handle, float attenuationRadius)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimplePointLightFeatureProcessor::SetAttenuationRadius().");

            attenuationRadius = AZStd::max<float>(attenuationRadius, 0.001f); // prevent divide by zero.
            m_lightData.GetData<0>(handle.GetIndex()).m_invAttenuationRadiusSquared = 1.0f / (attenuationRadius * attenuationRadius);
            m_lightData.GetData<1>(handle.GetIndex()).SetRadius(attenuationRadius);
            m_deviceBufferNeedsUpdate = true;
        }

        void SimplePointLightFeatureProcessor::SetAffectsGI(LightHandle handle, bool affectsGI)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimplePointLightFeatureProcessor::SetAffectsGI().");

            m_lightData.GetData<0>(handle.GetIndex()).m_affectsGI = affectsGI;
            m_deviceBufferNeedsUpdate = true;
        }

        void SimplePointLightFeatureProcessor::SetAffectsGIFactor(LightHandle handle, float affectsGIFactor)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimplePointLightFeatureProcessor::SetAffectsGIFactor().");

            m_lightData.GetData<0>(handle.GetIndex()).m_affectsGIFactor = affectsGIFactor;
            m_deviceBufferNeedsUpdate = true;
        }

        void SimplePointLightFeatureProcessor::SetLightingChannelMask(LightHandle handle, uint32_t lightingChannelMask)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimplePointLightFeatureProcessor::SetLightingChannelMask().");

            m_lightData.GetData<0>(handle.GetIndex()).m_lightingChannelMask = lightingChannelMask;
            m_deviceBufferNeedsUpdate = true;
        }

        const Data::Instance<RPI::Buffer> SimplePointLightFeatureProcessor::GetLightBuffer() const
        {
            return m_lightBufferHandler.GetBuffer();
        }

        uint32_t SimplePointLightFeatureProcessor::GetLightCount() const
        {
            return m_lightBufferHandler.GetElementCount();
        }

        void SimplePointLightFeatureProcessor::OnRenderPipelinePersistentViewChanged(
            RPI::RenderPipeline* renderPipeline,
            [[maybe_unused]] RPI::PipelineViewTag viewTag,
            RPI::ViewPtr newView,
            RPI::ViewPtr previousView)
        {
            Render::LightCommon::CacheCPUCulledPipelineInfo(renderPipeline, newView, previousView, m_cpuCulledPipelinesPerView);
        }

        void SimplePointLightFeatureProcessor::CullLights(const RPI::ViewPtr& view)
        {
            if (!AZ::RHI::CheckBitsAll(view->GetUsageFlags(), RPI::View::UsageFlags::UsageCamera) ||
                !Render::LightCommon::NeedsCPUCulling(view, m_cpuCulledPipelinesPerView))
            {
                return;
            }

            const auto& dataVector = m_lightData.GetDataVector<0>();
            const auto& dataBoundsVector = m_lightData.GetDataVector<1>();
            
            size_t numVisibleLights =
                r_maxVisiblePointLights < 0 ? dataVector.size() : AZStd::min(dataVector.size(), static_cast<size_t>(r_maxVisiblePointLights));
            AZStd::vector<uint32_t> sortedLights(dataVector.size());
            // Initialize with all the simple point light indices
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
                const auto& bound = dataBoundsVector[dataIndex];
                // Do the actual culling per light and only add the indices for the visible ones.
                if (AZ::ShapeIntersection::Overlaps(viewFrustum, bound))
                {
                    visibilityBuffer.push_back(dataIndex);
                }
            }

            // Create the appropriate buffer handlers for the visibility data
            Render::LightCommon::UpdateVisibleBuffers(
                "SimplePointLightVisibilityBuffer",
                "m_visibleSimplePointLightIndices",
                "m_visibleSimplePointLightCount",
                m_visiblePointLightsBufferUsedCount,
                m_visiblePointLightsBufferHandlers);

            // Update buffer and View SRG
            GpuBufferHandler& bufferHandler = m_visiblePointLightsBufferHandlers[m_visiblePointLightsBufferUsedCount++];
            bufferHandler.UpdateBuffer(visibilityBuffer);
            bufferHandler.UpdateSrg(view->GetShaderResourceGroup().get());
        }
    } // namespace Render
} // namespace AZ
