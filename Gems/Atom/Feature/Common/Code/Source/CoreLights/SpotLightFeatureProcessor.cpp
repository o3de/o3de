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

#include <CoreLights/Shadow.h>
#include <CoreLights/SpotLightFeatureProcessor.h>
#include <Math/GaussianMathFilter.h>

#include <AzCore/Debug/EventTrace.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>

#include <Atom/Feature/CoreLights/CoreLightsConstants.h>
#include <Atom/RHI/CpuProfiler.h>
#include <Atom/RHI/DrawList.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RPI.Public/ColorManagement/TransformColor.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Math/Matrix4x4.h>

namespace AZ
{
    namespace Render
    {
        namespace
        {
            static AZStd::array<float, 2> GetDepthUnprojectConstants(const RPI::ViewPtr view)
            {
                AZStd::array<float, 2> unprojectConstants;
                unprojectConstants[0] = view->GetViewToClipMatrix().GetRow(2).GetElement(2);
                unprojectConstants[1] = view->GetViewToClipMatrix().GetRow(2).GetElement(3);
                return unprojectConstants;
            }
        }


        void SpotLightFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<SpotLightFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        void SpotLightFeatureProcessor::Activate()
        {
            const RHI::ShaderResourceGroupLayout* viewSrgLayout = RPI::RPISystemInterface::Get()->GetViewSrgAsset()->GetLayout();

            GpuBufferHandler::Descriptor desc;

            desc.m_bufferName = "SpotLightBuffer";
            desc.m_bufferSrgName = "m_spotLights";
            desc.m_elementCountSrgName = "m_spotLightCount";
            desc.m_elementSize = sizeof(SpotLightData);
            desc.m_srgLayout = viewSrgLayout;

            m_lightBufferHandler = GpuBufferHandler(desc);

            desc.m_bufferName = "SpotLightShadowBuffer";
            desc.m_bufferSrgName = "m_spotLightShadows";
            desc.m_elementCountSrgName = "";
            desc.m_elementSize = sizeof(SpotLightShadowData);
            desc.m_srgLayout = viewSrgLayout;

            m_shadowBufferHandler = GpuBufferHandler(desc);

            desc.m_bufferName = "EsmParameterBuffer(Spot)";
            desc.m_bufferSrgName = "m_esmsSpot";
            desc.m_elementCountSrgName = "";
            desc.m_elementSize = sizeof(EsmShadowmapsPass::FilterParameter);
            desc.m_srgLayout = viewSrgLayout;

            m_esmParameterBufferHandler = GpuBufferHandler(desc);

            m_shadowmapAtlasSizeIndex = viewSrgLayout->FindShaderInputConstantIndex(Name("m_shadowmapAtlasSize"));
            m_invShadowmapAtlasSize = viewSrgLayout->FindShaderInputConstantIndex(Name("m_invShadowmapAtlasSize"));

            CachePasses();
            EnableSceneNotification();
        }

        void SpotLightFeatureProcessor::Deactivate()
        {
            DisableSceneNotification();

            m_spotLightData.Clear();
            m_lightBufferHandler.Release();

            m_shadowData.Clear();
            m_shadowBufferHandler.Release();

            m_esmParameterData.Clear();
            m_esmParameterBufferHandler.Release();

            for (EsmShadowmapsPass* esmPass : m_esmShadowmapsPasses)
            {
                esmPass->SetEnabledComputation(false);
            }
        }

        SpotLightFeatureProcessor::LightHandle SpotLightFeatureProcessor::AcquireLight()
        {
            const uint16_t index = m_spotLightData.GetFreeSlotIndex();
            const uint16_t propIndex = m_lightProperties.GetFreeSlotIndex();
            AZ_Assert(index == propIndex, "light index is illegal.");
            if (index == IndexedDataVector<SpotLightData>::NoFreeSlot)
            {
                return LightHandle::Null;
            }
            else
            {
                m_deviceBufferNeedsUpdate = true;
                const LightHandle handle(index);
                return handle;
            }
        }

        bool SpotLightFeatureProcessor::ReleaseLight(LightHandle& handle)
        {
            if (handle.IsValid())
            {
                CleanUpShadow(handle);
                m_spotLightData.RemoveIndex(handle.GetIndex());
                m_lightProperties.RemoveIndex(handle.GetIndex());

                m_deviceBufferNeedsUpdate = true;
                handle.Reset();
                return true;
            }
            return false;
        }

        SpotLightFeatureProcessor::LightHandle SpotLightFeatureProcessor::CloneLight(LightHandle sourceLightHandle)
        {
            AZ_Assert(sourceLightHandle.IsValid(), "Invalid LightHandle passed to SpotLightFeatureProcessor::CloneLight().");

            LightHandle handle = AcquireLight();
            if (handle.IsValid())
            {
                m_spotLightData.GetData(handle.GetIndex()) = m_spotLightData.GetData(sourceLightHandle.GetIndex());
                m_deviceBufferNeedsUpdate = true;
            }
            return handle;
        }

