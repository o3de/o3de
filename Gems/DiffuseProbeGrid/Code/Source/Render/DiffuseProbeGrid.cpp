/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/RayTracing/RayTracingFeatureProcessorInterface.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Render/DiffuseProbeGrid.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/Factory.h>
#include <AzCore/Math/MathUtils.h>

namespace AZ
{
    namespace Render
    {
        DiffuseProbeGrid::DiffuseProbeGrid()
            : m_textureReadback(this)
        {
        }

        DiffuseProbeGrid::~DiffuseProbeGrid()
        {
            m_scene->GetCullingScene()->UnregisterCullable(m_cullable);
        }

        void DiffuseProbeGrid::Init(RPI::Scene* scene, DiffuseProbeGridRenderData* renderData)
        {
            AZ_Assert(scene, "DiffuseProbeGrid::Init called with a null Scene pointer");

            m_scene = scene;
            m_renderData = renderData;

            // create attachment Ids
            AZStd::string uuidString = AZ::Uuid::CreateRandom().ToString<AZStd::string>();
            m_rayTraceImageAttachmentId = AZStd::string::format("ProbeRayTraceImageAttachmentId_%s", uuidString.c_str());
            m_irradianceImageAttachmentId = AZStd::string::format("ProbeIrradianceImageAttachmentId_%s", uuidString.c_str());
            m_distanceImageAttachmentId = AZStd::string::format("ProbeDistanceImageAttachmentId_%s", uuidString.c_str());
            m_probeDataImageAttachmentId = AZStd::string::format("ProbeDataImageAttachmentId_%s", uuidString.c_str());
            m_gridDataBufferAttachmentId = AZStd::string::format("ProbeGridDataBufferAttachmentId_%s", uuidString.c_str());
            m_visualizationTlasAttachmentId = AZStd::string::format("ProbeVisualizationTlasAttachmentId_%s", uuidString.c_str());
            m_visualizationTlasInstancesAttachmentId = AZStd::string::format("ProbeVisualizationTlasInstancesAttachmentId_%s", uuidString.c_str());

            // setup culling
            m_cullable.SetDebugName(AZ::Name("DiffuseProbeGrid Volume"));

            // create the visualization TLAS
            m_visualizationTlas = aznew RHI::RayTracingTlas;

            // create the grid data buffer
            m_gridDataBuffer = aznew RHI::Buffer;

            RHI::BufferDescriptor descriptor;
            descriptor.m_byteCount = DiffuseProbeGridRenderData::GridDataBufferSize;
            descriptor.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite;

            RHI::BufferInitRequest request;
            request.m_buffer = m_gridDataBuffer.get();
            request.m_descriptor = descriptor;
            [[maybe_unused]] RHI::ResultCode result = m_renderData->m_bufferPool->InitBuffer(request);
            AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize GridData buffer");
        }

        void DiffuseProbeGrid::Simulate(uint32_t probeIndex)
        {
            UpdateTextures();

            if (m_renderObjectSrg)
            {
                // the list index passed in from the feature processor is the index of this probe in the sorted probe list.
                // this is needed to render the probe volumes in order from largest to smallest
                RHI::DrawItemSortKey sortKey = static_cast<RHI::DrawItemSortKey>(probeIndex);
                if (sortKey != m_sortKey)
                {
                    if (m_renderData->m_pipelineState->GetRHIPipelineState())
                    {
                        // the sort key changed, rebuild draw packets
                        m_sortKey = sortKey;

                        RHI::DrawPacketBuilder drawPacketBuilder{RHI::MultiDevice::AllDevices};
                        drawPacketBuilder.Begin(nullptr);
                        drawPacketBuilder.SetGeometryView(&m_renderData->m_geometryView);
                        drawPacketBuilder.AddShaderResourceGroup(m_renderObjectSrg->GetRHIShaderResourceGroup());

                        RHI::DrawPacketBuilder::DrawRequest drawRequest;
                        drawRequest.m_streamIndices = m_renderData->m_geometryView.GetFullStreamBufferIndices();
                        drawRequest.m_listTag = m_renderData->m_drawListTag;
                        drawRequest.m_pipelineState = m_renderData->m_pipelineState->GetRHIPipelineState();
                        drawRequest.m_sortKey = m_sortKey;
                        drawPacketBuilder.AddDrawItem(drawRequest);

                        m_drawPacket = drawPacketBuilder.End();

                        // we also need to update culling with the new draw packet
                        UpdateCulling();
                    }
                }
            }

            m_probeRayRotation = AZ::Quaternion::CreateIdentity();
            m_frameUpdateIndex = (m_frameUpdateIndex + 1) % m_frameUpdateCount;
        }

        bool DiffuseProbeGrid::ValidateProbeSpacing(const AZ::Vector3& newSpacing)
        {
            return ValidateProbeCount(m_extents, newSpacing);
        }

        void DiffuseProbeGrid::SetProbeSpacing(const AZ::Vector3& probeSpacing)
        {
            // remove previous spacing from the render extents
            m_renderExtents -= m_probeSpacing;

            // update probe spacing
            m_probeSpacing = probeSpacing;

            // expand the extents by one probe spacing unit in order to blend properly around the edges of the volume
            m_renderExtents += m_probeSpacing;

            m_obbWs = Obb::CreateFromPositionRotationAndHalfLengths(m_transform.GetTranslation(), m_transform.GetRotation(), m_renderExtents / 2.0f);

            // recompute the number of probes since the spacing changed
            UpdateProbeCount();

            m_updateTextures = true;
        }

