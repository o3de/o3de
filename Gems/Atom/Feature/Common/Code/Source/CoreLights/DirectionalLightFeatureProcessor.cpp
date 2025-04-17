/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/DirectionalLightFeatureProcessor.h>
#include <CoreLights/CascadedShadowmapsPass.h>
#include <CoreLights/EsmShadowmapsPass.h>
#include <CoreLights/Shadow.h>
#include <Math/GaussianMathFilter.h>

#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>
#include <Atom/RPI.Public/ColorManagement/TransformColor.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/Specific/EnvironmentCubeMapPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>
#include <AtomCore/Instance/Instance.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Console/Console.h>
#include <PostProcessing/FastDepthAwareBlurPasses.h>
#include <Shadows/FullscreenShadowPass.h>

namespace AZ
{
    namespace Render
    {
        AZ_CVAR(bool, r_excludeItemsInSmallerShadowCascades, true, nullptr, ConsoleFunctorFlags::Null, "Set to true to exclude drawing items to a directional shadow cascade that are already covered by a smaller cascade.");
        
        AZ_CVAR(int, r_directionalShadowFilteringMethod, -1, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Cvar to override directional shadow filtering mode. -1 = Default settings from Editor, 0 = None, 1 = Pcf, 2 = Esm, 3 = EsmPcf.");
        AZ_CVAR(int, r_directionalShadowFilteringSampleCountMode, -1, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Cvar to override directional shadow sample count mode. -1 = Default settings from Editor, 0 = PcfTap4, 1 = PcfTap9, 2 = PcfTap16");

        // --- Camera Configuration ---

        CascadeShadowCameraConfiguration::CascadeShadowCameraConfiguration()
        {
            SetDepthCenterRatio();
        }

        void CascadeShadowCameraConfiguration::SetBaseConfiguration(const Camera::Configuration& baseConfig)
        {
            m_baseConfiguration = baseConfig;

            constexpr float MinFovY = Constants::Pi / 1e4;
            constexpr float MaxFovY = Constants::Pi - MinFovY;
            if (baseConfig.m_fovRadians < MinFovY)
            {
                AZ_Error("CameraConfiguration", false, "FoV must be positive.");
                m_baseConfiguration.m_fovRadians = MinFovY;
            }
            else if (baseConfig.m_fovRadians > MaxFovY)
            {
                AZ_Error("CameraConfiguration", false, "FoV must be less than 180 degree.");
                m_baseConfiguration.m_fovRadians = MaxFovY;
            }

            AZ_Error("CameraConfiguration", m_baseConfiguration.m_nearClipDistance > 0, "near depth clip distance must be positive.");
            AZ_Error("CameraConfiguration", m_baseConfiguration.m_nearClipDistance < m_baseConfiguration.m_farClipDistance, "far depth clip distance must be greater than near depth clip distance.");

            m_aspectRatio = m_baseConfiguration.m_frustumWidth / m_baseConfiguration.m_frustumHeight;
            AZ_Error("CameraConfiguration", m_aspectRatio > 0, "AspectRatio must be positive.");

            SetDepthCenterRatio();
        }

        void CascadeShadowCameraConfiguration::SetShadowDepthFar(float depthFar)
        {
            m_shadowDepthFar = depthFar;
        }

        float CascadeShadowCameraConfiguration::GetFovY() const
        {
            return m_baseConfiguration.m_fovRadians;
        }

        float CascadeShadowCameraConfiguration::GetDepthNear() const
        {
            return m_baseConfiguration.m_nearClipDistance;
        }

        float CascadeShadowCameraConfiguration::GetDepthFar() const
        {
            return GetMin(m_baseConfiguration.m_farClipDistance, m_shadowDepthFar);
        }

        float CascadeShadowCameraConfiguration::GetAspectRatio() const
        {
            return m_aspectRatio;
        }

        float CascadeShadowCameraConfiguration::GetDepthCenter(float depthNear, float depthFar) const
        {
            AZ_Assert(m_depthCenterRatio > 0.f, "m_depthCenterRatio has not been initialized properly.");
            // Letting the position of the center (0, dC, 0),
            // we assume the distances from the center to 8 vertices of the frustum
            // are equal.  Then we have the following equation:
            // (dF - dC)^2 + hF^2 + wF^2 = (dC - dN)^2 + hN^2 + wN^2,
            // where dF is depthFar, dN is depthNear,
            // hF = dF tan(fov/2) is the half of the far plane's height,
            // wF = dF tan(fov/2) ar is the half of the far plane's width,
            // ar is the aspect ratio, fov is the FoVY,
            // hN and wN is similar to hF and wF w.r.t. near plane.
            // (Y=dN and Y=dF are the near and far planes resp.)
            // Solving this equation, we have
            // dc = (dN + dF) / 2 * {1 + tan^2(fov/2) (1 + ar^2)}.
            return (depthNear + depthFar) / 2 * m_depthCenterRatio;
        }

        float CascadeShadowCameraConfiguration::GetDepthCenterRatio() const
        {
            AZ_Assert(m_depthCenterRatio > 0.f, "m_depthCenterRatio has not been initialized.");
            return m_depthCenterRatio;
        }

        bool CascadeShadowCameraConfiguration::HasSameConfiguration(const Camera::Configuration& config) const
        {
            return (
                m_baseConfiguration.m_fovRadians == config.m_fovRadians &&
                m_baseConfiguration.m_nearClipDistance == config.m_nearClipDistance &&
                m_baseConfiguration.m_farClipDistance == config.m_farClipDistance &&
                m_baseConfiguration.m_frustumWidth == config.m_frustumWidth &&
                m_baseConfiguration.m_frustumHeight == config.m_frustumHeight);
        }

        void CascadeShadowCameraConfiguration::SetDepthCenterRatio()
        {
            // For the meaning of the calculation, refer GetDepthCenter().
            const float tanFovYHalf = tanf(m_baseConfiguration.m_fovRadians / 2);
            m_depthCenterRatio =
                (1 + tanFovYHalf * tanFovYHalf * (1 + m_aspectRatio * m_aspectRatio));
        }

        // --- Feature Processor ---

        void DirectionalLightFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<DirectionalLightFeatureProcessor, RPI::FeatureProcessor>()
                    ->Version(0);
            }
        }

        void DirectionalLightFeatureProcessor::Activate()
        {
            const RHI::ShaderResourceGroupLayout* sceneSrgLayout = RPI::RPISystemInterface::Get()->GetSceneSrgLayout().get();

            GpuBufferHandler::Descriptor desc;

            desc.m_bufferName = "DirectionalLightBuffer";
            desc.m_bufferSrgName = "m_directionalLights";
            desc.m_elementCountSrgName = "m_directionalLightCount";
            desc.m_elementSize = sizeof(DirectionalLightData);
            desc.m_srgLayout = sceneSrgLayout;

            m_lightBufferHandler = GpuBufferHandler(desc);

            m_shadowIndexDirectionalLightIndex.Reset();

            m_auxGeomFeatureProcessor = GetParentScene()->GetFeatureProcessor<RPI::AuxGeomFeatureProcessorInterface>();

            PrepareForChangingRenderPipelineAndCameraView();
            EnableSceneNotification();
        }

        void DirectionalLightFeatureProcessor::Deactivate()
        {
            DisableSceneNotification();

            m_lightBufferHandler.Release();
            m_lightData.Clear();

            m_shadowBufferHandlers.clear();
            m_shadowData.clear();

            m_esmParameterBufferHandlers.clear();
            m_esmParameterData.clear();

            m_shadowProperties.Clear();

            if (GetParentScene()->GetDefaultRenderPipeline())
            {
                SleepShadowmapPasses();
            }

            // Remove retaining AuxGeomDraw for camera views.
            if (m_auxGeomFeatureProcessor)
            {
                for (const RPI::View* cameraView : m_viewsRetainingAuxGeomDraw)
                {
                    m_auxGeomFeatureProcessor->ReleaseDrawQueueForView(cameraView);
                }
                m_viewsRetainingAuxGeomDraw.clear();
            }
        }