        void SpotLightFeatureProcessor::OnRenderPipelinePassesChanged([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
        {
            CachePasses();
        }

        void SpotLightFeatureProcessor::OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline)
        {
            CachePasses();
        }

        void SpotLightFeatureProcessor::OnRenderPipelineRemoved([[maybe_unused]] RPI::RenderPipeline* pipeline)
        {
            CachePasses();
        }

        void SpotLightFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_ATOM_PROFILE_FUNCTION("RPI", "SpotLightFeatureProcessor: Simulate");
            AZ_UNUSED(packet);

            UpdateShadowmapViews();
            SetShadowParameterToShadowData();

            if (m_shadowmapPassNeedsUpdate)
            {
                AZStd::vector<SpotLightShadowmapsPass::ShadowmapSizeWithIndices> shadowmapSizes(m_shadowProperties.size());
                for (auto& it : m_shadowProperties)
                {
                    const int32_t shadowIndexInSrgSigned = m_spotLightData.GetData(it.first.GetIndex()).m_shadowIndex;
                    AZ_Assert(shadowIndexInSrgSigned >= 0, "Shadow index in SRG is illegal.");
                    const uint16_t shadowIndexInSrg = aznumeric_cast<uint16_t>(shadowIndexInSrgSigned);
                    it.second.m_viewTagIndex = shadowIndexInSrg;
                    SpotLightShadowmapsPass::ShadowmapSizeWithIndices& sizeWithIndices = shadowmapSizes[shadowIndexInSrg];
                    sizeWithIndices.m_size = static_cast<ShadowmapSize>(m_shadowData.GetData(it.second.m_shadowHandle.GetIndex()).m_shadowmapSize);
                    sizeWithIndices.m_shadowIndexInSrg = shadowIndexInSrg;
                }
                for (SpotLightShadowmapsPass* shadowPass : m_spotLightShadowmapsPasses)
                {
                    shadowPass->UpdateShadowmapSizes(shadowmapSizes);
                }
                for (EsmShadowmapsPass* esmPass : m_esmShadowmapsPasses)
                {
                    esmPass->QueueForBuildAttachments();
                }

                for (const SpotLightShadowmapsPass* shadowPass : m_spotLightShadowmapsPasses)
                {
                    for (const auto& it : m_shadowProperties)
                    {
                        const int32_t shadowIndexInSrg = m_spotLightData.GetData(it.first.GetIndex()).m_shadowIndex;
                        if (shadowIndexInSrg >= 0)
                        {
                            const ShadowmapAtlas::Origin origin = shadowPass->GetOriginInAtlas(aznumeric_cast<uint16_t>(shadowIndexInSrg));
                            SpotLightShadowData& shadow = m_shadowData.GetData(it.second.m_shadowHandle.GetIndex());
                            shadow.m_shadowmapArraySlice = origin.m_arraySlice;
                            shadow.m_shadowmapOriginInSlice = origin.m_originInSlice;
                            m_deviceBufferNeedsUpdate = true;
                        }
                    }
                    break;
                }
                m_shadowmapPassNeedsUpdate = false;
            }

            // This has to be called after UpdateShadowmapSizes().
            UpdateFilterParameters();

            if (m_deviceBufferNeedsUpdate)
            {
                m_lightBufferHandler.UpdateBuffer(m_spotLightData.GetDataVector());
                m_shadowBufferHandler.UpdateBuffer(m_shadowData.GetDataVector());
                m_deviceBufferNeedsUpdate = false;
            }
        }

        void SpotLightFeatureProcessor::PrepareViews(const PrepareViewsPacket&, AZStd::vector<AZStd::pair<RPI::PipelineViewTag, RPI::ViewPtr>>& outViews)
        {
            for (SpotLightShadowmapsPass* pass : m_spotLightShadowmapsPasses)
            {
                RPI::RenderPipeline* renderPipeline = pass->GetRenderPipeline();
                if (renderPipeline)
                {
                    for (auto& it : m_shadowProperties)
                    {
                        SpotLightShadowData& shadow = m_shadowData.GetData(it.second.m_shadowHandle.GetIndex());
                        if (shadow.m_shadowmapSize == aznumeric_cast<uint32_t>(ShadowmapSize::None))
                        {
                            continue;
                        }

                        const RPI::PipelineViewTag& viewTag = pass->GetPipelineViewTagOfChild(it.second.m_viewTagIndex);
                        const RHI::DrawListMask drawListMask = renderPipeline->GetDrawListMask(viewTag);
                        if (it.second.m_shadowmapView->GetDrawListMask() != drawListMask)
                        {
                            it.second.m_shadowmapView->Reset();
                            it.second.m_shadowmapView->SetDrawListMask(drawListMask);
                        }

                        outViews.emplace_back(AZStd::make_pair(
                            viewTag,
                            it.second.m_shadowmapView));
                    }
                }
                break;
            }
        }