        void DiffuseProbeGrid::SetViewBias(float viewBias)
        {
            m_viewBias = viewBias;
            m_updateRenderObjectSrg = true;
        }

        void DiffuseProbeGrid::SetNormalBias(float normalBias)
        {
            m_normalBias = normalBias;
            m_updateRenderObjectSrg = true;
        }

        void DiffuseProbeGrid::SetNumRaysPerProbe(DiffuseProbeGridNumRaysPerProbe numRaysPerProbe)
        {
            m_numRaysPerProbe = numRaysPerProbe;
            m_updateTextures = true;
        }

        void DiffuseProbeGrid::SetTransform(const AZ::Transform& transform)
        {
            m_transform = transform;

            m_obbWs = Obb::CreateFromPositionRotationAndHalfLengths(m_transform.GetTranslation(), m_transform.GetRotation(), m_renderExtents / 2.0f);

            // probes need to be relocated since the grid position changed
            m_remainingRelocationIterations = DefaultNumRelocationIterations;

            m_updateRenderObjectSrg = true;
        }

        bool DiffuseProbeGrid::ValidateExtents(const AZ::Vector3& newExtents)
        {
            return ValidateProbeCount(newExtents, m_probeSpacing);
        }

        void DiffuseProbeGrid::SetExtents(const AZ::Vector3& extents)
        {
            m_extents = extents;

            // recompute the number of probes since the extents changed
            UpdateProbeCount();

            // expand the extents by one probe spacing unit in order to blend properly around the edges of the volume
            m_renderExtents = m_extents + m_probeSpacing;

            m_obbWs = Obb::CreateFromPositionRotationAndHalfLengths(m_transform.GetTranslation(), m_transform.GetRotation(), m_renderExtents / 2.0f);

            m_updateTextures = true;
        }

        void DiffuseProbeGrid::SetAmbientMultiplier(float ambientMultiplier)
        {
            m_ambientMultiplier = ambientMultiplier;
            m_updateRenderObjectSrg = true;
        }

        void DiffuseProbeGrid::Enable(bool enabled)
        {
            m_enabled = enabled;
            m_updateRenderObjectSrg = true;
        }

        void DiffuseProbeGrid::SetMode(DiffuseProbeGridMode mode)
        {
            // handle auto-select
            if (mode == DiffuseProbeGridMode::AutoSelect)
            {
                m_mode = (RHI::RHISystemInterface::Get()->GetRayTracingSupport() != RHI::MultiDevice::NoDevices) ? DiffuseProbeGridMode::RealTime : DiffuseProbeGridMode::Baked;
            }
            else
            {
                m_mode = mode;
            }

            m_updateTextures = true;
        }

        void DiffuseProbeGrid::SetScrolling(bool scrolling)
        {
            if (m_scrolling == scrolling)
            {
                return;
            }

            m_scrolling = scrolling;

            // probes need to be relocated since the scrolling mode changed
            m_remainingRelocationIterations = DefaultNumRelocationIterations;

            m_gridDataInitialized = false;
        }

        void DiffuseProbeGrid::SetEdgeBlendIbl(bool edgeBlendIbl)
        {
            if (m_edgeBlendIbl == edgeBlendIbl)
            {
                return;
            }

            m_edgeBlendIbl = edgeBlendIbl;

            m_updateRenderObjectSrg = true;
        }

        void DiffuseProbeGrid::SetBakedTextures(const DiffuseProbeGridBakedTextures& bakedTextures)
        {
            AZ_Assert(bakedTextures.m_irradianceImage.get(), "Invalid Irradiance image passed to SetBakedTextures");
            AZ_Assert(bakedTextures.m_distanceImage.get(), "Invalid Distance image passed to SetBakedTextures");
            AZ_Assert(bakedTextures.m_probeDataImage.get(), "Invalid ProbeData image passed to SetBakedTextures");

            m_bakedIrradianceImage = bakedTextures.m_irradianceImage;
            m_bakedDistanceImage = bakedTextures.m_distanceImage;
            m_bakedProbeDataImage = bakedTextures.m_probeDataImage;

            m_bakedIrradianceRelativePath = bakedTextures.m_irradianceImageRelativePath;
            m_bakedDistanceRelativePath = bakedTextures.m_distanceImageRelativePath;
            m_bakedProbeDataRelativePath = bakedTextures.m_probeDataImageRelativePath;

            m_updateTextures = true;
        }

        bool DiffuseProbeGrid::HasValidBakedTextures() const
        {
            return m_bakedIrradianceImage.get() &&
                m_bakedDistanceImage.get() &&
                m_bakedProbeDataImage.get();
        }

        void DiffuseProbeGrid::ResetCullingVisibility()
        {
            m_cullable.m_isVisible = false;
        }

        bool DiffuseProbeGrid::GetIsVisible() const
        {
            // we need to go through the DiffuseProbeGrid passes at least once in order to initialize
            // the RenderObjectSrg, which means we need to be visible until the RenderObjectSrg is created
            if (m_renderObjectSrg == nullptr)
            {
                return true;
            }

            // if a bake is in progress we need to make this DiffuseProbeGrid visible
            if (!m_textureReadback.IsIdle())
            {
                return true;
            }

            return m_cullable.m_isVisible;
        }

        void DiffuseProbeGrid::SetVisualizationEnabled(bool visualizationEnabled)
        {
            m_visualizationEnabled = visualizationEnabled;
            m_visualizationTlasUpdateRequired = true;
        }