        void DirectionalLightFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket&)
        {
            AZ_PROFILE_SCOPE(RPI, "DirectionalLightFeatureProcessor: Simulate");

            if (m_shadowingLightHandle.IsValid())
            {
                SetFullscreenPassSettings();

                const auto& shadowData = m_shadowData.at(nullptr).GetData(m_shadowingLightHandle.GetIndex());

                uint32_t shadowFilterMethod = shadowData.m_shadowFilterMethod;
                if(r_directionalShadowFilteringMethod >= 0)
                {
                    shadowFilterMethod = r_directionalShadowFilteringMethod;
                }
                RPI::ShaderSystemInterface::Get()->SetGlobalShaderOption(m_directionalShadowFilteringMethodName, AZ::RPI::ShaderOptionValue{shadowFilterMethod});

                uint32_t shadowFilteringSampleCountMode = shadowData.m_filteringSampleCountMode;
                if(r_directionalShadowFilteringSampleCountMode >= 0)
                {
                    shadowFilteringSampleCountMode = r_directionalShadowFilteringSampleCountMode;
                }
                
                RPI::ShaderSystemInterface::Get()->SetGlobalShaderOption(
                    m_directionalShadowFilteringSamplecountName, AZ::RPI::ShaderOptionValue{ shadowFilteringSampleCountMode });
                
                RPI::ShaderSystemInterface::Get()->SetGlobalShaderOption(m_directionalShadowReceiverPlaneBiasEnableName, AZ::RPI::ShaderOptionValue{ m_shadowProperties.GetData(m_shadowingLightHandle.GetIndex()).m_isReceiverPlaneBiasEnabled });

                const uint32_t cascadeCount = shadowData.m_cascadeCount;

                RPI::ShaderSystemInterface::Get()->SetGlobalShaderOption(m_BlendBetweenCascadesEnableName, AZ::RPI::ShaderOptionValue{cascadeCount > 1 && m_shadowProperties.GetData(m_shadowingLightHandle.GetIndex()).m_blendBetweenCascades });

                ShadowProperty& property = m_shadowProperties.GetData(m_shadowingLightHandle.GetIndex());
                bool segmentsNeedUpdate = property.m_segments.empty();
                for (const auto& passIt : m_cascadedShadowmapsPasses)
                {
                    const CascadedShadowmapsPass* pass = passIt.second.front();
                    RPI::RenderPipeline* pipeline = pass->GetRenderPipeline();
                    const RPI::View* cameraView = pipeline->GetDefaultView().get();
                    if (property.m_segments.find(cameraView) == property.m_segments.end())
                    {
                        segmentsNeedUpdate = true;
                        break;
                    }
                    if (property.m_segments.at(cameraView).size() != cascadeCount)
                    {
                        segmentsNeedUpdate = true;
                        break;
                    }
                }
                if (segmentsNeedUpdate)
                {
                    UpdateViewsOfCascadeSegments(m_shadowingLightHandle, static_cast<uint16_t>(cascadeCount));
                    SetShadowmapImageSizeArraySize(m_shadowingLightHandle);
                }

                if (property.m_frustumNeedsUpdate)
                {
                    UpdateFrustums(m_shadowingLightHandle);
                    property.m_frustumNeedsUpdate = false;
                }
                if (property.m_borderDepthsForSegmentsNeedsUpdate)
                {
                    UpdateBorderDepthsForSegments(m_shadowingLightHandle);
                    property.m_borderDepthsForSegmentsNeedsUpdate = false;
                }
                if (property.m_shadowmapViewNeedsUpdate || m_previousExcludeCvarValue != r_excludeItemsInSmallerShadowCascades)
                {
                    UpdateShadowmapViews(m_shadowingLightHandle);
                    UpdateFilterParameters(m_shadowingLightHandle);
                    property.m_shadowmapViewNeedsUpdate = false;
                    m_previousExcludeCvarValue = r_excludeItemsInSmallerShadowCascades;
                }
                SetShadowParameterToShadowData(m_shadowingLightHandle);
            }

            if (m_lightBufferNeedsUpdate)
            {
                m_lightBufferHandler.UpdateBuffer(m_lightData.GetDataVector());
                m_lightBufferNeedsUpdate = false;
            }
            if (m_shadowBufferNeedsUpdate)
            {
                for (auto& handlerIt : m_shadowBufferHandlers)
                {
                    handlerIt.second.UpdateBuffer(m_shadowData.at(handlerIt.first).GetDataVector());
                }
                m_shadowBufferNeedsUpdate = false;
            }
        }

        void DirectionalLightFeatureProcessor::PrepareViews(const PrepareViewsPacket&, AZStd::vector<AZStd::pair<RPI::PipelineViewTag, RPI::ViewPtr>>& outViews)
        {
            if (m_shadowingLightHandle.IsValid())
            {
                const ShadowProperty& property = m_shadowProperties.GetData(m_shadowingLightHandle.GetIndex());
                for (const auto& segmentIt : property.m_segments)
                {
                    for (const CascadeSegment& segment : segmentIt.second)
                    {
                        RHI::DrawListMask drawListMask;
                        for (const RPI::RenderPipelineId& renderPipelineId : m_renderPipelineIdsForPersistentView.at(segmentIt.first))
                        {
                            const RPI::RenderPipelinePtr renderPipeline = GetParentScene()->GetRenderPipeline(renderPipelineId);
                            const RHI::DrawListMask pipelineDrawListMask = renderPipeline->GetDrawListMask(segment.m_pipelineViewTag);
                            drawListMask |= pipelineDrawListMask;
                        }

                        segment.m_view->SetDrawListMask(drawListMask);

                        outViews.emplace_back(AZStd::make_pair(
                            segment.m_pipelineViewTag,
                            segment.m_view));
                    }
                }
            }
        }

        void DirectionalLightFeatureProcessor::Render(const FeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "DirectionalLightFeatureProcessor: Render");

            if (m_shadowingLightHandle.IsValid())
            {
                DrawCascadeBoundingBoxes(m_shadowingLightHandle);
            }

            m_lightBufferHandler.UpdateSrg(GetParentScene()->GetShaderResourceGroup().get());