        void SpotLightFeatureProcessor::Render(const SpotLightFeatureProcessor::RenderPacket& packet)
        {
            AZ_ATOM_PROFILE_FUNCTION("RPI", "SpotLightFeatureProcessor: Render");

            for (const SpotLightShadowmapsPass* pass : m_spotLightShadowmapsPasses)
            {
                for (const RPI::ViewPtr& view : packet.m_views)
                {
                    if (view->GetUsageFlags() & RPI::View::UsageFlags::UsageCamera)
                    {
                        RPI::ShaderResourceGroup* srg = view->GetShaderResourceGroup().get();
                        srg->SetConstant(m_shadowmapAtlasSizeIndex, static_cast<float>(pass->GetShadowmapAtlasSize()));
                        const float invShadowmapSize = 1.0f / static_cast<float>(pass->GetShadowmapAtlasSize());
                        srg->SetConstant(m_invShadowmapAtlasSize, invShadowmapSize);                        

                        m_lightBufferHandler.UpdateSrg(srg);
                        m_shadowBufferHandler.UpdateSrg(srg);
                        m_esmParameterBufferHandler.UpdateSrg(srg);
                    }
                }
                break;
            }
        }

        void SpotLightFeatureProcessor::SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnit::Candela>& lightRgbIntensity)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SpotLightFeatureProcessor::SetRgbIntensity().");

            auto transformedColor = AZ::RPI::TransformColor(lightRgbIntensity, AZ::RPI::ColorSpaceId::LinearSRGB, AZ::RPI::ColorSpaceId::ACEScg);

            AZStd::array<float, 3>& rgbIntensity = m_spotLightData.GetData(handle.GetIndex()).m_rgbIntensity;
            rgbIntensity[0] = transformedColor.GetR();
            rgbIntensity[1] = transformedColor.GetG();
            rgbIntensity[2] = transformedColor.GetB();