        void DiffuseProbeGrid::SetVisualizationSphereRadius(float visualizationSphereRadius)
        {
            m_visualizationSphereRadius = visualizationSphereRadius;
            m_visualizationTlasUpdateRequired = true;
        }

        bool DiffuseProbeGrid::GetVisualizationTlasUpdateRequired() const
        {
            return m_visualizationTlasUpdateRequired || m_remainingRelocationIterations > 0;
        }

        bool DiffuseProbeGrid::ContainsPosition(const AZ::Vector3& position) const
        {
            return m_obbWs.Contains(position);
        }

        uint32_t DiffuseProbeGrid::GetTotalProbeCount() const
        {
            return m_probeCountX * m_probeCountY * m_probeCountZ;
        }

        // compute probe counts for a 2D texture layout
        void DiffuseProbeGrid::GetTexture2DProbeCount(uint32_t& probeCountX, uint32_t& probeCountY) const
        {
            // z-up left-handed
            probeCountX = m_probeCountY * m_probeCountZ;
            probeCountY = m_probeCountX;
        }

        void DiffuseProbeGrid::UpdateTextures()
        {
            if (!m_updateTextures)
            {
                return;
            }

            uint32_t probeCountX;
            uint32_t probeCountY;
            GetTexture2DProbeCount(probeCountX, probeCountY);

            if (m_mode == DiffuseProbeGridMode::RealTime)
            {
                AZStd::vector<uint8_t> initData;
                auto initImage = [&](const RHI::ImageInitRequest& request)
                {
                    initData.resize(
                        request.m_descriptor.m_size.m_width * request.m_descriptor.m_size.m_height *
                            RHI::GetFormatSize(request.m_descriptor.m_format),
                        0);
                    RHI::ImageUpdateRequest updateRequest;
                    updateRequest.m_image = request.m_image;
                    updateRequest.m_image->GetSubresourceLayout(updateRequest.m_sourceSubresourceLayout);
                    updateRequest.m_sourceData = initData.data();
                    m_renderData->m_imagePool->UpdateImageContents(updateRequest);
                };
                // advance to the next image in the frame image array
                m_currentImageIndex = (m_currentImageIndex + 1) % ImageFrameCount;

                // probe raytrace
                {
                    uint32_t width = GetNumRaysPerProbe().m_rayCount;
                    uint32_t height = GetTotalProbeCount();

                    m_rayTraceImage[m_currentImageIndex] = aznew RHI::Image;

                    RHI::ImageInitRequest request;
                    request.m_image = m_rayTraceImage[m_currentImageIndex].get();
                    request.m_descriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyRead, width, height, DiffuseProbeGridRenderData::RayTraceImageFormat);
                    [[maybe_unused]] RHI::ResultCode result = m_renderData->m_imagePool->InitImage(request);
                    AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize m_probeRayTraceImage image");
                    m_rayTraceImage[m_currentImageIndex]->SetName(AZ::Name("ProbeRaytrace"));
                    initImage(request);
                }

                // probe irradiance
                {
                    uint32_t width = probeCountX * (DefaultNumIrradianceTexels + 2);
                    uint32_t height = probeCountY * (DefaultNumIrradianceTexels + 2);

                    m_irradianceImage[m_currentImageIndex] = aznew RHI::Image;

                    RHI::ImageInitRequest request;
                    request.m_image = m_irradianceImage[m_currentImageIndex].get();
                    request.m_descriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyRead, width, height, DiffuseProbeGridRenderData::IrradianceImageFormat);
                    RHI::ClearValue clearValue = RHI::ClearValue::CreateVector4Float(0.0f, 0.0f, 0.0f, 0.0f);
                    request.m_optimizedClearValue = &clearValue;
                    [[maybe_unused]] RHI::ResultCode result = m_renderData->m_imagePool->InitImage(request);
                    AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize m_probeIrradianceImage image");
                    m_irradianceImage[m_currentImageIndex]->SetName(AZ::Name("ProbeIrradiance"));
                    initImage(request);
                }

                // probe distance
                {
                    uint32_t width = probeCountX * (DefaultNumDistanceTexels + 2);
                    uint32_t height = probeCountY * (DefaultNumDistanceTexels + 2);

                    m_distanceImage[m_currentImageIndex] = aznew RHI::Image;

                    RHI::ImageInitRequest request;
                    request.m_image = m_distanceImage[m_currentImageIndex].get();
                    request.m_descriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyRead, width, height, DiffuseProbeGridRenderData::DistanceImageFormat);
                    [[maybe_unused]] RHI::ResultCode result = m_renderData->m_imagePool->InitImage(request);
                    AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize m_probeDistanceImage image");
                    m_distanceImage[m_currentImageIndex]->SetName(AZ::Name("ProbeDistance"));
                    initImage(request);
                }