            for (const RPI::ViewPtr& view : packet.m_views)
            {
                if (m_renderPipelineIdsForPersistentView.find(view.get()) != m_renderPipelineIdsForPersistentView.end() &&
                    (RHI::CheckBitsAny(view->GetUsageFlags(), RPI::View::UsageCamera | RPI::View::UsageReflectiveCubeMap)))
                {
                    RPI::ShaderResourceGroup* viewSrg = view->GetShaderResourceGroup().get();

                    uint32_t rawShadowIndex = 0; // The shader recoginizable index of the shadowing light.
                    if (m_shadowingLightHandle.IsValid())
                    {
                        rawShadowIndex = aznumeric_cast<uint32_t>(m_shadowData.at(view.get()).GetRawIndex(m_shadowingLightHandle.GetIndex()));
                    }

                    m_shadowBufferHandlers.at(view.get()).UpdateSrg(viewSrg);
                    auto itr = m_esmParameterBufferHandlers.find(view.get());
                    if (itr != m_esmParameterBufferHandlers.end())
                    {
                        itr->second.UpdateBuffer(m_esmParameterData.at(view.get()).GetDataVector());
                        itr->second.UpdateSrg(viewSrg);
                    }
                    viewSrg->SetConstant(m_shadowIndexDirectionalLightIndex, rawShadowIndex);
                }
            }
        }

        // --- Directional Light ---

        DirectionalLightFeatureProcessorInterface::LightHandle DirectionalLightFeatureProcessor::AcquireLight()
        {
            const uint16_t index = m_lightData.GetFreeSlotIndex();
            [[maybe_unused]] const uint16_t shadowPropIndex = m_shadowProperties.GetFreeSlotIndex();
            AZ_Assert(index == shadowPropIndex, "light index is illegal.");
            for (const auto& viewIt : m_cameraViewNames)
            {
                [[maybe_unused]] const uint16_t shadowIndex = m_shadowData.at(viewIt.first).GetFreeSlotIndex();
                AZ_Assert(index == shadowIndex, "light index is illegal.");
            }

            if (index == IndexedDataVector<DirectionalLightData>::NoFreeSlot)
            {
                return LightHandle::Null;
            }
            else
            {
                m_lightBufferNeedsUpdate = true;
                m_shadowBufferNeedsUpdate = true;

                m_shadowProperties.GetData(index).m_cameraConfigurations[nullptr] = {};

                const LightHandle handle(index);
                SetCascadeCount(handle, 1); // 1 cascade initially.
                return handle;
            }
        }

        bool DirectionalLightFeatureProcessor::ReleaseLight(LightHandle& handle)
        {
            if (handle.IsValid())
            {
                m_lightData.RemoveIndex(handle.GetIndex());
                for (auto& it : m_shadowData)
                {
                    it.second.RemoveIndex(handle.GetIndex());
                }
                m_shadowProperties.RemoveIndex(handle.GetIndex());

                if (handle == m_shadowingLightHandle)
                {
                    m_shadowingLightHandle.Reset();
                    SleepShadowmapPasses(); // The shadowing light is released, so shadowmap passes can sleep.
                }

                m_lightBufferNeedsUpdate = true;
                m_shadowBufferNeedsUpdate = true;
                handle.Reset();
                return true;
            }
            return false;
        }

        DirectionalLightFeatureProcessorInterface::LightHandle DirectionalLightFeatureProcessor::CloneLight(LightHandle sourceLightHandle)
        {
            AZ_Assert(sourceLightHandle.IsValid(), "Invalid LightHandle passed to DirectionalLightFeatureProcessor::CloneLight().");

            LightHandle handle = AcquireLight();
            if (handle.IsValid())
            {
                m_lightData.GetData(handle.GetIndex()) = m_lightData.GetData(sourceLightHandle.GetIndex());
                for (auto& it : m_shadowData)
                {
                    it.second.GetData(handle.GetIndex()) = it.second.GetData(sourceLightHandle.GetIndex());
                }
                m_shadowProperties.GetData(handle.GetIndex()) = m_shadowProperties.GetData(sourceLightHandle.GetIndex());

                m_lightBufferNeedsUpdate = true;
                m_shadowBufferNeedsUpdate = true;
            }
            return handle;
        }

        void DirectionalLightFeatureProcessor::SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnit::Lux>& lightColor)
        {
            const auto transformedColor = RPI::TransformColor(lightColor, RPI::ColorSpaceId::LinearSRGB, RPI::ColorSpaceId::ACEScg);

            auto& rgbIntensity = m_lightData.GetData(handle.GetIndex()).m_rgbIntensity;
            rgbIntensity[0] = transformedColor.GetR();
            rgbIntensity[1] = transformedColor.GetG();
            rgbIntensity[2] = transformedColor.GetB();
            m_lightBufferNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::SetDirection(LightHandle handle, const Vector3& lightDirection)
        {
            auto& direction = m_lightData.GetData(handle.GetIndex()).m_direction;
            lightDirection.StoreToFloat3(direction.data());

            m_lightBufferNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::SetAngularDiameter(LightHandle handle, float angularDiameter)
        {
            // Convert diameter to radius (diameter / 2.0) then radians (radius * pi / 180);
            m_lightData.GetData(handle.GetIndex()).m_angularRadius = angularDiameter * (Constants::Pi / 360.0f);
            m_lightBufferNeedsUpdate = true;
        }

        // --- Cascade Shadows ---

        void DirectionalLightFeatureProcessor::SetShadowEnabled(LightHandle handle, bool enable)
        {
            m_shadowingLightHandle.Reset();
            if (enable)
            {
                m_shadowingLightHandle = handle;
                ShadowingDirectionalLightNotificationsBus::Broadcast(&ShadowingDirectionalLightNotifications::OnShadowingDirectionalLightChanged, handle);
                m_shadowBufferNeedsUpdate = true;
            }
        }

        void DirectionalLightFeatureProcessor::SetShadowmapSize(LightHandle handle, ShadowmapSize size)
        {
            for (auto& it : m_shadowData)
            {
                it.second.GetData(handle.GetIndex()).m_shadowmapSize = aznumeric_cast<uint32_t>(size);
            }
            SetShadowmapImageSizeArraySize(handle);
        }

        void DirectionalLightFeatureProcessor::SetCascadeCount(LightHandle handle, uint16_t cascadeCount)
        {
            AZ_Assert(cascadeCount <= Shadow::MaxNumberOfCascades, "cascadeCount is out of range.");
            for (auto& it : m_shadowData)
            {
                it.second.GetData(handle.GetIndex()).m_cascadeCount = cascadeCount;
            }
            m_shadowBufferNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::SetShadowmapFrustumSplitSchemeRatio(LightHandle handle, float ratio)
        {
            ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());
            property.m_isShadowmapFrustumSplitAutomatic = true;
            property.m_shadowmapFrustumSplitSchemeRatio = ratio;
            property.m_borderDepthsForSegmentsNeedsUpdate = true;
            property.m_shadowmapViewNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::SetCascadeFarDepth(LightHandle handle, uint16_t cascadeIndex, float farDepth)
        {
            ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());
            property.m_isShadowmapFrustumSplitAutomatic = false;
            property.m_borderDepthsForSegmentsNeedsUpdate = true;
            property.m_shadowmapViewNeedsUpdate = true;

            AZ_Warning("DirectionaLightFeatureProcessor", cascadeIndex < Shadow::MaxNumberOfCascades, "The cascade index is out of bounds.");
            if (cascadeIndex < Shadow::MaxNumberOfCascades)
            {
                property.m_defaultFarDepths[cascadeIndex] = farDepth;
            }
        }

        void DirectionalLightFeatureProcessor::SetCameraConfiguration(
            LightHandle handle,
            const Camera::Configuration& baseCameraConfiguration,
            const RPI::RenderPipelineId& renderPipelineId)
        {
            ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());
            auto update = [&property, &baseCameraConfiguration](const RPI::View* view)
            {
                CascadeShadowCameraConfiguration& cameraConfig = property.m_cameraConfigurations[view];
                if (!cameraConfig.HasSameConfiguration(baseCameraConfiguration))
                {
                    cameraConfig.SetBaseConfiguration(baseCameraConfiguration);
                    cameraConfig.SetShadowDepthFar(property.m_shadowDepthFar);
                }
            };

            if (RPI::RenderPipeline* renderPipeline = GetParentScene()->GetRenderPipeline(renderPipelineId).get())
            {
                const RPI::View* cameraView = renderPipeline->GetDefaultView().get();
                update(cameraView);
            }
            else
            {
                update(nullptr);
            }
            property.m_frustumNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::SetCameraTransform(
            LightHandle handle,
            const Transform&,
            const RPI::RenderPipelineId&)
        {
            ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());
            property.m_shadowmapViewNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::SetShadowFarClipDistance(LightHandle handle, float farDist)
        {
            ShadowProperty& property = GetShadowProperty(handle);
            property.m_shadowDepthFar = farDist;
            for (auto& it : property.m_cameraConfigurations)
            {
                it.second.SetShadowDepthFar(farDist);
            }
            property.m_borderDepthsForSegmentsNeedsUpdate = true;
            property.m_frustumNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::SetGroundHeight(LightHandle handle, float groundHeight)
        {
            ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());
            property.m_groundHeight = groundHeight;
            property.m_shadowmapViewNeedsUpdate = property.m_isViewFrustumCorrectionEnabled;
        }

        void DirectionalLightFeatureProcessor::SetViewFrustumCorrectionEnabled(LightHandle handle, bool enabled)
        {
            ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());
            property.m_isViewFrustumCorrectionEnabled = enabled;
            property.m_shadowmapViewNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::SetDebugFlags(LightHandle handle, DirectionalLightFeatureProcessorInterface::DebugDrawFlags flags)
        {
            for (auto& it : m_shadowData)
            {
                it.second.GetData(handle.GetIndex()).m_debugFlags = flags;
            }
            m_shadowBufferNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::SetShadowFilterMethod(LightHandle handle, ShadowFilterMethod method)
        {
            m_shadowProperties.GetData(handle.GetIndex()).m_shadowFilterMethod = method;
            for (auto& dataIt : m_shadowData)
            {
                dataIt.second.GetData(handle.GetIndex()).m_shadowFilterMethod = aznumeric_cast<uint32_t>(method);
            }
            m_shadowBufferNeedsUpdate = true;

            if (handle == m_shadowingLightHandle)
            {
                for (const auto& it : m_esmShadowmapsPasses)
                {
                    for (EsmShadowmapsPass* esmPass : it.second)
                    {
                        esmPass->SetEnabledComputation(
                            method == ShadowFilterMethod::Esm ||
                            method == ShadowFilterMethod::EsmPcf);
                    }
                }
            }
        }

        void DirectionalLightFeatureProcessor::SetFilteringSampleCount(LightHandle handle, uint16_t count)
        {
            if (count > Shadow::MaxPcfSamplingCount)
            {
                AZ_Warning(FeatureProcessorName, false, "Sampling count exceed the limit.");
                count = Shadow::MaxPcfSamplingCount;
            }

            //Remap the count value to an enum value associated with that count.
            ShadowFilterSampleCount samplingCountMode = ShadowFilterSampleCount::PcfTap16;
            if (count <= 4)
            {
                samplingCountMode = ShadowFilterSampleCount::PcfTap4;
            }
            else if (count <= 9)
            {
                samplingCountMode = ShadowFilterSampleCount::PcfTap9;
            }

            for (auto& it : m_shadowData)
            {
                it.second.GetData(handle.GetIndex()).m_filteringSampleCountMode = static_cast<uint32_t>(samplingCountMode);
            }
            m_shadowBufferNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::SetShadowReceiverPlaneBiasEnabled(LightHandle handle, bool enable)
        {
            m_shadowProperties.GetData(handle.GetIndex()).m_isReceiverPlaneBiasEnabled = enable;
        }

        void DirectionalLightFeatureProcessor::SetCascadeBlendingEnabled(LightHandle handle, bool enable)
        {
            m_shadowProperties.GetData(handle.GetIndex()).m_blendBetweenCascades = enable;
        }

        void DirectionalLightFeatureProcessor::SetShadowBias(LightHandle handle, float bias) 
        {
            for (auto& it : m_shadowData) 
            {
                it.second.GetData(handle.GetIndex()).m_shadowBias = bias;               
            }
            m_shadowBufferNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::SetNormalShadowBias(LightHandle handle, float normalShadowBias)
        {
            for (auto& it : m_shadowData)
            {
                it.second.GetData(handle.GetIndex()).m_normalShadowBias = normalShadowBias;
            }
            m_shadowBufferNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::SetFullscreenBlurEnabled(LightHandle handle, bool enable)
        {
            ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());
            property.m_fullscreenBlurEnabled = enable;
        }

        void DirectionalLightFeatureProcessor::SetFullscreenBlurConstFalloff(LightHandle handle, float blurConstFalloff)
        {
            ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());
            property.m_fullscreenBlurConstFalloff = blurConstFalloff;
        }

        void DirectionalLightFeatureProcessor::SetFullscreenBlurDepthFalloffStrength(LightHandle handle, float blurDepthFalloffStrength)
        {
            ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());
            property.m_fullscreenBlurDepthFalloffStrength = blurDepthFalloffStrength;
        }

        void DirectionalLightFeatureProcessor::SetAffectsGI(LightHandle handle, bool affectsGI)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DirectionalLightFeatureProcessor::SetAffectsGI().");

            m_lightData.GetData(handle.GetIndex()).m_affectsGI = affectsGI;
            m_lightBufferNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::SetAffectsGIFactor(LightHandle handle, float affectsGIFactor)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DirectionalLightFeatureProcessor::SetAffectsGIFactor().");

            m_lightData.GetData(handle.GetIndex()).m_affectsGIFactor = affectsGIFactor;
            m_lightBufferNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::SetLightingChannelMask(LightHandle handle, uint32_t lightingChannelMask)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DirectionalLightFeatureProcessor::SetLightingChannelMask().");

            m_lightData.GetData(handle.GetIndex()).m_lightingChannelMask = lightingChannelMask;
            m_lightBufferNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::OnRenderPipelineChanged([[maybe_unused]] RPI::RenderPipeline* pipeline,
            RPI::SceneNotification::RenderPipelineChangeType changeType)
        {
            if (changeType == RPI::SceneNotification::RenderPipelineChangeType::Added
                || changeType == RPI::SceneNotification::RenderPipelineChangeType::PassChanged)
            {
                PrepareForChangingRenderPipelineAndCameraView();
            }
            else if (changeType == RPI::SceneNotification::RenderPipelineChangeType::Removed)
            {
                if (m_cascadedShadowmapsPasses.find(pipeline->GetId()) != m_cascadedShadowmapsPasses.end()
                    || m_esmShadowmapsPasses.find(pipeline->GetId()) != m_esmShadowmapsPasses.end())
                {
                    PrepareForChangingRenderPipelineAndCameraView();
                }
            }
        }

        void DirectionalLightFeatureProcessor::OnRenderPipelinePersistentViewChanged(RPI::RenderPipeline*, RPI::PipelineViewTag, RPI::ViewPtr, RPI::ViewPtr)
        {
            PrepareForChangingRenderPipelineAndCameraView();
        }

        void DirectionalLightFeatureProcessor::PrepareForChangingRenderPipelineAndCameraView() 
        {
            CacheFullscreenPass();
            CacheCascadedShadowmapsPass();
            CacheEsmShadowmapsPass();
            PrepareCameraViews();
            PrepareShadowBuffers();
            CacheRenderPipelineIdsForPersistentView();
            SetConfigurationToPasses();
            SetCameraViewNameToPass();
            UpdateViewsOfCascadeSegments();
        }

        void DirectionalLightFeatureProcessor::CacheCascadedShadowmapsPass()
        {
            m_cascadedShadowmapsPasses.clear();

            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithTemplateName(Name("CascadedShadowmapsTemplate"), GetParentScene());
            RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                {
                    RPI::RenderPipeline* pipeline = pass->GetRenderPipeline();
                    const RPI::RenderPipelineId pipelineId = pipeline->GetId();

                    CascadedShadowmapsPass* shadowPass = azrtti_cast<CascadedShadowmapsPass*>(pass);
                    AZ_Assert(shadowPass, "It is not a CascadedShadowmapPass.");
                    if (pipeline->GetDefaultView())
                    {
                        m_cascadedShadowmapsPasses[pipelineId].push_back(shadowPass);
                    }
                    return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                });
        }

        void DirectionalLightFeatureProcessor::CacheFullscreenPass()
        {
            m_fullscreenShadowPass = nullptr;
            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithTemplateName(Name("FullscreenShadowTemplate"), GetParentScene());
            RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                {
                    RPI::RenderPipeline* pipeline = pass->GetRenderPipeline();
                    const RPI::RenderPipelineId pipelineId = pipeline->GetId();

                    FullscreenShadowPass* shadowPass = azrtti_cast<FullscreenShadowPass*>(pass);
                    AZ_Assert(shadowPass, "It is not a FullscreenShadowPass.");
                    if (pipeline->GetDefaultView())
                    {
                        m_fullscreenShadowPass = shadowPass;
                    }
                    return RPI::PassFilterExecutionFlow::StopVisitingPasses;
                });

            m_fullscreenShadowBlurPass = nullptr;
            RPI::PassFilter blurPassFilter = RPI::PassFilter::CreateWithPassName(Name("FullscreenShadowBlur"), GetParentScene());
            RPI::PassSystemInterface::Get()->ForEachPass(blurPassFilter, [this](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                {
                    RPI::RenderPipeline* pipeline = pass->GetRenderPipeline();
                    const RPI::RenderPipelineId pipelineId = pipeline->GetId();

                    RPI::ParentPass* fullscreenShadowBlurPass = azrtti_cast<RPI::ParentPass*>(pass);
                    if (pipeline->GetDefaultView())
                    {
                        m_fullscreenShadowBlurPass = fullscreenShadowBlurPass;
                    }
                    return RPI::PassFilterExecutionFlow::StopVisitingPasses;
                });
        }

        void DirectionalLightFeatureProcessor::CacheEsmShadowmapsPass()
        {
            m_esmShadowmapsPasses.clear();

            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithTemplateName(Name("EsmShadowmapsTemplate"), GetParentScene());
            RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                {
                    const RPI::RenderPipelineId pipelineId = pass->GetRenderPipeline()->GetId();

                    if (m_cascadedShadowmapsPasses.find(pipelineId) != m_cascadedShadowmapsPasses.end())
                    {
                        EsmShadowmapsPass* esmPass = azrtti_cast<EsmShadowmapsPass*>(pass);
                        AZ_Assert(esmPass, "It is not an EsmShadowmapPass.");
                        if (esmPass->GetLightTypeName() == m_lightTypeName)
                        {
                            m_esmShadowmapsPasses[pipelineId].push_back(esmPass);
                        }
                    }
                    return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                });
        }

        void DirectionalLightFeatureProcessor::PrepareCameraViews()
        {
            m_cameraViewNames.clear();
            m_cameraViewNames[nullptr] = "Null Camera View"; // it makes a placeholder for the null camera view.
            AZStd::vector<const RPI::View*> cameraViews;
            for (const auto& passIt : m_cascadedShadowmapsPasses)
            {
                const RPI::RenderPipelineId& pipelineId = passIt.first;
                for (const CascadedShadowmapsPass* pass : passIt.second)
                {
                    if (RPI::View* cameraView = pass->GetRenderPipeline()->GetDefaultView().get())
                    {
                        cameraViews.push_back(cameraView);
                        if (m_cameraViewNames.find(cameraView) == m_cameraViewNames.end())
                        {
                            m_cameraViewNames[cameraView] = AZStd::string::format(
                                "%s_%s",
                                cameraView->GetName().GetCStr(),
                                pipelineId.GetCStr());
                        }
                    }
                }
            }
            
            // Remove unnecessary camera views in shadow properties
            auto& shadowPropertiesVector = m_shadowProperties.GetDataVector();
            for (ShadowProperty& shadowProperty : shadowPropertiesVector)
            {
                auto& cascades = shadowProperty.m_segments;
                for (auto it = cascades.begin(); it != cascades.end();)
                {
                    if (AZStd::find(cameraViews.begin(), cameraViews.end(), it->first) != cameraViews.end())
                    {
                        ++it;
                    }
                    else
                    {
                        it = cascades.erase(it);
                    }
                }
            }

            // Remove retaining AuxGeomDraw for camera views.
            if (m_auxGeomFeatureProcessor)
            {
                for (const RPI::View* cameraView : m_viewsRetainingAuxGeomDraw)
                {
                    m_auxGeomFeatureProcessor->ReleaseDrawQueueForView(cameraView);
                }
                m_viewsRetainingAuxGeomDraw.clear();
            }
        }

        void DirectionalLightFeatureProcessor::PrepareShadowBuffers()
        {
            // This function is called only when camera view is changed.
            // When the change happens frequently, creation of a new buffer handler
            // for the new camera view can happen before destruction of
            // the buffer handler for the old camera view with the same name.
            // So bumping of buffer handler name is required as below.

            const auto removeIfNotOccur = [](auto& unorderedMap, const AZStd::vector<const RPI::View*> activeViews)
            {
                for (auto handlerIt = unorderedMap.begin(); handlerIt != unorderedMap.end(); )
                {
                    if (handlerIt->first == nullptr ||
                        AZStd::find(activeViews.begin(), activeViews.end(), handlerIt->first) != activeViews.end())
                    {
                        ++handlerIt;
                    }
                    else
                    {
                        handlerIt = unorderedMap.erase(handlerIt);
                    }
                }
            };

            const RHI::ShaderResourceGroupLayout* viewSrgLayout = RPI::RPISystemInterface::Get()->GetViewSrgLayout().get();
            const IndexedDataVector<DirectionalLightShadowData> lastShadowData = m_shadowData[nullptr];
            IndexedDataVector<EsmShadowmapsPass::FilterParameter> lastEsmParameter = m_esmParameterData[nullptr];
            while (lastEsmParameter.GetDataCount() < Shadow::MaxNumberOfCascades)
            {
                lastEsmParameter.GetFreeSlotIndex(); // reserve placeholder for each cascade.
            }

            m_shadowData[nullptr] = lastShadowData;
            m_esmParameterData[nullptr] = lastEsmParameter;
            AZStd::vector<const RPI::View*> cameraViews;
            cameraViews.reserve(m_cascadedShadowmapsPasses.size());
            for (const auto& passIt : m_cascadedShadowmapsPasses)
            {
                const RPI::RenderPipelineId& pipelineId = passIt.first;
                const RPI::View* cameraView = passIt.second.front()->GetRenderPipeline()->GetDefaultView().get();
                if (!cameraView)
                {
                    continue;
                }

                cameraViews.push_back(cameraView);
                if (m_shadowBufferHandlers.find(cameraView) == m_shadowBufferHandlers.end())
                {
                    GpuBufferHandler::Descriptor desc;
                    desc.m_bufferName = AZStd::string::format("%s(%s)%d",
                        "DirectionalLightShadowBuffer",
                        pipelineId.GetCStr(),
                        m_shadowBufferNameIndex);
                    desc.m_bufferSrgName = "m_directionalLightShadows";
                    desc.m_elementCountSrgName = "m_directionalLightCount";
                    desc.m_elementSize = sizeof(DirectionalLightShadowData);
                    desc.m_srgLayout = viewSrgLayout;

                    m_shadowBufferHandlers[cameraView] = GpuBufferHandler(desc);
                }
                if (m_shadowData.find(cameraView) == m_shadowData.end())
                {
                    m_shadowData[cameraView] = lastShadowData;
                }
            }
            removeIfNotOccur(m_shadowBufferHandlers, cameraViews);
            removeIfNotOccur(m_shadowData, cameraViews);

            cameraViews.clear();
            for (const auto& passIt : m_esmShadowmapsPasses)
            {
                const RPI::RenderPipelineId& pipelineId = passIt.first;
                const RPI::View* cameraView = passIt.second.front()->GetRenderPipeline()->GetDefaultView().get();
                if (!cameraView)
                {
                    continue;
                }

                cameraViews.push_back(cameraView);
                if (m_esmParameterBufferHandlers.find(cameraView) == m_esmParameterBufferHandlers.end())
                {
                    GpuBufferHandler::Descriptor desc;
                    desc.m_bufferName = AZStd::string::format("%s(%s)%d",
                        "EsmParameterBuffer(Directional)",
                        pipelineId.GetCStr(),
                        m_shadowBufferNameIndex);
                    desc.m_bufferSrgName = "m_esmsDirectional";
                    desc.m_elementCountSrgName = ""; // does not update count in SRG.
                    desc.m_elementSize = sizeof(EsmShadowmapsPass::FilterParameter);
                    desc.m_srgLayout = viewSrgLayout;

                    m_esmParameterBufferHandlers[cameraView] = GpuBufferHandler(desc);
                }
                if (m_esmParameterData.find(cameraView) == m_esmParameterData.end())
                {
                    m_esmParameterData[cameraView] = lastEsmParameter;
                }
            }
            removeIfNotOccur(m_esmParameterBufferHandlers, cameraViews);
            removeIfNotOccur(m_esmParameterData, cameraViews);

            ++m_shadowBufferNameIndex;
            m_shadowBufferNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::CacheRenderPipelineIdsForPersistentView()
        {
            m_renderPipelineIdsForPersistentView.clear();
            for (const auto& passIt : m_cascadedShadowmapsPasses)
            {
                const RPI::RenderPipeline* pipeline = passIt.second.front()->GetRenderPipeline();
                for (const auto& pipelineView : pipeline->GetPipelineViews())
                {
                    if (pipelineView.second.m_type == RPI::PipelineViewType::Persistent)
                    {
                        for (const RPI::ViewPtr& view : pipelineView.second.m_views)
                        {
                            m_renderPipelineIdsForPersistentView[view.get()].push_back(passIt.first);
                        }
                    }
                }
            }
        }

        void DirectionalLightFeatureProcessor::SetConfigurationToPasses()
        {
            if (m_shadowingLightHandle.IsNull())
            {
                return;
            }
            const ShadowProperty& property = m_shadowProperties.GetData(m_shadowingLightHandle.GetIndex());

            for (const auto& passIt : m_cascadedShadowmapsPasses)
            {
                RPI::RenderPipeline* pipeline = passIt.second.front()->GetRenderPipeline();
                const RPI::View* cameraView = pipeline->GetDefaultView().get();

                const ShadowProperty& property2 = m_shadowProperties.GetData(m_shadowingLightHandle.GetIndex());
                if (property2.m_segments.find(cameraView) != property2.m_segments.end())
                {
                    const size_t cascadeCount = property2.m_segments.at(cameraView).size();
                    if (cascadeCount > 0)
                    {
                        SetCascadeCount(m_shadowingLightHandle, aznumeric_cast<uint16_t>(cascadeCount));
                        break;
                    }
                }
            }

            SetShadowFilterMethod(m_shadowingLightHandle, property.m_shadowFilterMethod);
            SetShadowmapImageSizeArraySize(m_shadowingLightHandle);
        }

        void DirectionalLightFeatureProcessor::SleepShadowmapPasses()
        {
            for (const auto& it : m_cascadedShadowmapsPasses)
            {
                for (CascadedShadowmapsPass* pass : it.second)
                {
                    pass->SetShadowmapSize(ShadowmapSize::None, 1);
                }
            }
            for (const auto& it : m_esmShadowmapsPasses)
            {
                for (EsmShadowmapsPass* pass : it.second)
                {
                    pass->SetEnabledComputation(false);
                }
            }
        }

        uint16_t DirectionalLightFeatureProcessor::GetCascadeCount(LightHandle handle) const
        {
            const auto& segments = m_shadowProperties.GetData(handle.GetIndex()).m_segments;
            if (!segments.empty())
            {
                return aznumeric_cast<uint16_t>(segments.begin()->second.size());
            }
            return 0;
        }

        const CascadeShadowCameraConfiguration& DirectionalLightFeatureProcessor::GetCameraConfiguration(LightHandle handle, const RPI::View* cameraView) const
        {
            const ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());
            const auto findIt = property.m_cameraConfigurations.find(cameraView);
            if (findIt != property.m_cameraConfigurations.end())
            {
                return findIt->second;
            }
            return property.m_cameraConfigurations.at(nullptr);
        }

        void DirectionalLightFeatureProcessor::UpdateFrustums(LightHandle handle)
        {
            ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());
            for (const auto& segmentIt : property.m_segments)
            {
                const CascadeShadowCameraConfiguration& cameraConfig = GetCameraConfiguration(handle, segmentIt.first);

                // update entire frustum radius and center
                const float depthNear = cameraConfig.GetDepthNear();
                const float depthFar = cameraConfig.GetDepthFar();
                const float depthCenter = cameraConfig.GetDepthCenter(depthNear, depthFar);

                // The point (0, depthCenter, 0) is the center of the sphere S0
                // on which every vertices of the frustum are.  When the FoV is
                // sufficiently large, the frustum is contained in the hemisphere
                //     {(x,y,z) : x^2 + (y-dC)^2 + z^2 <= r^2, y < dC}
                // where dC is depthCenter and r is the radius of the sphere,
                // and so (0, dC, 0) becomes outside of the frustum.
                // In such a case, S0 is not the radius minimum sphere which
                // contains the frustum, and the radius minimum sphere S1
                // has the center point at (0, depthFar, 0).
                if (depthCenter < depthFar)
                {
                    // Then the local position of the center is (0, depthCenter, 0).
                    // The radius minimum sphere is S0.
                    // In this case, we let dc = depthCenter
                    // since the distances from (0, dC, 0) to each vertex are same.
                    // We consider a vertex on the far plane to calculate 
                    // the radius r as below:
                    //  r^2 = (dF - dC)^2 + (dF tan(fov/2))^2 + (dF tan(fov/2) ar)^2
                    //      = (dF - dC)^2 + dF^2 (depthCenterRatio - 1)
                    // where dF is depthFar, fov is FoVY, ar is aspectRatio,
                    // and depthCenterRatio is CameraConfiguration::m_depthCenterRatio.
                    const float r2 = (depthFar - depthCenter) * (depthFar - depthCenter) +
                        depthFar * depthFar * (cameraConfig.GetDepthCenterRatio() - 1);
                    property.m_entireFrustumRadius = sqrtf(r2);
                    property.m_entireFrustumCenterLocal = Vector3::CreateAxisY(depthCenter);
                }
                else
                {
                    // Then the local position of the center is (0, depthFar, 0).
                    // The radius minimum sphere is S1.
                    // In this case, since the vertices on the near plane are
                    // inside of S1, we consider a vertex on the far plane
                    // to calculate the radius r as below:
                    //  r^2 = (dF tan(fov/2))^2 + (dF tan(fov/2) ar)^2
                    //      = (dF tan(fov/2) sqrt(1 + ar^2))^2
                    // where dF is depthFar, fov is FoVY, and ar is aspectRatio.
                    const float aspectRatio = cameraConfig.GetAspectRatio();
                    const float fovY = cameraConfig.GetFovY();
                    const float diagonalRatio = sqrtf(1 + aspectRatio * aspectRatio);
                    const float halfHeightRatio = tanf(fovY / 2);
                    property.m_entireFrustumRadius = depthFar * halfHeightRatio * diagonalRatio;
                    property.m_entireFrustumCenterLocal = Vector3::CreateAxisY(depthFar);
                }
            }

            property.m_borderDepthsForSegmentsNeedsUpdate = true;
            property.m_shadowmapViewNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::SetCameraViewNameToPass() const
        {
            if (m_cascadedShadowmapsPasses.empty())
            {
                return;
            }

            for (const auto& passIt : m_cascadedShadowmapsPasses)
            {
                CascadedShadowmapsPass* shadowPass = passIt.second.front();
                RPI::RenderPipeline* pipeline = shadowPass->GetRenderPipeline();
                RPI::View* cameraView = pipeline->GetDefaultView().get();
                shadowPass->SetCameraViewName(m_cameraViewNames.at(cameraView));
            }
        }

        void DirectionalLightFeatureProcessor::UpdateViewsOfCascadeSegments(LightHandle handle, uint16_t cascadeCount)
        {
            if (m_cascadedShadowmapsPasses.empty())
            {
                return;
            }

            ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());
            for (const auto& passIt : m_cascadedShadowmapsPasses)
            {
                CascadedShadowmapsPass* shadowPass = passIt.second.front();
                const AZStd::span<const RPI::PipelineViewTag>& viewTags = shadowPass->GetPipelineViewTags();
                AZ_Assert(viewTags.size() >= cascadeCount, "DirectionalLightFeatureProcessor: There is not enough pipeline view tags.");

                RPI::RenderPipeline* pipeline = shadowPass->GetRenderPipeline();
                RPI::View* cameraView = pipeline->GetDefaultView().get();
                AZ_Assert(cameraView, "The default view of the pipeline is null.");
                property.m_segments[cameraView].resize(cascadeCount);
                for (size_t index = 0; index < cascadeCount; ++index)
                {
                    CascadeSegment& segment = property.m_segments.at(cameraView)[index];
                    const RPI::PipelineViewTag& viewTag = viewTags[index];
                    const Name viewName{
                        AZStd::string::format("DirLightShadowView (cascade: %zu, LightHandle: %d)", index, handle.GetIndex()) };

                    segment.m_pipelineViewTag = viewTag;
                    if (!segment.m_view || segment.m_view->GetName() != viewName)
                    {
                        RPI::View::UsageFlags usageFlags = RPI::View::UsageShadow;

                        // if the shadow is rendering in an EnvironmentCubeMapPass it also needs to be a ReflectiveCubeMap view,
                        // to filter out shadows from objects that are excluded from the cubemap
                        RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassClass<RPI::EnvironmentCubeMapPass>();
                        passFilter.SetOwnerScene(GetParentScene()); // only handles passes for this scene
                        RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [&usageFlags]([[maybe_unused]] RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                            {
                                usageFlags |= RPI::View::UsageReflectiveCubeMap;
                                return RPI::PassFilterExecutionFlow::StopVisitingPasses;
                            });

                        segment.m_view = RPI::View::CreateView(viewName, usageFlags);
                        segment.m_view->SetShadowPassRenderPipelineId(pipeline->GetId());
                    }
                }
            }

            property.m_borderDepthsForSegmentsNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::SetShadowmapImageSizeArraySize(LightHandle handle)
        {
            if (m_cascadedShadowmapsPasses.empty())
            {
                return;
            }

            ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());
            for (const auto& segmentIt : property.m_segments)
            {
                DirectionalLightShadowData& shadowData = m_shadowData.at(segmentIt.first).GetData(handle.GetIndex());
                const u16 numCascades = aznumeric_cast<u16>(segmentIt.second.size());

                // [GFX TODO][ATOM-2012] shadow for multiple directional lights
                if (handle == m_shadowingLightHandle && numCascades > 0)
                {
                    const ShadowmapSize shadowmapSize = aznumeric_cast<ShadowmapSize>(shadowData.m_shadowmapSize);
                    for (const auto& it : m_cascadedShadowmapsPasses)
                    {
                        for (CascadedShadowmapsPass* pass : it.second)
                        {
                            pass->SetShadowmapSize(shadowmapSize, numCascades);
                        }
                    }
                }
            }

            property.m_borderDepthsForSegmentsNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::UpdateFilterParameters(LightHandle handle)
        {
            if (handle != m_shadowingLightHandle)
            {
                return;
            }

            for (const auto& passIt : m_esmShadowmapsPasses)
            {
                const RPI::View* cameraView = passIt.second.front()->GetRenderPipeline()->GetDefaultView().get();
                UpdateFilterEnabled(handle, cameraView);
                UpdateShadowmapPositionInAtlas(handle, cameraView);
                SetFilterParameterToPass(handle, cameraView);
            }
        }

        void DirectionalLightFeatureProcessor::UpdateFilterEnabled(LightHandle handle, const RPI::View* cameraView)
        {
            if (handle != m_shadowingLightHandle)
            {
                return;
            }

            const DirectionalLightShadowData& shadowData = m_shadowData.at(cameraView).GetData(handle.GetIndex());
            if (shadowData.m_shadowFilterMethod == aznumeric_cast<uint32_t>(ShadowFilterMethod::Esm) ||
                (shadowData.m_shadowFilterMethod == aznumeric_cast<uint32_t>(ShadowFilterMethod::EsmPcf)))
            {
                // Write filter offsets and filter counts to ESM data
                for (uint16_t index = 0; index < GetCascadeCount(handle); ++index)
                {
                    EsmShadowmapsPass::FilterParameter& filterParameter = m_esmParameterData.at(cameraView).GetData(index);
                    filterParameter.m_isEnabled = true;
                }
            }
            else
            {
                // If ESM is not used, set filter offsets and filter counts zero in ESM data.
                for (uint16_t index = 0; index < GetCascadeCount(handle); ++index)
                {
                    EsmShadowmapsPass::FilterParameter& filterParameter = m_esmParameterData.at(cameraView).GetData(index);
                    filterParameter.m_isEnabled = false;
                }
            }
        }
        
        void DirectionalLightFeatureProcessor::UpdateShadowmapPositionInAtlas(LightHandle handle, const RPI::View* cameraView)
        {
            if (handle != m_shadowingLightHandle)
            {
                return;
            }

            // Get shadowmap size of the camera view.
            const DirectionalLightShadowData& data = m_shadowData.at(cameraView).GetData(handle.GetIndex());
            const uint32_t shadowmapSize = data.m_shadowmapSize;

            // Set shadowmap origin and shadowmap size to ESM data.
            // Note that the same size of shadowmap is used for a directional light.
            for (uint16_t index = 0; index < Shadow::MaxNumberOfCascades; ++index)
            {
                EsmShadowmapsPass::FilterParameter& filterParameter = m_esmParameterData.at(cameraView).GetData(index);
                filterParameter.m_shadowmapOriginInSlice = { {0, 0} };
                filterParameter.m_shadowmapSize = shadowmapSize;
            }
        }

        void DirectionalLightFeatureProcessor::SetFilterParameterToPass(LightHandle handle, const RPI::View* cameraView)
        {
            AZ_PROFILE_SCOPE(RPI, "DirectionalLightFeatureProcessor::SetFilterParameterToPass");

            if (handle != m_shadowingLightHandle)
            {
                return;
            }

            // Update ESM parameter buffer which is attached to
            // both of Forward Pass and ESM Shadowmaps Pass.
            auto itr = m_esmParameterBufferHandlers.find(cameraView);
            if (itr != m_esmParameterBufferHandlers.end())
            {
                itr->second.UpdateBuffer(m_esmParameterData.at(cameraView).GetDataVector());
            }

            // Create index table buffer.
            const RPI::RenderPipelineId cameraPipelineId = m_renderPipelineIdsForPersistentView.at(cameraView).front();
            const ShadowmapAtlas& atlas = m_cascadedShadowmapsPasses.at(cameraPipelineId).front()->GetShadowmapAtlas();
            const AZStd::string indexTableBufferName =
                AZStd::string::format("IndexTableBuffer(Directional) %d",
                    m_shadowmapIndexTableBufferNameIndex++);
            const Data::Instance<RPI::Buffer> indexTableBuffer = atlas.CreateShadowmapIndexTableBuffer(indexTableBufferName);

            // Set index table buffer and ESM parameter buffer to ESM pass.
            for (const RPI::RenderPipelineId& pipelineId : m_renderPipelineIdsForPersistentView.at(cameraView))
            {
                for (EsmShadowmapsPass* esmPass : m_esmShadowmapsPasses.at(pipelineId))
                {
                    esmPass->SetShadowmapIndexTableBuffer(indexTableBuffer);
                    esmPass->SetFilterParameterBuffer(m_esmParameterBufferHandlers.at(cameraView).GetBuffer());
                }
            }
        }

        void DirectionalLightFeatureProcessor::UpdateBorderDepthsForSegments(LightHandle handle)
        {
            ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());
            if (property.m_isShadowmapFrustumSplitAutomatic)
            {
                const float ratio = property.m_shadowmapFrustumSplitSchemeRatio;
                AZ_Assert(ratio >= 0.f && ratio <= 1.f, "Cascade splitting scheme ratio is not between 0 and 1.");
                for (auto& segmentIt : property.m_segments)
                {
                    const uint16_t cascadeCount = aznumeric_cast<uint16_t>(segmentIt.second.size());
                    const CascadeShadowCameraConfiguration& cameraConfig = GetCameraConfiguration(handle, segmentIt.first);
                    const float nearD = cameraConfig.GetDepthNear();
                    const float farD = cameraConfig.GetDepthFar();

                    AZ_Assert(cascadeCount > 0, "Number of cascades must be positive.");
                    for (uint16_t index = 0; index < cascadeCount - 1; ++index)
                    {
                        const float uniD = nearD + (farD - nearD) * (index + 1) / cascadeCount;
                        const float logD = nearD * powf(farD / nearD, 1.f * (index + 1) / cascadeCount);
                        const float segFarD = ratio * logD + (1 - ratio) * uniD;
                        segmentIt.second[index].m_borderFarDepth = segFarD;
                    }
                    segmentIt.second[cascadeCount - 1].m_borderFarDepth = farD;
                }
            }
            else
            {
                for (auto& segmentIt : property.m_segments)
                {
                    const uint16_t cascadeCount = aznumeric_cast<uint16_t>(segmentIt.second.size());
                    float farDepth = 0.f;
                    for (uint16_t index = 0; index < cascadeCount; ++index)
                    {
                        farDepth = GetMax(farDepth, property.m_defaultFarDepths[index]);
                        segmentIt.second[index].m_borderFarDepth = farDepth;
                    }
                }
            }

            property.m_shadowmapViewNeedsUpdate = true;
        }

        float DirectionalLightFeatureProcessor::GetShadowmapSizeFromCameraView(const LightHandle handle, const RPI::View* cameraView) const
        {
            const DirectionalLightShadowData& shadowData = m_shadowData.at(cameraView).GetData(handle.GetIndex());
            return static_cast<float>(shadowData.m_shadowmapSize);
        }

        void DirectionalLightFeatureProcessor::SnapAabbToPixelIncrements(const float invShadowmapSize, Vector3& orthoMin, Vector3& orthoMax)
        {
            // This function stops the cascaded shadowmap from shimmering as the camera moves.
            // See CascadedShadowsManager.cpp in the Microsoft CascadedShadowMaps11 sample for details.

            const Vector3 normalizeByBufferSize = Vector3(invShadowmapSize, invShadowmapSize, invShadowmapSize);

            const Vector3 worldUnitsPerTexel = (orthoMax - orthoMin) * normalizeByBufferSize;

            // We snap the camera to 1 pixel increments so that moving the camera does not cause the shadows to jitter.
            // This is a matter of dividing by the world space size of a texel
            orthoMin /= worldUnitsPerTexel;
            orthoMin = orthoMin.GetFloor();
            orthoMin *= worldUnitsPerTexel;

            orthoMax /= worldUnitsPerTexel;
            orthoMax = orthoMax.GetFloor();
            orthoMax *= worldUnitsPerTexel;
        }

        void DirectionalLightFeatureProcessor::UpdateShadowmapViews(LightHandle handle)
        {
            ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());

            const DirectionalLightData light = m_lightData.GetData(handle.GetIndex());
            static const Vector3 position = Vector3::CreateZero();
            const Vector3 direction = Vector3::CreateFromFloat3(light.m_direction.data());
            const auto lightTransform = Matrix3x4::CreateLookAt(position, position + direction);

            for (auto& segmentIt : property.m_segments)
            {
                const float invShadowmapSize = 1.0f / GetShadowmapSizeFromCameraView(handle, segmentIt.first);

                Vector3 previousAabbMin = Vector3::CreateZero();
                Vector3 previousAabbMax = Vector3::CreateZero();
                float previousNear = 0.0f;
                float previousFar = 0.0f;

                for (uint16_t cascadeIndex = 0; cascadeIndex < segmentIt.second.size(); ++cascadeIndex)
                {
                    const Aabb viewAabb = CalculateShadowViewAabb(handle, segmentIt.first, cascadeIndex, lightTransform);

                    if (viewAabb.IsValid() && viewAabb.IsFinite())
                    {
                        const float cascadeNear = viewAabb.GetMin().GetY();
                        const float cascadeFar = viewAabb.GetMax().GetY();

                        Vector3 snappedAabbMin = viewAabb.GetMin();
                        Vector3 snappedAabbMax = viewAabb.GetMax();

                        SnapAabbToPixelIncrements(invShadowmapSize, snappedAabbMin, snappedAabbMax);

                        Matrix4x4 viewToClipMatrix = Matrix4x4::CreateIdentity();
                        MakeOrthographicMatrixRH(
                            viewToClipMatrix, snappedAabbMin.GetElement(0), snappedAabbMax.GetElement(0), snappedAabbMin.GetElement(2),
                            snappedAabbMax.GetElement(2), cascadeNear, cascadeFar);

                        CascadeSegment& segment = segmentIt.second[cascadeIndex];
                        segment.m_aabb = viewAabb;
                        segment.m_view->SetCameraTransform(lightTransform);
                        segment.m_view->SetViewToClipMatrix(viewToClipMatrix);

                        if (cascadeIndex > 0 && r_excludeItemsInSmallerShadowCascades)
                        {
                            // Build a matrix (which will be turned into a frustum during culling) to exclude items completely
                            // contained in the previous cascade.

                            Vector3 excludeAabbMin = previousAabbMin;
                            Vector3 excludeAabbMax = previousAabbMax;

                            if (property.m_blendBetweenCascades)
                            {
                                // Adjust the size of the exclude matrix to be slightly smaller due to the blend region.
                                Vector3 previousAabbDiff = previousAabbMin - previousAabbMax;
                                previousAabbDiff *= CascadeBlendArea;
                                excludeAabbMin += previousAabbDiff;
                                excludeAabbMax -= previousAabbDiff;
                            }

                            MakeOrthographicMatrixRH(
                                viewToClipMatrix, excludeAabbMin.GetElement(0), excludeAabbMax.GetElement(0), excludeAabbMin.GetElement(2),
                                excludeAabbMax.GetElement(2), previousNear, previousFar);

                            segment.m_view->SetViewToClipExcludeMatrix(&viewToClipMatrix);
                        }
                        else
                        {
                            segment.m_view->SetViewToClipExcludeMatrix(nullptr);
                        }
                        previousAabbMin = snappedAabbMin;
                        previousAabbMax = snappedAabbMax;
                        previousNear = cascadeNear;
                        previousFar = cascadeFar;
                    }
                }
            }
        }

        void DirectionalLightFeatureProcessor::UpdateViewsOfCascadeSegments()
        {
            if (m_shadowingLightHandle.IsValid())
            {
                const uint16_t cascadeCount = GetCascadeCount(m_shadowingLightHandle);
                UpdateViewsOfCascadeSegments(m_shadowingLightHandle, cascadeCount);
            }
        }

        Aabb DirectionalLightFeatureProcessor::CalculateShadowViewAabb(
            LightHandle handle,
            const RPI::View* cameraView,
            uint16_t cascadeIndex,
            const Matrix3x4& lightTransform)
        {
            ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());

            // The least detailed segment is not corrected.
            const bool shouldBeCorrected =
                (property.m_isViewFrustumCorrectionEnabled &&
                    cascadeIndex < GetCascadeCount(handle) - 1);

            float depthNear = 0.f;
            float depthFar = 0.f;
            GetDepthNearFar(handle, cameraView, cascadeIndex, depthNear, depthFar);
            const Vector3 boundaryCenterWorld =
                (shouldBeCorrected) ?
                CalculateCorrectedWorldCenterPosition(handle, cameraView, depthNear, depthFar) :
                GetWorldCenterPosition(handle, cameraView, depthNear, depthFar);
            const Matrix3x4 lightTransformInverse = lightTransform.GetInverseFast();
            const Vector3 boundaryCenterLight = lightTransformInverse * boundaryCenterWorld;

            const float boundaryRadius = GetRadius(handle, cameraView, depthNear, depthFar);
            const Vector3 radiusDiff{
                boundaryRadius,
                boundaryRadius,
                boundaryRadius };
            Vector3 minPoint = boundaryCenterLight - radiusDiff;
            Vector3 maxPoint = boundaryCenterLight + radiusDiff;

            // [GFX TODO][ATOM-2495] consider shadow caster outside of camera view frustum
            // For Y-direction (forward), the AABB must cover
            // from A to B, where A is the nearest point in the entire camera view frustum
            // from the light origin and B is the farthest point in the segment
            // from the light origin.
            // There are points outside of a boundary sphere of a segment
            // but on the light path which pass through the boundary sphere.
            // If we used an AABB whose Y-direction range is from a segment,
            // the depth value on the shadowmap saturated to 0 or 1,
            // and we could not draw shadow correctly.
            const Transform cameraTransform = cameraView->GetCameraTransform();
            const Vector3 entireFrustumCenterLight =
                lightTransform.GetInverseFast() * (cameraTransform.TransformPoint(property.m_entireFrustumCenterLocal));
            const float entireCenterY = entireFrustumCenterLight.GetElement(1);
            const Vector3 cameraLocationWorld = cameraTransform.GetTranslation();
            const Vector3 cameraLocationLight = lightTransformInverse * cameraLocationWorld;
            // Extend light view frustum by camera depth far in order to avoid shadow lacking behind camera.
            const float cameraBehindMinY = cameraLocationLight.GetElement(1) - GetCameraConfiguration(handle, cameraView).GetDepthFar();
            float minYSegment = minPoint.GetElement(1);
            float maxYSegment = maxPoint.GetElement(1);
            const float minY = AZStd::GetMin(minYSegment,
                AZStd::GetMin(entireCenterY - property.m_entireFrustumRadius, cameraBehindMinY));
            minPoint.SetElement(1, minY);

            // Set parameter to convert to emphasize from minYSegment to maxYSegment
            // to mitigate Peter-Panning.
            auto itr = m_esmParameterData.find(cameraView);
            if (itr != m_esmParameterData.end())
            {
                itr->second.GetData(cascadeIndex).m_lightDistanceOfCameraViewFrustum =
                    (minYSegment - minY) / (maxYSegment - minY);
            }

            // Set coefficient of slope bias to remove shadow acne.
            // Slope bias is shadowmapTexelDiameter * tan(theta) / depthRange
            // where theta is the angle between light direction and
            // the inverse of surface normal.
            DirectionalLightShadowData& shadowData = m_shadowData.at(cameraView).GetData(handle.GetIndex());
            const float shadowmapTexelWidth = boundaryRadius * 2 / shadowData.m_shadowmapSize;
            const float shadowmapTexelDiameter = shadowmapTexelWidth * sqrtf(2.f);
            const float depthRange = maxYSegment - minY;
            shadowData.m_slopeBiasBase[cascadeIndex] = shadowmapTexelDiameter / depthRange;

            return Aabb::CreateFromMinMax(minPoint, maxPoint);
        }

        void DirectionalLightFeatureProcessor::GetDepthNearFar(
            LightHandle handle,
            const RPI::View* cameraView,
            uint16_t cascadeIndex,
            float& outDepthNear,
            float& outDepthFar) const
        {
            const ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());
            outDepthNear = (cascadeIndex == 0) ?
                GetCameraConfiguration(handle, cameraView).GetDepthNear() :
                property.m_segments.at(cameraView)[cascadeIndex - 1].m_borderFarDepth;

            outDepthFar = property.m_segments.at(cameraView)[cascadeIndex].m_borderFarDepth;
        }

        void DirectionalLightFeatureProcessor::GetDepthNearFar(LightHandle handle, const RPI::View* cameraView, float& outDepthNear, float& outDepthFar) const
        {
            float unused;
            GetDepthNearFar(handle, cameraView, 0, outDepthNear, unused);
            GetDepthNearFar(handle, cameraView, GetCascadeCount(handle) - 1, unused, outDepthFar);                
        }

        Vector3 DirectionalLightFeatureProcessor::GetWorldCenterPosition(
            LightHandle handle,
            const RPI::View* cameraView,
            float depthNear,
            float depthFar) const
        {
            const float depthCenter = AZStd::GetMin<float>(
                GetCameraConfiguration(handle, cameraView).GetDepthCenter(depthNear, depthFar),
                depthFar);

            const Vector3 localCenter{ 0.f, depthCenter, 0.f };            
            return cameraView->GetCameraTransform().TransformPoint(localCenter);
        }

        float DirectionalLightFeatureProcessor::GetRadius(
            LightHandle handle,
            const RPI::View* cameraView,
            float depthNear,
            float depthFar) const
        {
            const CascadeShadowCameraConfiguration& cameraConfig =
                GetCameraConfiguration(handle, cameraView);
            const float depthCenter = cameraConfig.GetDepthCenter(depthNear, depthFar);
            if (depthCenter < depthFar)
            {
                // Then the local position of the center is (0, depthCenter, 0).
                // The distance between the center and any vertex is the radius.
                const float r2 = (depthFar - depthCenter) * (depthFar - depthCenter) +
                    depthFar * depthFar * (cameraConfig.GetDepthCenterRatio() - 1);
                return sqrtf(r2);
            }
            else
            {
                // Then the local position of the center is (0, depthFar, 0).
                // The distance between the center and a vertex on the far depth plane
                // is the radius.
                const float fovY = cameraConfig.GetFovY();
                const float aspectRatio = cameraConfig.GetAspectRatio();
                return depthFar * tanf(fovY / 2) *
                    sqrtf(1 + aspectRatio * aspectRatio);
            }
        }

        Vector3 DirectionalLightFeatureProcessor::CalculateCorrectedWorldCenterPosition(
            LightHandle handle,
            const RPI::View* cameraView,
            float depthNear,
            float depthFar) const
        {
            // This calculates the center of bounding sphere for a camera view frustum.
            // By this, on the camera view (2D), the bounding sphere's center
            // shifts to the remarkable point.
            // We define the remarkable point by the middle of the bottom line
            // of the camera view.
            //       +----------------------------------+
            //       |                                  |
            //       |           camera view            |
            //       |                                  |
            //       +----------------@-----------------+
            //                       the remarkable point
            // We assume the normal vector of the ground is (0, 0, 1) and
            // the camera height is given in m_cameraHeight in this correction.
            const ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());
            const Vector3& boundaryCenter = GetWorldCenterPosition(handle, cameraView, depthNear, depthFar);
            const CascadeShadowCameraConfiguration& cameraConfiguration = GetCameraConfiguration(handle, cameraView);
            const Transform cameraTransform = cameraView->GetCameraTransform();
            const Vector3& cameraFwd = cameraTransform.GetBasis(1);
            const Vector3& cameraUp = cameraTransform.GetBasis(2);
            const Vector3 cameraToBoundaryCenter = boundaryCenter - cameraTransform.GetTranslation();
            const float cameraDepthNear = cameraConfiguration.GetDepthNear();

            // 1. Calculation of cameraDiffVecFromCamera,
            // which is the vector on XY plane
            // from camera view point to the remarkable point on the ground
            // which we can see at the middle of the bottom line of the camera viewport.

            // direction from camera viewpoint to the remarkable point.
            Vector3 lowVec = (cameraFwd - cameraUp * tanf(cameraConfiguration.GetFovY() / 2.f));
            lowVec.Normalize();
            const float lowVecZ = lowVec.GetElement(2);
            // If camera is pointing upward and ground goes outside of view, skip correction.
            if (lowVecZ >= 0.f)
            {
                return boundaryCenter;
            }

            // Difference between camera position and the remarkable point on the ground.
            const float cameraHeight = cameraTransform.GetTranslation().GetElement(2) - property.m_groundHeight;
            float cameraDiffLen = GetMax(cameraHeight / -lowVecZ, 0.f);
            const float distanceToBoundaryCenter = cameraToBoundaryCenter.GetLength();
            // The remarkable point should not further than the boundary center.
            cameraDiffLen = GetMin(cameraDiffLen, distanceToBoundaryCenter);
            Vector3 cameraDiffVec = lowVec * cameraDiffLen;

            // Project the vector onto the XY plane.
            cameraDiffVec.SetElement(2, 0.f);

            // 2. Calculation of centerDiffVecFromCamera = A to B,
            // where A is the Z-projection of the camera view point to the ground
            // and B is the light direction projection of
            // the bounding sphere's center to the ground.

            // direction of the light.
            const DirectionalLightData& light = m_lightData.GetData(handle.GetIndex());
            const Vector3 lightDir = Vector3::CreateFromFloat3(light.m_direction.data());
            const float lightDirZ = lightDir.GetElement(2);
            // If light is pointing upwards or straight down, skip correction
            if (lightDirZ <= -1.0f || lightDirZ >= 0.0f)
            {
                return boundaryCenter;
            }

            // Height of the center of the boundary sphere from the ground.
            const float centerHeight = GetMax<float>(boundaryCenter.GetElement(2) - property.m_groundHeight, 0.f);

            // Difference between boundaryCenter and it's projected position on the ground plane
            const Vector3 centerDiff = lightDir * (centerHeight / -lightDirZ);

            Vector3 centerDiffVec = cameraToBoundaryCenter + centerDiff;

            // Project the vector onto the XY plane.
            centerDiffVec.SetElement(2, 0.f);

            // 3. correction of center position
            const Vector3 slippage = centerDiffVec - cameraDiffVec;
            float slippageScale = 1.f;
            if (cameraDiffLen < cameraDepthNear)
            {
                slippageScale = cameraDiffLen / cameraDepthNear;
            }
            return boundaryCenter - slippage * slippageScale;
        }

        void DirectionalLightFeatureProcessor::SetShadowParameterToShadowData(LightHandle handle)
        {
            // [GFX TODO][ATOM-2012] shadow for multiple directional lights
            if (handle != m_shadowingLightHandle)
            {
                return;
            }

            const ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());
            for (const auto& it : m_cascadedShadowmapsPasses)
            {
                const RPI::View* cameraView = it.second.front()->GetRenderPipeline()->GetDefaultView().get();
                DirectionalLightShadowData& shadowData = m_shadowData.at(cameraView).GetData(handle.GetIndex());

                for (uint16_t cascadeIndex = 0; cascadeIndex < GetCascadeCount(handle); ++cascadeIndex)
                {
                    const Matrix4x4& lightViewToLightClipMatrix = property.m_segments.at(cameraView)[cascadeIndex].m_view->GetViewToClipMatrix();
                    const Matrix4x4 lightViewToShadowmapMatrix = Shadow::GetClipToShadowmapTextureMatrix() * lightViewToLightClipMatrix;
                    shadowData.m_lightViewToShadowmapMatrices[cascadeIndex] = lightViewToShadowmapMatrix;
                    shadowData.m_worldToLightViewMatrices[cascadeIndex] = property.m_segments.at(cameraView)[cascadeIndex].m_view->GetWorldToViewMatrix();
                }

                float nearDepth, farDepth;
                GetDepthNearFar(handle, cameraView, nearDepth, farDepth);
                shadowData.m_far_minus_near = farDepth - nearDepth;
            }

            m_shadowBufferNeedsUpdate = true;
        }

        void DirectionalLightFeatureProcessor::DrawCascadeBoundingBoxes(LightHandle handle)
        {
            if (!m_auxGeomFeatureProcessor)
            {
                return;
            }

            static const Color colors[Shadow::MaxNumberOfCascades] = { Colors::Red, Colors::Green, Colors::Blue, Colors::Yellow };

            if (handle != m_shadowingLightHandle)
            {
                return;
            }

            for (const auto& shadowIt : m_shadowData)
            {
                const RPI::View* cameraView = shadowIt.first;
                if (!cameraView || (shadowIt.second.GetData(handle.GetIndex()).m_debugFlags & DebugDrawFlags::DebugDrawBoundingBoxes) == 0)
                {
                    continue;
                }

                RPI::AuxGeomDrawPtr auxGeom = m_auxGeomFeatureProcessor->GetDrawQueueForView(cameraView);
                if (!auxGeom)
                {
                    auxGeom = m_auxGeomFeatureProcessor->GetOrCreateDrawQueueForView(cameraView);
                    m_viewsRetainingAuxGeomDraw.push_back(cameraView);
                }

                const DirectionalLightData light = m_lightData.GetData(handle.GetIndex());
                const auto direction = Vector3::CreateFromFloat3(light.m_direction.data());
                const auto transformOrigin = Matrix3x4::CreateLookAt(Vector3::CreateZero(), direction);

                const ShadowProperty& property = m_shadowProperties.GetData(handle.GetIndex());
                for (uint16_t cascadeIndex = 0; cascadeIndex < GetCascadeCount(handle); ++cascadeIndex)
                {
                    const CascadeSegment& segment = property.m_segments.at(cameraView)[cascadeIndex];
                    const Aabb& aabb = segment.m_aabb;
                    if (!aabb.IsValid() || !aabb.IsFinite())
                    {
                        continue;
                    }

                    const Matrix4x4& viewToWorldMatrix = segment.m_view->GetViewToWorldMatrix();
                    // This converts view space (Y-up) to world space (Z-up).
                    const Vector3 axisX = viewToWorldMatrix.GetColumnAsVector3(0);
                    const Vector3 axisY = -viewToWorldMatrix.GetColumnAsVector3(2);
                    const Vector3 axisZ = viewToWorldMatrix.GetColumnAsVector3(1);

                    const AZ::Quaternion rotation = AZ::Quaternion::CreateFromMatrix3x3(
                        AZ::Matrix3x3::CreateFromColumns(axisX, axisY, axisZ));

                    Vector3 lightLocation = aabb.GetCenter();
                    lightLocation.SetY(aabb.GetMin().GetY());
                    const Vector3 center = transformOrigin * lightLocation + axisY * aabb.GetYExtent() / 2.f;
                    const auto obb = Obb::CreateFromPositionRotationAndHalfLengths(
                        center, rotation, aabb.GetExtents() / 2.0f);
                    auxGeom->DrawObb(obb, Vector3::CreateZero(), colors[cascadeIndex], RPI::AuxGeomDraw::DrawStyle::Line);
                }
            }
        }

        void DirectionalLightFeatureProcessor::SetFullscreenPassSettings()
        {
            ShadowProperty& shadowProperty = m_shadowProperties.GetData(m_shadowingLightHandle.GetIndex());

            if (m_fullscreenShadowPass)
            {
                const auto& shadowData = m_shadowData.at(nullptr).GetData(m_shadowingLightHandle.GetIndex());
                const uint32_t shadowFilterMethod = shadowData.m_shadowFilterMethod;
                const uint32_t filteringSampleCountMode = shadowData.m_filteringSampleCountMode;
                const uint32_t cascadeCount = shadowData.m_cascadeCount;
                m_fullscreenShadowPass->SetLightRawIndex(m_shadowProperties.GetRawIndex(m_shadowingLightHandle.GetIndex()));
                m_fullscreenShadowPass->SetBlendBetweenCascadesEnable(cascadeCount > 1 && shadowProperty.m_blendBetweenCascades);
                m_fullscreenShadowPass->SetFilterMethod(static_cast<ShadowFilterMethod>(shadowFilterMethod));
                m_fullscreenShadowPass->SetFilteringSampleCountMode(static_cast<ShadowFilterSampleCount>(filteringSampleCountMode));
                m_fullscreenShadowPass->SetReceiverShadowPlaneBiasEnable(shadowProperty.m_isReceiverPlaneBiasEnabled);
            }

            if (m_fullscreenShadowBlurPass)
            {
                bool fullscreenBlurEnabled = shadowProperty.m_fullscreenBlurEnabled;
                m_fullscreenShadowBlurPass->SetEnabled(fullscreenBlurEnabled);

                if(fullscreenBlurEnabled)
                {
                    RPI::Pass* child_0 = m_fullscreenShadowBlurPass->FindChildPass(Name("VerticalBlur")).get();
                    RPI::Pass* child_1 = m_fullscreenShadowBlurPass->FindChildPass(Name("HorizontalBlur")).get();

                    FastDepthAwareBlurVerPass* verBlurPass = azrtti_cast<FastDepthAwareBlurVerPass*>(child_0);
                    FastDepthAwareBlurHorPass* horBlurPass = azrtti_cast<FastDepthAwareBlurHorPass*>(child_1);

                    AZ_Assert(verBlurPass != nullptr, "Could not find vertical blur on fullscreen shadow blur pass");
                    AZ_Assert(horBlurPass != nullptr, "Could not find horizontal blur on fullscreen shadow blur pass");

                    const float depthThreshold = 0.0f;

                    verBlurPass->SetConstants(shadowProperty.m_fullscreenBlurConstFalloff, depthThreshold,
                                              shadowProperty.m_fullscreenBlurDepthFalloffStrength);

                    horBlurPass->SetConstants(shadowProperty.m_fullscreenBlurConstFalloff, depthThreshold,
                                              shadowProperty.m_fullscreenBlurDepthFalloffStrength);
                }
            }
        }

    } // namespace Render
} // namespace AZ