            m_deviceBufferNeedsUpdate = true;
        }

        void SpotLightFeatureProcessor::SetPosition(LightHandle handle, const AZ::Vector3& lightPosition)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SpotLightFeatureProcessor::SetPosition().");

            SpotLightData& light = m_spotLightData.GetData(handle.GetIndex());
            lightPosition.StoreToFloat3(light.m_position.data());

            if (light.m_shadowIndex >= 0)
            {
                AZ_Assert(m_shadowProperties.find(handle) != m_shadowProperties.end(), "ShadowmapProperty is incorrect.");
                m_shadowProperties.at(handle).m_shadowmapViewNeedsUpdate = true;
                m_filterParameterNeedsUpdate = true;
            }
            m_deviceBufferNeedsUpdate = true;
        }

        void SpotLightFeatureProcessor::SetDirection(LightHandle handle, const AZ::Vector3& lightDirection)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SpotLightFeatureProcessor::SetDirection().");

            SpotLightData& light = m_spotLightData.GetData(handle.GetIndex());
            lightDirection.GetNormalized().StoreToFloat3(light.m_direction.data());

            if (light.m_shadowIndex >= 0)
            {
                AZ_Assert(m_shadowProperties.find(handle) != m_shadowProperties.end(), "ShadowmapProperty is incorrect.");
                m_shadowProperties.at(handle).m_shadowmapViewNeedsUpdate = true;
                m_filterParameterNeedsUpdate = true;
            }
            m_deviceBufferNeedsUpdate = true;
        }

        void SpotLightFeatureProcessor::SetBulbRadius(LightHandle handle, float bulbRadius)
        {
            SpotLightData& light = m_spotLightData.GetData(handle.GetIndex());
            light.m_bulbRadius = bulbRadius;
            UpdateBulbPositionOffset(light);

            if (light.m_shadowIndex >= 0)
            {
                auto itr = m_shadowProperties.find(handle);
                AZ_Assert(itr != m_shadowProperties.end(), "ShadowmapProperty is incorrect.");
                itr->second.m_shadowmapViewNeedsUpdate = true;
                m_filterParameterNeedsUpdate = true;
            }
            m_deviceBufferNeedsUpdate = true;
        }

        void SpotLightFeatureProcessor::SetConeAngles(LightHandle handle, float innerDegrees, float outerDegrees)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SpotLightFeatureProcessor::SetConeAngles().");
            SpotLightData& light = m_spotLightData.GetData(handle.GetIndex());

            if (light.m_shadowIndex < 0)
            {
                innerDegrees = AZStd::GetMin(innerDegrees, MaxSpotLightConeAngleDegree);
                outerDegrees = AZStd::GetMin(outerDegrees, MaxSpotLightConeAngleDegree);
            }
            else
            {
                innerDegrees = AZStd::GetMin(innerDegrees, MaxSpotLightConeAngleDegreeWithShadow);
                outerDegrees = AZStd::GetMin(outerDegrees, MaxSpotLightConeAngleDegreeWithShadow);

                AZ_Assert(m_shadowProperties.find(handle) != m_shadowProperties.end(), "ShadowmapProperty is incorrect.");
                m_shadowProperties[handle].m_shadowmapViewNeedsUpdate = true;
                m_filterParameterNeedsUpdate = true;
            }

            light.m_innerConeAngle = cosf(DegToRad(innerDegrees) * 0.5f);
            light.m_outerConeAngle = cosf(DegToRad(outerDegrees) * 0.5f);
            m_lightProperties.GetData(handle.GetIndex()).m_outerConeAngle = DegToRad(outerDegrees);
            UpdateBulbPositionOffset(light);
            m_deviceBufferNeedsUpdate = true;
        }

        void SpotLightFeatureProcessor::SetPenumbraBias(LightHandle handle, float penumbraBias)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SpotLightFeatureProcessor::SetPenumbraBias().");

            // Biases at 1.0 and -1.0 exactly can cause div by zero / inf in the shader, so clamp them just inside that range.
            penumbraBias = AZStd::clamp(penumbraBias, -0.999f, 0.999f);

            // Change space from (-1.0 to 1.0) to (-1.0 to infinity)
            penumbraBias = (2.0f * penumbraBias) / (1.0f - penumbraBias);

            m_spotLightData.GetData(handle.GetIndex()).m_penumbraBias = penumbraBias;
            m_deviceBufferNeedsUpdate = true;
        }

        void SpotLightFeatureProcessor::SetAttenuationRadius(LightHandle handle, float attenuationRadius)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SpotLightFeatureProcessor::SetAttenuationRadius().");

            attenuationRadius = AZStd::max<float>(attenuationRadius, 0.001f); // prevent divide by zero.
            SpotLightData& light = m_spotLightData.GetData(handle.GetIndex());
            light.m_invAttenuationRadiusSquared = 1.0f / (attenuationRadius * attenuationRadius);

            if (light.m_shadowIndex >= 0)
            {
                AZ_Assert(m_shadowProperties.find(handle) != m_shadowProperties.end(), "ShadowmapProperty is incorrect.");
                m_shadowProperties[handle].m_shadowmapViewNeedsUpdate = true;
                m_filterParameterNeedsUpdate = true;
            }
            m_deviceBufferNeedsUpdate = true;
        }

        void SpotLightFeatureProcessor::SetShadowmapSize(LightHandle handle, ShadowmapSize shadowmapSize)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SpotLightFeatureProcessor::SetShadowmapSize().");

            if (shadowmapSize == ShadowmapSize::None)
            {
                CleanUpShadow(handle);
            }
            else
            {
                PrepareForShadow(handle, shadowmapSize);
            }
        }

        void SpotLightFeatureProcessor::SetShadowFilterMethod(LightHandle handle, ShadowFilterMethod method)
        {
            ShadowProperty& property = GetOrCreateShadowProperty(handle);
            const uint16_t shadowIndex = property.m_shadowHandle.GetIndex();
            m_shadowData.GetData(shadowIndex).m_shadowFilterMethod = aznumeric_cast<uint32_t>(method);

            m_deviceBufferNeedsUpdate = true;
            property.m_shadowmapViewNeedsUpdate = true;

            if (m_shadowData.GetData(shadowIndex).m_shadowmapSize !=
                aznumeric_cast<uint32_t>(ShadowmapSize::None))
            {
                m_filterParameterNeedsUpdate = true;

                for (EsmShadowmapsPass* esmPass : m_esmShadowmapsPasses)
                {
                    esmPass->SetEnabledComputation(
                        method == ShadowFilterMethod::Esm ||
                        method == ShadowFilterMethod::EsmPcf);
                }
            }
        }

        void SpotLightFeatureProcessor::SetShadowBoundaryWidthAngle(LightHandle handle, float boundaryWidthDegree)
        {
            const auto& property = GetOrCreateShadowProperty(handle);
            const uint16_t shadowIndex = property.m_shadowHandle.GetIndex();
            m_shadowData.GetData(shadowIndex).m_boundaryScale = DegToRad(boundaryWidthDegree / 2.f);
            m_filterParameterNeedsUpdate = true;
            m_deviceBufferNeedsUpdate = true;
        }

        void SpotLightFeatureProcessor::SetPredictionSampleCount(LightHandle handle, uint16_t count)
        {
            if (count > Shadow::MaxPcfSamplingCount)
            {
                AZ_Warning("SpotLightFeatureProcessor", false, "Sampling count exceed the limit.");
                count = Shadow::MaxPcfSamplingCount;
            }
            const auto& property = GetOrCreateShadowProperty(handle);
            const uint16_t shadowIndex = property.m_shadowHandle.GetIndex();
            m_shadowData.GetData(shadowIndex).m_predictionSampleCount = count;
            
            m_deviceBufferNeedsUpdate = true;
        }

        void SpotLightFeatureProcessor::SetPcfMethod(LightHandle handle, PcfMethod method)
        {
            const auto& property = GetOrCreateShadowProperty(handle);
            const uint16_t shadowIndex = property.m_shadowHandle.GetIndex();
            m_shadowData.GetData(shadowIndex).m_pcfMethod = method;

            m_deviceBufferNeedsUpdate = true;
        }

        void SpotLightFeatureProcessor::SetFilteringSampleCount(LightHandle handle, uint16_t count)
        {
            if (count > Shadow::MaxPcfSamplingCount)
            {
                AZ_Warning("SpotLightFeatureProcessor", false, "Sampling count exceed the limit.");
                count = Shadow::MaxPcfSamplingCount;
            }
            auto property = m_shadowProperties.find(handle);
            if (property == m_shadowProperties.end())
            {
                // If shadow has not been ready yet, prepare it
                // for placeholder of shadow filter method value.
                PrepareForShadow(handle, ShadowmapSize::None);
                property = m_shadowProperties.find(handle);
            }

            const uint16_t shadowIndex = property->second.m_shadowHandle.GetIndex();
            m_shadowData.GetData(shadowIndex).m_filteringSampleCount = count;
            m_deviceBufferNeedsUpdate = true;
        }

        void SpotLightFeatureProcessor::SetSpotLightData(LightHandle handle, const SpotLightData& data)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SpotLightFeatureProcessor::SetSpotLightData().");

            m_spotLightData.GetData(handle.GetIndex()) = data;
            m_deviceBufferNeedsUpdate = true;
            m_shadowmapPassNeedsUpdate = true;
        }

        const Data::Instance<RPI::Buffer> SpotLightFeatureProcessor::GetLightBuffer()const
        {
            return m_lightBufferHandler.GetBuffer();
        }

        uint32_t SpotLightFeatureProcessor::GetLightCount() const
        {
            return m_lightBufferHandler.GetElementCount();
        }

        SpotLightFeatureProcessor::ShadowProperty& SpotLightFeatureProcessor::GetOrCreateShadowProperty(LightHandle handle)
        {
            auto propIt = m_shadowProperties.find(handle);
            if (propIt == m_shadowProperties.end())
            {
                // If shadow has not been ready yet, prepare it
                // for placeholder of shadow filter method value.
                PrepareForShadow(handle, ShadowmapSize::None);
                propIt = m_shadowProperties.find(handle);
            }
            return propIt->second;
        }

        void SpotLightFeatureProcessor::PrepareForShadow(LightHandle handle, ShadowmapSize size)
        {
            m_deviceBufferNeedsUpdate = true;
            m_shadowmapPassNeedsUpdate = true;

            auto propIt = m_shadowProperties.find(handle);

            // If shadowmap size is already set, early return;
            if (propIt != m_shadowProperties.end() &&
                m_shadowData.GetData(propIt->second.m_shadowHandle.GetIndex()).m_shadowmapSize == aznumeric_cast<uint32_t>(size))
            {
                return;
            }

            // If shadow is not ready, prepare related structures.
            if (propIt == m_shadowProperties.end())
            {
                const uint16_t shadowIndex = m_shadowData.GetFreeSlotIndex();
                const uint16_t esmIndex = m_esmParameterData.GetFreeSlotIndex();
                AZ_Assert(shadowIndex == esmIndex, "Indices of shadow must coincide.");

                m_spotLightData.GetData(handle.GetIndex()).m_shadowIndex = m_shadowData.GetRawIndex(shadowIndex);

                ShadowProperty property;
                property.m_shadowHandle = LightHandle(shadowIndex);
                AZ::Name viewName(AZStd::string::format("SpotLightShadowView (lightId:%d)", handle.GetIndex()));
                property.m_shadowmapView = RPI::View::CreateView(viewName, RPI::View::UsageShadow);
                property.m_shadowmapViewNeedsUpdate = true;
                m_shadowProperties.insert(AZStd::make_pair(handle, AZStd::move(property)));

                propIt = m_shadowProperties.find(handle);
            }

            // Set shadowmap size to shadow data.
            const uint16_t shadowIndex = propIt->second.m_shadowHandle.GetIndex();
            m_shadowData.GetData(shadowIndex).m_shadowmapSize = aznumeric_cast<uint32_t>(size);

            m_filterParameterNeedsUpdate = true;
        }

        void SpotLightFeatureProcessor::CleanUpShadow(LightHandle handle)
        {
            const auto& propIt = m_shadowProperties.find(handle);
            if (propIt == m_shadowProperties.end())
            {
                return;
            }

            const uint16_t shadowIndex = propIt->second.m_shadowHandle.GetIndex();
            m_shadowData.RemoveIndex(shadowIndex);
            m_esmParameterData.RemoveIndex(shadowIndex);
            m_shadowProperties.erase(handle);
            m_spotLightData.GetData(handle.GetIndex()).m_shadowIndex = -1;

            // By removing shadow of a light, shadow indices of the other lights
            // can become stale.  So they should be updated.
            for (auto& propIt2 : m_shadowProperties)
            {
                const LightHandle lightHandle = propIt2.first;
                const LightHandle shadowHandle = propIt2.second.m_shadowHandle;
                m_spotLightData.GetData(lightHandle.GetIndex()).m_shadowIndex = m_shadowData.GetRawIndex(shadowHandle.GetIndex());
            }

            m_shadowmapPassNeedsUpdate = true;
        }

        void SpotLightFeatureProcessor::UpdateShadowmapViews()
        {
            if (m_spotLightShadowmapsPasses.empty() || m_esmShadowmapsPasses.empty())
            {
                return;
            }

            for (auto& it : m_shadowProperties)
            {
                if (!it.second.m_shadowmapViewNeedsUpdate)
                {
                    continue;
                }
                it.second.m_shadowmapViewNeedsUpdate = false;
                const SpotLightData& light = m_spotLightData.GetData(it.first.GetIndex());

                const float invRadiusSquared = light.m_invAttenuationRadiusSquared;
                if (invRadiusSquared <= 0.f)
                {
                    AZ_Assert(false, "Attenuation radius have to be set before use the light.");
                    continue;
                }
                const float attenuationRadius = sqrtf(1.f / invRadiusSquared);

                constexpr float SmallAngle = 0.01f;
                const float coneAngle = GetMax(m_lightProperties.GetData(it.first.GetIndex()).m_outerConeAngle, SmallAngle);

                // Set view's matrices.
                RPI::ViewPtr view = it.second.m_shadowmapView;
                Vector3 position = Vector3::CreateFromFloat3(light.m_position.data());
                const Vector3 direction = Vector3::CreateFromFloat3(light.m_direction.data());

                // To handle bulb radius, set the position of the shadow caster behind the actual light depending on the radius of the bulb
                //
                //   \         /
                //    \       /
                //     \_____/  <-- position of light itself (and forward plane of shadow casting view)
                //      .   .
                //       . .
                //        *     <-- position of shadow casting view
                //
                position += light.m_bulbPostionOffset * -direction;
                const auto transform = Matrix3x4::CreateLookAt(position, position + direction);
                view->SetCameraTransform(transform);
                
                // If you adjust "NearFarRatio" below, the constant "bias" in SpotLightShadow::GetVisibility()
                // in SpotLightShadow.azsli should also be adjusted in order to avoid Peter-Pannings.
                constexpr float NearFarRatio = 10000.f;
                const float minDist = attenuationRadius / NearFarRatio;

                const float nearDist = GetMax(minDist, light.m_bulbPostionOffset); 
                float farDist = attenuationRadius + light.m_bulbPostionOffset;

                constexpr float AspectRatio = 1.0f;

                Matrix4x4 viewToClipMatrix;
                MakePerspectiveFovMatrixRH(
                    viewToClipMatrix,
                    coneAngle,
                    AspectRatio,
                    nearDist,
                    farDist);

                view->SetViewToClipMatrix(viewToClipMatrix);

                const uint16_t shadowIndex = it.second.m_shadowHandle.GetIndex();
                SpotLightShadowData& shadow = m_shadowData.GetData(shadowIndex);
                
                EsmShadowmapsPass::FilterParameter& esmData = m_esmParameterData.GetData(shadowIndex);
                if (shadow.m_shadowFilterMethod == aznumeric_cast<uint32_t>(ShadowFilterMethod::Esm) ||
                    shadow.m_shadowFilterMethod == aznumeric_cast<uint32_t>(ShadowFilterMethod::EsmPcf))
                {
                    // Set parameters to calculate linear depth if ESM is used.
                    m_filterParameterNeedsUpdate = true;
                    esmData.m_isEnabled = true;
                    esmData.m_n_f_n = nearDist / (farDist - nearDist);
                    esmData.m_n_f = nearDist - farDist;
                    esmData.m_f = farDist;
                }
                else
                {
                    // Reset enabling flag if ESM is not used.
                    esmData.m_isEnabled = false;
                }
            }
        }

        void SpotLightFeatureProcessor::SetShadowParameterToShadowData()
        {
            for (const auto& propIt : m_shadowProperties)
            {
                const LightHandle& shadowHandle = propIt.second.m_shadowHandle;
                AZ_Assert(shadowHandle.IsValid(), "Shadow handle is invalid.");
                SpotLightShadowData& shadowData = m_shadowData.GetData(shadowHandle.GetIndex());

                // Set depth bias matrix.
                const Matrix4x4& worldToLightClipMatrix = propIt.second.m_shadowmapView->GetWorldToClipMatrix();
                const Matrix4x4 depthBiasMatrix = Shadow::GetClipToShadowmapTextureMatrix() * worldToLightClipMatrix;
                shadowData.m_depthBiasMatrix = depthBiasMatrix;
                shadowData.m_unprojectConstants = GetDepthUnprojectConstants(propIt.second.m_shadowmapView);

                m_deviceBufferNeedsUpdate = true;
            }
        }

        uint16_t SpotLightFeatureProcessor::GetLightIndexInSrg(LightHandle handle) const
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SpotLightFeatureProcessor::GetLightIndexInSrg().");

            return m_spotLightData.GetRawIndex(handle.GetIndex());
        }

        void SpotLightFeatureProcessor::CachePasses()
        {
            const AZStd::vector<RPI::RenderPipelineId> validPipelineIds = CacheSpotLightShadowmapsPass();
            CacheEsmShadowmapsPass(validPipelineIds);
            m_shadowmapPassNeedsUpdate = true;
        }

        AZStd::vector<RPI::RenderPipelineId> SpotLightFeatureProcessor::CacheSpotLightShadowmapsPass()
        {
            const AZStd::vector<RPI::RenderPipelinePtr>& renderPipelines = GetParentScene()->GetRenderPipelines();
            const auto* passSystem = RPI::PassSystemInterface::Get();
            const AZStd::vector<RPI::Pass*>& passes = passSystem->GetPassesForTemplateName(Name("SpotLightShadowmapsTemplate"));

            AZStd::vector<RPI::RenderPipelineId> validPipelineIds;
            m_spotLightShadowmapsPasses.clear();
            for (RPI::Pass* pass : passes)
            {
                SpotLightShadowmapsPass* shadowPass = azrtti_cast<SpotLightShadowmapsPass*>(pass);
                AZ_Assert(shadowPass, "It is not a SpotLightShadowmapsPass.");
                for (const RPI::RenderPipelinePtr& pipeline : renderPipelines)
                {
                    if (pipeline.get() == shadowPass->GetRenderPipeline())
                    {
                        m_spotLightShadowmapsPasses.emplace_back(shadowPass);
                        validPipelineIds.push_back(shadowPass->GetRenderPipeline()->GetId());
                    }
                }
            }
            return validPipelineIds;
        }

        void SpotLightFeatureProcessor::CacheEsmShadowmapsPass(const AZStd::vector<RPI::RenderPipelineId>& validPipelineIds)
        {
            const auto* passSystem = RPI::PassSystemInterface::Get();
            const AZStd::vector<RPI::Pass*> passes = passSystem->GetPassesForTemplateName(Name("EsmShadowmapsTemplate"));

            m_esmShadowmapsPasses.clear();
            for (RPI::Pass* pass : passes)
            {
                EsmShadowmapsPass* esmPass = azrtti_cast<EsmShadowmapsPass*>(pass);
                AZ_Assert(esmPass, "It is not an EsmShadowmapsPass.");
                if (esmPass->GetRenderPipeline() &&
                    AZStd::find(validPipelineIds.begin(), validPipelineIds.end(), esmPass->GetRenderPipeline()->GetId()) != validPipelineIds.end() &&
                    esmPass->GetLightTypeName() == m_lightTypeName)
                {
                    m_esmShadowmapsPasses.emplace_back(esmPass);
                }
            }
        }

        void SpotLightFeatureProcessor::UpdateFilterParameters()
        {
            if (m_filterParameterNeedsUpdate)
            {
                UpdateStandardDeviations();
                UpdateFilterOffsetsCounts();
                UpdateShadowmapPositionsInAtlas();
                SetFilterParameterToPass();
                m_filterParameterNeedsUpdate = false;
            }
        }

        void SpotLightFeatureProcessor::UpdateStandardDeviations()
        {
            if (m_esmShadowmapsPasses.empty())
            {
                AZ_Error("SpotLightFeatureProcessor", false, "Cannot find a required pass.");
                return;
            }

            AZStd::vector<float> standardDeviations(m_shadowData.GetDataCount());
            for (const auto& propIt : m_shadowProperties)
            {
                if (!NeedsFilterUpdate(propIt.second.m_shadowHandle))
                {
                    continue;
                }
                const SpotLightShadowData& shadow = m_shadowData.GetData(propIt.second.m_shadowHandle.GetIndex());
                const float boundaryWidthAngle = shadow.m_boundaryScale * 2.f;
                constexpr float SmallAngle = 0.01f;
                const float coneAngle = GetMax(m_lightProperties.GetData(propIt.first.GetIndex()).m_outerConeAngle, SmallAngle);
                const float ratioToEntireWidth = boundaryWidthAngle / coneAngle;
                const float widthInPixels = ratioToEntireWidth * shadow.m_shadowmapSize;
                const float standardDeviation = widthInPixels / (2 * GaussianMathFilter::ReliableSectionFactor);
                const int32_t shadowIndexInSrg = m_spotLightData.GetData(propIt.first.GetIndex()).m_shadowIndex;
                standardDeviations[shadowIndexInSrg] = standardDeviation;
            }
            if (standardDeviations.empty())
            {
                for (EsmShadowmapsPass* esmPass : m_esmShadowmapsPasses)
                {
                    esmPass->SetEnabledComputation(false);
                }
                return;
            }
            for (EsmShadowmapsPass* esmPass : m_esmShadowmapsPasses)
            {
                esmPass->SetEnabledComputation(true);
                esmPass->SetFilterParameters(standardDeviations);
            }
        }

        void SpotLightFeatureProcessor::UpdateFilterOffsetsCounts()
        {
            if (m_esmShadowmapsPasses.empty())
            {
                AZ_Error("SpotLightFeatureProcessor", false, "Cannot find a required pass.");
                return;
            }

            // Get array of filter counts for the camera view.
            const AZStd::array_view<uint32_t> filterCounts = m_esmShadowmapsPasses.front()->GetFilterCounts();

            // Create array of filter offsets.
            AZStd::vector<uint32_t> filterOffsets;
            filterOffsets.reserve(filterCounts.size());
            uint32_t filterOffset = 0;
            for (const uint32_t count : filterCounts)
            {
                filterOffsets.push_back(filterOffset);
                filterOffset += count;
            }

            for (auto& propIt : m_shadowProperties)
            {
                const LightHandle shadowHandle = propIt.second.m_shadowHandle;
                EsmShadowmapsPass::FilterParameter& filterParameter = m_esmParameterData.GetData(shadowHandle.GetIndex());
                if (NeedsFilterUpdate(shadowHandle))
                {
                    // Write filter offsets and filter counts to ESM data.
                    const int32_t shadowIndexInSrg = m_spotLightData.GetData(propIt.first.GetIndex()).m_shadowIndex;
                    AZ_Assert(shadowIndexInSrg >= 0, "Shadow index in SRG must be non-negative.");
                    filterParameter.m_parameterOffset = filterOffsets[shadowIndexInSrg];
                    filterParameter.m_parameterCount = filterCounts[shadowIndexInSrg];
                }
                else
                {
                    // If filter is not required, reset offsets and counts of filter in ESM data.
                    filterParameter.m_parameterOffset = 0;
                    filterParameter.m_parameterCount = 0;
                }
            }
        }

        void SpotLightFeatureProcessor::UpdateShadowmapPositionsInAtlas()
        {
            if (m_spotLightShadowmapsPasses.empty())
            {
                AZ_Error("SpotLightFeatureProcessor", false, "Cannot find a required pass.");
                return;
            }

            const ShadowmapAtlas& atlas = m_spotLightShadowmapsPasses.front()->GetShadowmapAtlas();
            for (auto& propIt : m_shadowProperties)
            {
                const uint16_t shadowIndex = propIt.second.m_shadowHandle.GetIndex();
                EsmShadowmapsPass::FilterParameter& esmData = m_esmParameterData.GetData(shadowIndex);

                // Set shadowmap size to ESM data.
                const uint32_t shadowmapSize = m_shadowData.GetData(shadowIndex).m_shadowmapSize;
                esmData.m_shadowmapSize = shadowmapSize;

                // Set shadowmap origin to ESM data.
                const int32_t shadowIndexInSrg = m_spotLightData.GetData(propIt.first.GetIndex()).m_shadowIndex;
                AZ_Assert(shadowIndexInSrg >= 0, "Shadow index required to be non-negative.")
                const ShadowmapAtlas::Origin origin = atlas.GetOrigin(aznumeric_cast<size_t>(shadowIndexInSrg));
                const AZStd::array<uint32_t, 2> originInSlice = origin.m_originInSlice;
                esmData.m_shadowmapOriginInSlice = originInSlice;
            }
        }

        void SpotLightFeatureProcessor::SetFilterParameterToPass()
        {
            if (m_spotLightShadowmapsPasses.empty() || m_esmShadowmapsPasses.empty())
            {
                AZ_Error("SpotLightFeatureProcessor", false, "Cannot find a required pass.");
                return;
            }

            // Create index table buffer.
            const AZStd::string indexTableBufferName =
                AZStd::string::format("IndexTableBuffer(Spot) %d",
                    m_shadowmapIndexTableBufferNameIndex++);
            const ShadowmapAtlas& atlas = m_spotLightShadowmapsPasses.front()->GetShadowmapAtlas();
            const Data::Instance<RPI::Buffer> indexTableBuffer = atlas.CreateShadowmapIndexTableBuffer(indexTableBufferName);

            // Update ESM parameter buffer which is attached to
            // both of Forward Pass and ESM Shadowmaps Pass.
            m_esmParameterBufferHandler.UpdateBuffer(m_esmParameterData.GetDataVector());

            // Set index table buffer and ESM parameter buffer to ESM pass.
            for (EsmShadowmapsPass* esmPass : m_esmShadowmapsPasses)
            {
                esmPass->SetShadowmapIndexTableBuffer(indexTableBuffer);
                esmPass->SetFilterParameterBuffer(m_esmParameterBufferHandler.GetBuffer());
            }
        }

        bool SpotLightFeatureProcessor::NeedsFilterUpdate(LightHandle shadowHandle) const
        {
            const SpotLightShadowData& shadow = m_shadowData.GetData(shadowHandle.GetIndex());
            const bool useEsm =
                (aznumeric_cast<ShadowFilterMethod>(shadow.m_shadowFilterMethod) == ShadowFilterMethod::Esm ||
                 aznumeric_cast<ShadowFilterMethod>(shadow.m_shadowFilterMethod) == ShadowFilterMethod::EsmPcf);
            return (aznumeric_cast<ShadowmapSize>(shadow.m_shadowmapSize) != ShadowmapSize::None) && useEsm;
        }
        
        void SpotLightFeatureProcessor::UpdateBulbPositionOffset(SpotLightData& light)
        {
            // If we have the outer cone angle in radians, the offset is (radius * tan(pi/2 - coneRadians)). However
            // light stores the cosine of outerConeRadians, making the equation (radius * tan(pi/2 - acosf(cosConeRadians)).
            // This simplifies to the equation below.
            float cosConeRadians = light.m_outerConeAngle;
            light.m_bulbPostionOffset = light.m_bulbRadius * cosConeRadians / sqrt(1 - cosConeRadians * cosConeRadians);
        }
    } // namespace Render
} // namespace AZ