                // probe data
                {
                    uint32_t width = probeCountX;
                    uint32_t height = probeCountY;

                    m_probeDataImage[m_currentImageIndex] = aznew RHI::Image;

                    RHI::ImageInitRequest request;
                    request.m_image = m_probeDataImage[m_currentImageIndex].get();
                    request.m_descriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyRead, width, height, DiffuseProbeGridRenderData::ProbeDataImageFormat);
                    [[maybe_unused]] RHI::ResultCode result = m_renderData->m_imagePool->InitImage(request);
                    AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize m_probeDataImage image");
                    m_probeDataImage[m_currentImageIndex]->SetName(AZ::Name("ProbeData"));
                    initImage(request);
                }

                // probes need to be relocated since the textures changed
                m_remainingRelocationIterations = DefaultNumRelocationIterations;
            }

            m_updateTextures = false;

            // textures have changed so we need to update the render Srg to bind the new ones
            m_updateRenderObjectSrg = true;
        }

        void DiffuseProbeGrid::ComputeProbeCount(const AZ::Vector3& extents, const AZ::Vector3& probeSpacing, uint32_t& probeCountX, uint32_t& probeCountY, uint32_t& probeCountZ)
        {
            probeCountX = aznumeric_cast<uint32_t>(AZStd::floorf(extents.GetX() / probeSpacing.GetX()));
            probeCountY = aznumeric_cast<uint32_t>(AZStd::floorf(extents.GetY() / probeSpacing.GetY()));
            probeCountZ = aznumeric_cast<uint32_t>(AZStd::floorf(extents.GetZ() / probeSpacing.GetZ()));
        }

        bool DiffuseProbeGrid::ValidateProbeCount(const AZ::Vector3& extents, const AZ::Vector3& probeSpacing)
        {
            uint32_t probeCountX = 0;
            uint32_t probeCountY = 0;
            uint32_t probeCountZ = 0;
            ComputeProbeCount(extents, probeSpacing, probeCountX, probeCountY, probeCountZ);
            uint32_t totalProbeCount = probeCountX * probeCountY * probeCountZ;

            if (totalProbeCount == 0)
            {
                return false;
            }

            // radiance texture height is equal to the probe count
            if (totalProbeCount > MaxTextureDimension)
            {
                return false;
            }

            // distance texture uses the largest number of texels per probe
            // z-up left-handed
            uint32_t width = probeCountY * probeCountZ * (DefaultNumDistanceTexels + 2);
            uint32_t height = probeCountX * (DefaultNumDistanceTexels + 2);

            if (width > MaxTextureDimension || height > MaxTextureDimension)
            {
                return false;
            }

            return true;
        }

        void DiffuseProbeGrid::UpdateProbeCount()
        {
            ComputeProbeCount(m_extents,
                              m_probeSpacing,
                              m_probeCountX,
                              m_probeCountY,
                              m_probeCountZ);
        }

        void DiffuseProbeGrid::UpdatePrepareSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
        {
            if (!m_prepareSrg)
            {
                m_prepareSrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
                AZ_Error("DiffuseProbeGrid", m_prepareSrg.get(), "Failed to create Prepare shader resource group");
            }
        
            // scale the normal bias based on the grid density to reduce artifacts on thin geometry, less density results in more bias
            float scaledNormalBias = m_normalBias + 0.15f * (m_probeSpacing.GetMaxElement() / 2.0f);
        
            // scale the probe distance exponent based on the grid density to reduce artifacts on thin geometry
            static const float MinProbeDistanceExponent = 50.0f;
            float scaledProbeDistanceExponent = AZStd::max(m_probeDistanceExponent * (m_probeSpacing.GetMaxElement() / 1.5f), MinProbeDistanceExponent);
        
            // setup packed data
            uint32_t packed0 = m_probeCountX | (m_probeCountY << 8) | (m_probeCountZ << 16);
            uint32_t packed1 = aznumeric_cast<uint32_t>(m_probeRandomRayBackfaceThreshold * 65535) | (aznumeric_cast<uint32_t>(m_probeFixedRayBackfaceThreshold * 65535) << 16);
            uint32_t packed2 = GetNumRaysPerProbe().m_rayCount | (DefaultNumIrradianceTexels << 16) | (DefaultNumDistanceTexels << 24);
            uint32_t packed3 = 0;
            uint32_t packed4 = (m_scrolling << 16) | (1 << 17) | (1 << 18) | (1 << 19) | (1 << 20); // scrolling, rayFormat, irradianceFormat, relocation, classification

            m_prepareSrg->SetBufferView(m_renderData->m_prepareSrgGridDataNameIndex, m_gridDataBuffer->BuildBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());
            m_prepareSrg->SetConstant(m_renderData->m_prepareSrgGridDataInitializedNameIndex, m_gridDataInitialized);
            m_prepareSrg->SetConstant(m_renderData->m_prepareSrgProbeGridOriginNameIndex, m_transform.GetTranslation());
            m_prepareSrg->SetConstant(m_renderData->m_prepareSrgProbeGridProbeHysteresisNameIndex, m_probeHysteresis);
            m_prepareSrg->SetConstant(m_renderData->m_prepareSrgProbeGridRotationNameIndex, m_transform.GetRotation());
            m_prepareSrg->SetConstant(m_renderData->m_prepareSrgProbeGridProbeRayRotationNameIndex, m_probeRayRotation);
            m_prepareSrg->SetConstant(m_renderData->m_prepareSrgProbeGridProbeMaxRayDistanceNameIndex, m_probeMaxRayDistance);
            m_prepareSrg->SetConstant(m_renderData->m_prepareSrgProbeGridProbeNormalBiasNameIndex, scaledNormalBias);
            m_prepareSrg->SetConstant(m_renderData->m_prepareSrgProbeGridProbeViewBiasNameIndex, m_viewBias);
            m_prepareSrg->SetConstant(m_renderData->m_prepareSrgProbeGridProbeDistanceExponentNameIndex, scaledProbeDistanceExponent);
            m_prepareSrg->SetConstant(m_renderData->m_prepareSrgProbeGridProbeSpacingNameIndex, m_probeSpacing);           
            m_prepareSrg->SetConstant(m_renderData->m_prepareSrgProbeGridPacked0NameIndex, packed0);
            m_prepareSrg->SetConstant(m_renderData->m_prepareSrgProbeGridProbeIrradianceEncodingGammaNameIndex, m_probeIrradianceEncodingGamma);
            m_prepareSrg->SetConstant(m_renderData->m_prepareSrgProbeGridProbeIrradianceThresholdNameIndex, m_probeIrradianceThreshold);
            m_prepareSrg->SetConstant(m_renderData->m_prepareSrgProbeGridProbeBrightnessThresholdNameIndex, m_probeBrightnessThreshold);
            m_prepareSrg->SetConstant(m_renderData->m_prepareSrgProbeGridPacked1NameIndex, packed1);
            m_prepareSrg->SetConstant(m_renderData->m_prepareSrgProbeGridProbeMinFrontfaceDistanceNameIndex, m_probeMinFrontfaceDistance);
            m_prepareSrg->SetConstant(m_renderData->m_prepareSrgProbeGridPacked2NameIndex, packed2);
            m_prepareSrg->SetConstant(m_renderData->m_prepareSrgProbeGridPacked3NameIndex, packed3);
            m_prepareSrg->SetConstant(m_renderData->m_prepareSrgProbeGridPacked4NameIndex, packed4);
        
            m_gridDataInitialized = true;
        }

        void DiffuseProbeGrid::UpdateRayTraceSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
        {
            if (!m_rayTraceSrg)
            {
                m_rayTraceSrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
                AZ_Error("DiffuseProbeGrid", m_rayTraceSrg.get(), "Failed to create RayTrace shader resource group");
            }

            m_rayTraceSrg->SetBufferView(m_renderData->m_rayTraceSrgGridDataNameIndex, m_gridDataBuffer->BuildBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());
            m_rayTraceSrg->SetImageView(m_renderData->m_rayTraceSrgProbeRayTraceNameIndex, m_rayTraceImage[m_currentImageIndex]->BuildImageView(m_renderData->m_probeRayTraceImageViewDescriptor).get());
            m_rayTraceSrg->SetImageView(m_renderData->m_rayTraceSrgProbeIrradianceNameIndex, m_irradianceImage[m_currentImageIndex]->BuildImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());
            m_rayTraceSrg->SetImageView(m_renderData->m_rayTraceSrgProbeDistanceNameIndex, m_distanceImage[m_currentImageIndex]->BuildImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());
            m_rayTraceSrg->SetImageView(m_renderData->m_rayTraceSrgProbeDataNameIndex, m_probeDataImage[m_currentImageIndex]->BuildImageView(m_renderData->m_probeDataImageViewDescriptor).get());
            m_rayTraceSrg->SetConstant(m_renderData->m_rayTraceSrgAmbientMultiplierNameIndex, m_ambientMultiplier);
            m_rayTraceSrg->SetConstant(m_renderData->m_rayTraceSrgGiShadowsNameIndex, m_giShadows);
            m_rayTraceSrg->SetConstant(m_renderData->m_rayTraceSrgUseDiffuseIblNameIndex, m_useDiffuseIbl);
            m_rayTraceSrg->SetConstant(m_renderData->m_rayTraceSrgFrameUpdateCountNameIndex, m_frameUpdateCount);
            m_rayTraceSrg->SetConstant(m_renderData->m_rayTraceSrgFrameUpdateIndexNameIndex, m_frameUpdateIndex);
            m_rayTraceSrg->SetConstant(m_renderData->m_rayTraceSrgTransparencyModeNameIndex, aznumeric_cast<uint32_t>(m_transparencyMode));
            m_rayTraceSrg->SetConstant(m_renderData->m_rayTraceSrgEmissiveMultiplierNameIndex, m_emissiveMultiplier);
        }

        void DiffuseProbeGrid::UpdateBlendIrradianceSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
        {
            if (!m_blendIrradianceSrg)
            {
                m_blendIrradianceSrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
                AZ_Error("DiffuseProbeGrid", m_blendIrradianceSrg.get(), "Failed to create BlendIrradiance shader resource group");
            }

            m_blendIrradianceSrg->SetBufferView(m_renderData->m_blendIrradianceSrgGridDataNameIndex, m_gridDataBuffer->BuildBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());
            m_blendIrradianceSrg->SetImageView(m_renderData->m_blendIrradianceSrgProbeRayTraceNameIndex, m_rayTraceImage[m_currentImageIndex]->BuildImageView(m_renderData->m_probeRayTraceImageViewDescriptor).get());
            m_blendIrradianceSrg->SetImageView(m_renderData->m_blendIrradianceSrgProbeIrradianceNameIndex, m_irradianceImage[m_currentImageIndex]->BuildImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());
            m_blendIrradianceSrg->SetImageView(m_renderData->m_blendIrradianceSrgProbeDataNameIndex, m_probeDataImage[m_currentImageIndex]->BuildImageView(m_renderData->m_probeDataImageViewDescriptor).get());
            m_blendIrradianceSrg->SetConstant(m_renderData->m_blendIrradianceSrgFrameUpdateCountNameIndex, m_frameUpdateCount);
            m_blendIrradianceSrg->SetConstant(m_renderData->m_blendIrradianceSrgFrameUpdateIndexNameIndex, m_frameUpdateIndex);
        }

        void DiffuseProbeGrid::UpdateBlendDistanceSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
        {
            if (!m_blendDistanceSrg)
            {
                m_blendDistanceSrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
                AZ_Error("DiffuseProbeGrid", m_blendDistanceSrg.get(), "Failed to create BlendDistance shader resource group");
            }

            m_blendDistanceSrg->SetBufferView(m_renderData->m_blendDistanceSrgGridDataNameIndex, m_gridDataBuffer->BuildBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());
            m_blendDistanceSrg->SetImageView(m_renderData->m_blendDistanceSrgProbeRayTraceNameIndex, m_rayTraceImage[m_currentImageIndex]->BuildImageView(m_renderData->m_probeRayTraceImageViewDescriptor).get());
            m_blendDistanceSrg->SetImageView(m_renderData->m_blendDistanceSrgProbeDistanceNameIndex, m_distanceImage[m_currentImageIndex]->BuildImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());
            m_blendDistanceSrg->SetImageView(m_renderData->m_blendDistanceSrgProbeDataNameIndex, m_probeDataImage[m_currentImageIndex]->BuildImageView(m_renderData->m_probeDataImageViewDescriptor).get());
            m_blendDistanceSrg->SetConstant(m_renderData->m_blendDistanceSrgFrameUpdateCountNameIndex, m_frameUpdateCount);
            m_blendDistanceSrg->SetConstant(m_renderData->m_blendDistanceSrgFrameUpdateIndexNameIndex, m_frameUpdateIndex);
        }

        void DiffuseProbeGrid::UpdateBorderUpdateSrgs(
            const Data::Instance<RPI::Shader>& rowShader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& rowSrgLayout,
            const Data::Instance<RPI::Shader>& columnShader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& columnSrgLayout)
        {
            // border update row irradiance
            {
                if (!m_borderUpdateRowIrradianceSrg)
                {
                    m_borderUpdateRowIrradianceSrg = RPI::ShaderResourceGroup::Create(rowShader->GetAsset(), rowShader->GetSupervariantIndex(), rowSrgLayout->GetName());
                    AZ_Error("DiffuseProbeGrid", m_borderUpdateRowIrradianceSrg.get(), "Failed to create BorderUpdateRowIrradiance shader resource group");
                }

                m_borderUpdateRowIrradianceSrg->SetImageView(m_renderData->m_borderUpdateRowIrradianceSrgProbeTextureNameIndex, m_irradianceImage[m_currentImageIndex]->BuildImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());
                m_borderUpdateRowIrradianceSrg->SetConstant(m_renderData->m_borderUpdateRowIrradianceSrgNumTexelsNameIndex, DefaultNumIrradianceTexels);
            }

            // border update column irradiance
            {
                if (!m_borderUpdateColumnIrradianceSrg)
                {
                    m_borderUpdateColumnIrradianceSrg = RPI::ShaderResourceGroup::Create(columnShader->GetAsset(), columnShader->GetSupervariantIndex(), columnSrgLayout->GetName());
                    AZ_Error("DiffuseProbeGrid", m_borderUpdateColumnIrradianceSrg.get(), "Failed to create BorderUpdateColumnRowIrradiance shader resource group");
                }

                m_borderUpdateColumnIrradianceSrg->SetImageView(m_renderData->m_borderUpdateColumnIrradianceSrgProbeTextureNameIndex, m_irradianceImage[m_currentImageIndex]->BuildImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());
                m_borderUpdateColumnIrradianceSrg->SetConstant(m_renderData->m_borderUpdateColumnIrradianceSrgNumTexelsNameIndex, DefaultNumIrradianceTexels);
            }

            // border update row distance
            {
                if (!m_borderUpdateRowDistanceSrg)
                {
                    m_borderUpdateRowDistanceSrg = RPI::ShaderResourceGroup::Create(rowShader->GetAsset(), rowShader->GetSupervariantIndex(), rowSrgLayout->GetName());
                    AZ_Error("DiffuseProbeGrid", m_borderUpdateRowDistanceSrg.get(), "Failed to create BorderUpdateRowDistance shader resource group");
                }

                m_borderUpdateRowDistanceSrg->SetImageView(m_renderData->m_borderUpdateRowDistanceSrgProbeTextureNameIndex, m_distanceImage[m_currentImageIndex]->BuildImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());
                m_borderUpdateRowDistanceSrg->SetConstant(m_renderData->m_borderUpdateRowDistanceSrgNumTexelsNameIndex, DefaultNumDistanceTexels);
            }

            // border update column distance
            {
                if (!m_borderUpdateColumnDistanceSrg)
                {
                    m_borderUpdateColumnDistanceSrg = RPI::ShaderResourceGroup::Create(columnShader->GetAsset(), columnShader->GetSupervariantIndex(), columnSrgLayout->GetName());
                    AZ_Error("DiffuseProbeGrid", m_borderUpdateColumnDistanceSrg.get(), "Failed to create BorderUpdateColumnRowDistance shader resource group");
                }

                m_borderUpdateColumnDistanceSrg->SetImageView(m_renderData->m_borderUpdateColumnDistanceSrgProbeTextureNameIndex, m_distanceImage[m_currentImageIndex]->BuildImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());
                m_borderUpdateColumnDistanceSrg->SetConstant(m_renderData->m_borderUpdateColumnDistanceSrgNumTexelsNameIndex, DefaultNumDistanceTexels);
            }
        }

        void DiffuseProbeGrid::UpdateRelocationSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
        {
            if (!m_relocationSrg)
            {
                m_relocationSrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
                AZ_Error("DiffuseProbeGrid", m_relocationSrg.get(), "Failed to create Relocation shader resource group");
            }

            m_relocationSrg->SetBufferView(m_renderData->m_relocationSrgGridDataNameIndex, m_gridDataBuffer->BuildBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());
            m_relocationSrg->SetImageView(m_renderData->m_relocationSrgProbeRayTraceNameIndex, m_rayTraceImage[m_currentImageIndex]->BuildImageView(m_renderData->m_probeRayTraceImageViewDescriptor).get());
            m_relocationSrg->SetImageView(m_renderData->m_relocationSrgProbeDataNameIndex, m_probeDataImage[m_currentImageIndex]->BuildImageView(m_renderData->m_probeDataImageViewDescriptor).get());
            m_relocationSrg->SetConstant(m_renderData->m_relocationSrgFrameUpdateCountNameIndex, m_frameUpdateCount);
            m_relocationSrg->SetConstant(m_renderData->m_relocationSrgFrameUpdateIndexNameIndex, m_frameUpdateIndex);
        }

        void DiffuseProbeGrid::UpdateClassificationSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
        {
            if (!m_classificationSrg)
            {
                m_classificationSrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
                AZ_Error("DiffuseProbeGrid", m_classificationSrg.get(), "Failed to create Classification shader resource group");
            }

            m_classificationSrg->SetBufferView(m_renderData->m_classificationSrgGridDataNameIndex, m_gridDataBuffer->BuildBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());
            m_classificationSrg->SetImageView(m_renderData->m_classificationSrgProbeRayTraceNameIndex, m_rayTraceImage[m_currentImageIndex]->BuildImageView(m_renderData->m_probeRayTraceImageViewDescriptor).get());
            m_classificationSrg->SetImageView(m_renderData->m_classificationSrgProbeDataNameIndex, m_probeDataImage[m_currentImageIndex]->BuildImageView(m_renderData->m_probeDataImageViewDescriptor).get());
            m_classificationSrg->SetConstant(m_renderData->m_classificationSrgFrameUpdateCountNameIndex, m_frameUpdateCount);
            m_classificationSrg->SetConstant(m_renderData->m_classificationSrgFrameUpdateIndexNameIndex, m_frameUpdateIndex);
        }

        void DiffuseProbeGrid::UpdateRenderObjectSrg()
        {
            if (!m_updateRenderObjectSrg)
            {
                return;
            }

            if (!m_renderObjectSrg)
            {
                m_renderObjectSrg = RPI::ShaderResourceGroup::Create(m_renderData->m_shader->GetAsset(), m_renderData->m_shader->GetSupervariantIndex(), m_renderData->m_srgLayout->GetName());
                AZ_Error("DiffuseProbeGrid", m_renderObjectSrg.get(), "Failed to create render shader resource group");
            }

            m_renderObjectSrg->SetBufferView(m_renderData->m_renderSrgGridDataNameIndex, m_gridDataBuffer->BuildBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());

            AZ::Matrix3x4 modelToWorld = AZ::Matrix3x4::CreateFromTransform(m_transform) * AZ::Matrix3x4::CreateScale(m_renderExtents);
            m_renderObjectSrg->SetConstant(m_renderData->m_renderSrgModelToWorldNameIndex, modelToWorld);

            AZ::Matrix3x4 modelToWorldInverse = modelToWorld.GetInverseFull();
            m_renderObjectSrg->SetConstant(m_renderData->m_renderSrgModelToWorldInverseNameIndex, modelToWorldInverse);

            m_renderObjectSrg->SetConstant(m_renderData->m_renderSrgObbHalfLengthsNameIndex, m_obbWs.GetHalfLengths());
            m_renderObjectSrg->SetConstant(m_renderData->m_renderSrgEnableDiffuseGiNameIndex, m_enabled);
            m_renderObjectSrg->SetConstant(m_renderData->m_renderSrgAmbientMultiplierNameIndex, m_ambientMultiplier);
            m_renderObjectSrg->SetConstant(m_renderData->m_renderSrgEdgeBlendIblNameIndex, m_edgeBlendIbl);
            m_renderObjectSrg->SetImageView(m_renderData->m_renderSrgProbeIrradianceNameIndex, GetIrradianceImage()->BuildImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());
            m_renderObjectSrg->SetImageView(m_renderData->m_renderSrgProbeDistanceNameIndex, GetDistanceImage()->BuildImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());
            m_renderObjectSrg->SetImageView(m_renderData->m_renderSrgProbeDataNameIndex, GetProbeDataImage()->BuildImageView(m_renderData->m_probeDataImageViewDescriptor).get());

            m_updateRenderObjectSrg = false;

            // update culling now since the position and/or extents may have changed
            UpdateCulling();
        }

        void DiffuseProbeGrid::UpdateVisualizationPrepareSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
        {
            if (!m_visualizationPrepareSrg)
            {
                m_visualizationPrepareSrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
                AZ_Error("DiffuseProbeGrid", m_visualizationPrepareSrg.get(), "Failed to create VisualizationPrepare shader resource group");
            }

            uint32_t tlasInstancesBufferByteCount = aznumeric_cast<uint32_t>(m_visualizationTlas->GetTlasInstancesBuffer()->GetDescriptor().m_byteCount);
            RHI::BufferViewDescriptor bufferViewDescriptor = RHI::BufferViewDescriptor::CreateStructured(0, tlasInstancesBufferByteCount / RayTracingTlasInstanceElementSize, RayTracingTlasInstanceElementSize);
            m_visualizationPrepareSrg->SetBufferView(m_renderData->m_visualizationPrepareSrgTlasInstancesNameIndex, m_visualizationTlas->GetTlasInstancesBuffer()->BuildBufferView(bufferViewDescriptor).get());

            m_visualizationPrepareSrg->SetBufferView(m_renderData->m_visualizationPrepareSrgGridDataNameIndex, m_gridDataBuffer->BuildBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());
            m_visualizationPrepareSrg->SetImageView(m_renderData->m_visualizationPrepareSrgProbeDataNameIndex, GetProbeDataImage()->BuildImageView(m_renderData->m_probeDataImageViewDescriptor).get());
            m_visualizationPrepareSrg->SetConstant(m_renderData->m_visualizationPrepareSrgProbeSphereRadiusNameIndex, m_visualizationSphereRadius);
        }

        void DiffuseProbeGrid::UpdateVisualizationRayTraceSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout, const RHI::ImageView* outputImageView)
        {
            if (!m_visualizationRayTraceSrg)
            {
                m_visualizationRayTraceSrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
                AZ_Error("DiffuseProbeGrid", m_visualizationRayTraceSrg.get(), "Failed to create VisualizationRayTrace shader resource group");
            }

            uint32_t tlasBufferByteCount = aznumeric_cast<uint32_t>(m_visualizationTlas->GetTlasBuffer()->GetDescriptor().m_byteCount);
            RHI::BufferViewDescriptor bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRayTracingTLAS(tlasBufferByteCount);
            m_visualizationRayTraceSrg->SetBufferView(m_renderData->m_visualizationRayTraceSrgTlasNameIndex, m_visualizationTlas->GetTlasBuffer()->BuildBufferView(bufferViewDescriptor).get());

            m_visualizationRayTraceSrg->SetBufferView(m_renderData->m_visualizationRayTraceSrgGridDataNameIndex, m_gridDataBuffer->BuildBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());
            m_visualizationRayTraceSrg->SetImageView(m_renderData->m_visualizationRayTraceSrgProbeIrradianceNameIndex, GetIrradianceImage()->BuildImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());
            m_visualizationRayTraceSrg->SetImageView(m_renderData->m_visualizationRayTraceSrgProbeDistanceNameIndex, GetDistanceImage()->BuildImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());
            m_visualizationRayTraceSrg->SetImageView(m_renderData->m_visualizationRayTraceSrgProbeDataNameIndex, GetProbeDataImage()->BuildImageView(m_renderData->m_probeDataImageViewDescriptor).get());
            m_visualizationRayTraceSrg->SetConstant(m_renderData->m_visualizationRayTraceSrgShowInactiveProbesNameIndex, m_visualizationShowInactiveProbes);
            m_visualizationRayTraceSrg->SetImageView(m_renderData->m_visualizationRayTraceSrgOutputNameIndex, outputImageView);
        }

        void DiffuseProbeGrid::UpdateQuerySrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
        {
            if (!m_querySrg)
            {
                m_querySrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
                AZ_Error("DiffuseProbeGrid", m_querySrg.get(), "Failed to create Query shader resource group");
            }

            m_querySrg->SetBufferView(m_renderData->m_querySrgGridDataNameIndex, m_gridDataBuffer->BuildBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());
            m_querySrg->SetImageView(m_renderData->m_querySrgProbeIrradianceNameIndex, GetIrradianceImage()->BuildImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());
            m_querySrg->SetImageView(m_renderData->m_querySrgProbeDistanceNameIndex, GetDistanceImage()->BuildImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());
            m_querySrg->SetImageView(m_renderData->m_querySrgProbeDataNameIndex, GetProbeDataImage()->BuildImageView(m_renderData->m_probeDataImageViewDescriptor).get());
            m_querySrg->SetConstant(m_renderData->m_querySrgAmbientMultiplierNameIndex, m_ambientMultiplier);
        }

        void DiffuseProbeGrid::UpdateCulling()
        {
            if (!m_drawPacket)
            {
                return;
            }

            // set draw list mask
            m_cullable.m_cullData.m_drawListMask.reset();
            m_cullable.m_cullData.m_drawListMask = m_drawPacket->GetDrawListMask();

            // setup the Lod entry, only one entry is needed for the draw packet
            m_cullable.m_lodData.m_lods.clear();
            m_cullable.m_lodData.m_lods.resize(1);
            RPI::Cullable::LodData::Lod& lod = m_cullable.m_lodData.m_lods.back();

            // add the draw packet
            lod.m_drawPackets.push_back(m_drawPacket.get());

            // set screen coverage
            // probe volume should cover at least a screen pixel at 1080p to be drawn
            static const float MinimumScreenCoverage = 1.0f / 1080.0f;
            lod.m_screenCoverageMin = MinimumScreenCoverage;
            lod.m_screenCoverageMax = 1.0f;

            // update cullable bounds
            Aabb aabbWs = Aabb::CreateFromObb(m_obbWs);
            Vector3 center;
            float radius;
            aabbWs.GetAsSphere(center, radius);

            m_cullable.m_cullData.m_boundingSphere = Sphere(center, radius);
            m_cullable.m_cullData.m_boundingObb = m_obbWs;
            m_cullable.m_cullData.m_visibilityEntry.m_boundingVolume = aabbWs;
            m_cullable.m_cullData.m_visibilityEntry.m_userData = &m_cullable;
            m_cullable.m_cullData.m_visibilityEntry.m_typeFlags = AzFramework::VisibilityEntry::TYPE_RPI_Cullable;

            // register with culling system
            m_scene->GetCullingScene()->RegisterOrUpdateCullable(m_cullable);
        }
    }   // namespace Render
}   // namespace AZ
