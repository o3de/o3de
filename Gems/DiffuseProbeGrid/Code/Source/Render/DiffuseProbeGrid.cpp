/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Render/DiffuseProbeGrid.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/Factory.h>
#include <AzCore/Math/MathUtils.h>
#include <RayTracing/RayTracingFeatureProcessor.h>

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
            m_visualizationTlas = AZ::RHI::RayTracingTlas::CreateRHIRayTracingTlas();

            // create the grid data buffer
            m_gridDataBuffer = RHI::Factory::Get().CreateBuffer();

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

                        RHI::DrawPacketBuilder drawPacketBuilder;

                        RHI::DrawIndexed drawIndexed;
                        drawIndexed.m_indexCount = aznumeric_cast<uint32_t>(m_renderData->m_boxIndexCount);
                        drawIndexed.m_indexOffset = 0;
                        drawIndexed.m_vertexOffset = 0;

                        drawPacketBuilder.Begin(nullptr);
                        drawPacketBuilder.SetDrawArguments(drawIndexed);
                        drawPacketBuilder.SetIndexBufferView(m_renderData->m_boxIndexBufferView);
                        drawPacketBuilder.AddShaderResourceGroup(m_renderObjectSrg->GetRHIShaderResourceGroup());

                        RHI::DrawPacketBuilder::DrawRequest drawRequest;
                        drawRequest.m_listTag = m_renderData->m_drawListTag;
                        drawRequest.m_pipelineState = m_renderData->m_pipelineState->GetRHIPipelineState();
                        drawRequest.m_streamBufferViews = m_renderData->m_boxPositionBufferView;
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
                RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
                m_mode = (device->GetFeatures().m_rayTracing) ? DiffuseProbeGridMode::RealTime : DiffuseProbeGridMode::Baked;
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

            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();

            uint32_t probeCountX;
            uint32_t probeCountY;
            GetTexture2DProbeCount(probeCountX, probeCountY);

            if (m_mode == DiffuseProbeGridMode::RealTime)
            {
                // advance to the next image in the frame image array
                m_currentImageIndex = (m_currentImageIndex + 1) % ImageFrameCount;

                // probe raytrace
                {
                    uint32_t width = GetNumRaysPerProbe().m_rayCount;
                    uint32_t height = GetTotalProbeCount();

                    m_rayTraceImage[m_currentImageIndex] = RHI::Factory::Get().CreateImage();

                    RHI::ImageInitRequest request;
                    request.m_image = m_rayTraceImage[m_currentImageIndex].get();
                    request.m_descriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyRead, width, height, DiffuseProbeGridRenderData::RayTraceImageFormat);
                    [[maybe_unused]] RHI::ResultCode result = m_renderData->m_imagePool->InitImage(request);
                    AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize m_probeRayTraceImage image");
                }

                // probe irradiance
                {
                    uint32_t width = probeCountX * (DefaultNumIrradianceTexels + 2);
                    uint32_t height = probeCountY * (DefaultNumIrradianceTexels + 2);

                    m_irradianceImage[m_currentImageIndex] = RHI::Factory::Get().CreateImage();

                    RHI::ImageInitRequest request;
                    request.m_image = m_irradianceImage[m_currentImageIndex].get();
                    request.m_descriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyRead, width, height, DiffuseProbeGridRenderData::IrradianceImageFormat);
                    RHI::ClearValue clearValue = RHI::ClearValue::CreateVector4Float(0.0f, 0.0f, 0.0f, 0.0f);
                    request.m_optimizedClearValue = &clearValue;
                    [[maybe_unused]] RHI::ResultCode result = m_renderData->m_imagePool->InitImage(request);
                    AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize m_probeIrradianceImage image");
                }

                // probe distance
                {
                    uint32_t width = probeCountX * (DefaultNumDistanceTexels + 2);
                    uint32_t height = probeCountY * (DefaultNumDistanceTexels + 2);

                    m_distanceImage[m_currentImageIndex] = RHI::Factory::Get().CreateImage();

                    RHI::ImageInitRequest request;
                    request.m_image = m_distanceImage[m_currentImageIndex].get();
                    request.m_descriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyRead, width, height, DiffuseProbeGridRenderData::DistanceImageFormat);
                    [[maybe_unused]] RHI::ResultCode result = m_renderData->m_imagePool->InitImage(request);
                    AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize m_probeDistanceImage image");
                }

                // probe data
                {
                    uint32_t width = probeCountX;
                    uint32_t height = probeCountY;

                    m_probeDataImage[m_currentImageIndex] = RHI::Factory::Get().CreateImage();

                    RHI::ImageInitRequest request;
                    request.m_image = m_probeDataImage[m_currentImageIndex].get();
                    request.m_descriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyRead, width, height, DiffuseProbeGridRenderData::ProbeDataImageFormat);
                    [[maybe_unused]] RHI::ResultCode result = m_renderData->m_imagePool->InitImage(request);
                    AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize m_probeDataImage image");
                }

                // probes need to be relocated since the textures changed
                m_remainingRelocationIterations = DefaultNumRelocationIterations;
            }

            m_updateTextures = false;

            // textures have changed so we need to update the render Srg to bind the new ones
            m_updateRenderObjectSrg = true;

            // we need to clear the Irradiance, Distance, and ProbeData textures
            m_textureClearRequired = true;
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

            RHI::ShaderInputBufferIndex bufferIndex;
            RHI::ShaderInputConstantIndex constantIndex;

            bufferIndex = layout->FindShaderInputBufferIndex(AZ::Name("m_gridData"));
            m_prepareSrg->SetBufferView(bufferIndex, m_gridDataBuffer->GetBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_gridDataInitialized"));
            m_prepareSrg->SetConstant(constantIndex, m_gridDataInitialized);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.origin"));
            m_prepareSrg->SetConstant(constantIndex, m_transform.GetTranslation());

            // pass identity for the rotation when scrolling is enabled
            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.rotation"));
            m_prepareSrg->SetConstant(constantIndex, m_transform.GetRotation());

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeRayRotation"));
            m_prepareSrg->SetConstant(constantIndex, m_probeRayRotation);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.movementType"));
            m_prepareSrg->SetConstant(constantIndex, (uint32_t)m_scrolling);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeSpacing"));
            m_prepareSrg->SetConstant(constantIndex, m_probeSpacing);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeCounts"));
            uint32_t probeGridCounts[3];
            probeGridCounts[0] = m_probeCountX;
            probeGridCounts[1] = m_probeCountY;
            probeGridCounts[2] = m_probeCountZ;
            m_prepareSrg->SetConstantRaw(constantIndex, &probeGridCounts[0], sizeof(probeGridCounts));

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeNumRays"));
            m_prepareSrg->SetConstant(constantIndex, GetNumRaysPerProbe().m_rayCount);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeNumIrradianceTexels"));
            m_prepareSrg->SetConstant(constantIndex, DefaultNumIrradianceTexels);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeNumDistanceTexels"));
            m_prepareSrg->SetConstant(constantIndex, DefaultNumDistanceTexels);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeHysteresis"));
            m_prepareSrg->SetConstant(constantIndex, m_probeHysteresis);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeMaxRayDistance"));
            m_prepareSrg->SetConstant(constantIndex, m_probeMaxRayDistance);

            // scale the normal bias based on the grid density to reduce artifacts on thin geometry, less density results in more bias
            float scaledNormalBias = m_normalBias + 0.15f * (m_probeSpacing.GetMaxElement() / 2.0f);
            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeNormalBias"));
            m_prepareSrg->SetConstant(constantIndex, scaledNormalBias);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeViewBias"));
            m_prepareSrg->SetConstant(constantIndex, m_viewBias);

            // scale the probe distance exponent based on the grid density to reduce artifacts on thin geometry
            static const float MinProbeDistanceExponent = 50.0f;
            float scaledProbeDistanceExponent = AZStd::max(m_probeDistanceExponent * (m_probeSpacing.GetMaxElement() / 1.5f), MinProbeDistanceExponent);
            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeDistanceExponent"));
            m_prepareSrg->SetConstant(constantIndex, scaledProbeDistanceExponent);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeIrradianceThreshold"));
            m_prepareSrg->SetConstant(constantIndex, m_probeIrradianceThreshold);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeBrightnessThreshold"));
            m_prepareSrg->SetConstant(constantIndex, m_probeBrightnessThreshold);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeIrradianceEncodingGamma"));
            m_prepareSrg->SetConstant(constantIndex, m_probeIrradianceEncodingGamma);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeRandomRayBackfaceThreshold"));
            m_prepareSrg->SetConstant(constantIndex, m_probeRandomRayBackfaceThreshold);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeFixedRayBackfaceThreshold"));
            m_prepareSrg->SetConstant(constantIndex, m_probeFixedRayBackfaceThreshold);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeMinFrontfaceDistance"));
            m_prepareSrg->SetConstant(constantIndex, m_probeMinFrontfaceDistance);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeScrollOffsets"));
            m_prepareSrg->SetConstant(constantIndex, Vector3::CreateZero());

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeRayDataFormat"));
            m_prepareSrg->SetConstant(constantIndex, 1);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeIrradianceFormat"));
            m_prepareSrg->SetConstant(constantIndex, 1);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeRelocationEnabled"));
            m_prepareSrg->SetConstant(constantIndex, true);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeClassificationEnabled"));
            m_prepareSrg->SetConstant(constantIndex, true);

            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeScrollClear[0]"));
            m_prepareSrg->SetConstant(constantIndex, false);
            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeScrollClear[1]"));
            m_prepareSrg->SetConstant(constantIndex, false);
            constantIndex = layout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeScrollClear[2]"));
            m_prepareSrg->SetConstant(constantIndex, false);

            m_gridDataInitialized = true;
        }

        void DiffuseProbeGrid::UpdateRayTraceSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
        {
            if (!m_rayTraceSrg)
            {
                m_rayTraceSrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
                AZ_Error("DiffuseProbeGrid", m_rayTraceSrg.get(), "Failed to create RayTrace shader resource group");
            }

            const RHI::ShaderResourceGroupLayout* srgLayout = m_rayTraceSrg->GetLayout();
            RHI::ShaderInputConstantIndex constantIndex;
            RHI::ShaderInputImageIndex imageIndex;
            RHI::ShaderInputBufferIndex bufferIndex;

            // grid data
            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_gridData"));
            m_rayTraceSrg->SetBufferView(bufferIndex, m_gridDataBuffer->GetBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());

            // probe raytrace
            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeRayTrace"));
            m_rayTraceSrg->SetImageView(imageIndex, m_rayTraceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeRayTraceImageViewDescriptor).get());

            // probe irradiance
            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeIrradiance"));
            m_rayTraceSrg->SetImageView(imageIndex, m_irradianceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());

            // probe distance
            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeDistance"));
            m_rayTraceSrg->SetImageView(imageIndex, m_distanceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());

            // probe data
            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeData"));
            m_rayTraceSrg->SetImageView(imageIndex, m_probeDataImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeDataImageViewDescriptor).get());

            // grid settings
            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_ambientMultiplier"));
            m_rayTraceSrg->SetConstant(constantIndex, m_ambientMultiplier);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_giShadows"));
            m_rayTraceSrg->SetConstant(constantIndex, m_giShadows);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_useDiffuseIbl"));
            m_rayTraceSrg->SetConstant(constantIndex, m_useDiffuseIbl);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_frameUpdateCount"));
            m_rayTraceSrg->SetConstant(constantIndex, m_frameUpdateCount);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_frameUpdateIndex"));
            m_rayTraceSrg->SetConstant(constantIndex, m_frameUpdateIndex);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_transparencyMode"));
            m_rayTraceSrg->SetConstant(constantIndex, aznumeric_cast<uint32_t>(m_transparencyMode));
        }

        void DiffuseProbeGrid::UpdateBlendIrradianceSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
        {
            if (!m_blendIrradianceSrg)
            {
                m_blendIrradianceSrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
                AZ_Error("DiffuseProbeGrid", m_blendIrradianceSrg.get(), "Failed to create BlendIrradiance shader resource group");
            }

            const RHI::ShaderResourceGroupLayout* srgLayout = m_blendIrradianceSrg->GetLayout();
            RHI::ShaderInputConstantIndex constantIndex;
            RHI::ShaderInputImageIndex imageIndex;
            RHI::ShaderInputBufferIndex bufferIndex;

            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_gridData"));
            m_blendIrradianceSrg->SetBufferView(bufferIndex, m_gridDataBuffer->GetBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeRayTrace"));
            m_blendIrradianceSrg->SetImageView(imageIndex, m_rayTraceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeRayTraceImageViewDescriptor).get());
                    
            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeIrradiance"));
            m_blendIrradianceSrg->SetImageView(imageIndex, m_irradianceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeData"));
            m_blendIrradianceSrg->SetImageView(imageIndex, m_probeDataImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeDataImageViewDescriptor).get());

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_frameUpdateCount"));
            m_blendIrradianceSrg->SetConstant(constantIndex, m_frameUpdateCount);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_frameUpdateIndex"));
            m_blendIrradianceSrg->SetConstant(constantIndex, m_frameUpdateIndex);
        }

        void DiffuseProbeGrid::UpdateBlendDistanceSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
        {
            if (!m_blendDistanceSrg)
            {
                m_blendDistanceSrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
                AZ_Error("DiffuseProbeGrid", m_blendDistanceSrg.get(), "Failed to create BlendDistance shader resource group");
            }

            const RHI::ShaderResourceGroupLayout* srgLayout = m_blendDistanceSrg->GetLayout();
            RHI::ShaderInputConstantIndex constantIndex;
            RHI::ShaderInputImageIndex imageIndex;
            RHI::ShaderInputBufferIndex bufferIndex;

            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_gridData"));
            m_blendDistanceSrg->SetBufferView(bufferIndex, m_gridDataBuffer->GetBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeRayTrace"));
            m_blendDistanceSrg->SetImageView(imageIndex, m_rayTraceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeRayTraceImageViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeDistance"));
            m_blendDistanceSrg->SetImageView(imageIndex, m_distanceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeData"));
            m_blendDistanceSrg->SetImageView(imageIndex, m_probeDataImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeDataImageViewDescriptor).get());

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_frameUpdateCount"));
            m_blendDistanceSrg->SetConstant(constantIndex, m_frameUpdateCount);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_frameUpdateIndex"));
            m_blendDistanceSrg->SetConstant(constantIndex, m_frameUpdateIndex);
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

                const RHI::ShaderResourceGroupLayout* srgLayout = m_borderUpdateRowIrradianceSrg->GetLayout();
                RHI::ShaderInputConstantIndex constantIndex;
                RHI::ShaderInputImageIndex imageIndex;

                imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeTexture"));
                m_borderUpdateRowIrradianceSrg->SetImageView(imageIndex, m_irradianceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());

                constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_numTexels"));
                m_borderUpdateRowIrradianceSrg->SetConstant(constantIndex, DefaultNumIrradianceTexels);
            }

            // border update column irradiance
            {
                if (!m_borderUpdateColumnIrradianceSrg)
                {
                    m_borderUpdateColumnIrradianceSrg = RPI::ShaderResourceGroup::Create(columnShader->GetAsset(), columnShader->GetSupervariantIndex(), columnSrgLayout->GetName());
                    AZ_Error("DiffuseProbeGrid", m_borderUpdateColumnIrradianceSrg.get(), "Failed to create BorderUpdateColumnRowIrradiance shader resource group");
                }

                const RHI::ShaderResourceGroupLayout* srgLayout = m_borderUpdateColumnIrradianceSrg->GetLayout();
                RHI::ShaderInputConstantIndex constantIndex;
                RHI::ShaderInputImageIndex imageIndex;

                imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeTexture"));
                m_borderUpdateColumnIrradianceSrg->SetImageView(imageIndex, m_irradianceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());

                constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_numTexels"));
                m_borderUpdateColumnIrradianceSrg->SetConstant(constantIndex, DefaultNumIrradianceTexels);
            }

            // border update row distance
            {
                if (!m_borderUpdateRowDistanceSrg)
                {
                    m_borderUpdateRowDistanceSrg = RPI::ShaderResourceGroup::Create(rowShader->GetAsset(), rowShader->GetSupervariantIndex(), rowSrgLayout->GetName());
                    AZ_Error("DiffuseProbeGrid", m_borderUpdateRowDistanceSrg.get(), "Failed to create BorderUpdateRowDistance shader resource group");
                }

                const RHI::ShaderResourceGroupLayout* srgLayout = m_borderUpdateRowDistanceSrg->GetLayout();
                RHI::ShaderInputConstantIndex constantIndex;
                RHI::ShaderInputImageIndex imageIndex;

                imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeTexture"));
                m_borderUpdateRowDistanceSrg->SetImageView(imageIndex, m_distanceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());

                constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_numTexels"));
                m_borderUpdateRowDistanceSrg->SetConstant(constantIndex, DefaultNumDistanceTexels);
            }

            // border update column distance
            {
                if (!m_borderUpdateColumnDistanceSrg)
                {
                    m_borderUpdateColumnDistanceSrg = RPI::ShaderResourceGroup::Create(columnShader->GetAsset(), columnShader->GetSupervariantIndex(), columnSrgLayout->GetName());
                    AZ_Error("DiffuseProbeGrid", m_borderUpdateColumnDistanceSrg.get(), "Failed to create BorderUpdateColumnRowDistance shader resource group");
                }

                const RHI::ShaderResourceGroupLayout* srgLayout = m_borderUpdateColumnDistanceSrg->GetLayout();
                RHI::ShaderInputConstantIndex constantIndex;
                RHI::ShaderInputImageIndex imageIndex;

                imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeTexture"));
                m_borderUpdateColumnDistanceSrg->SetImageView(imageIndex, m_distanceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());

                constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_numTexels"));
                m_borderUpdateColumnDistanceSrg->SetConstant(constantIndex, DefaultNumDistanceTexels);
            }
        }

        void DiffuseProbeGrid::UpdateRelocationSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
        {
            if (!m_relocationSrg)
            {
                m_relocationSrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
                AZ_Error("DiffuseProbeGrid", m_relocationSrg.get(), "Failed to create Relocation shader resource group");
            }

            const RHI::ShaderResourceGroupLayout* srgLayout = m_relocationSrg->GetLayout();
            RHI::ShaderInputConstantIndex constantIndex;
            RHI::ShaderInputImageIndex imageIndex;
            RHI::ShaderInputBufferIndex bufferIndex;

            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_gridData"));
            m_relocationSrg->SetBufferView(bufferIndex, m_gridDataBuffer->GetBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeRayTrace"));
            m_relocationSrg->SetImageView(imageIndex, m_rayTraceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeRayTraceImageViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeData"));
            m_relocationSrg->SetImageView(imageIndex, m_probeDataImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeDataImageViewDescriptor).get());

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_frameUpdateCount"));
            m_relocationSrg->SetConstant(constantIndex, m_frameUpdateCount);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_frameUpdateIndex"));
            m_relocationSrg->SetConstant(constantIndex, m_frameUpdateIndex);
        }

        void DiffuseProbeGrid::UpdateClassificationSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
        {
            if (!m_classificationSrg)
            {
                m_classificationSrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
                AZ_Error("DiffuseProbeGrid", m_classificationSrg.get(), "Failed to create Classification shader resource group");
            }

            const RHI::ShaderResourceGroupLayout* srgLayout = m_classificationSrg->GetLayout();
            RHI::ShaderInputConstantIndex constantIndex;
            RHI::ShaderInputImageIndex imageIndex;
            RHI::ShaderInputBufferIndex bufferIndex;

            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_gridData"));
            m_classificationSrg->SetBufferView(bufferIndex, m_gridDataBuffer->GetBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeRayTrace"));
            m_classificationSrg->SetImageView(imageIndex, m_rayTraceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeRayTraceImageViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeData"));
            m_classificationSrg->SetImageView(imageIndex, m_probeDataImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeDataImageViewDescriptor).get());

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_frameUpdateCount"));
            m_classificationSrg->SetConstant(constantIndex, m_frameUpdateCount);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_frameUpdateIndex"));
            m_classificationSrg->SetConstant(constantIndex, m_frameUpdateIndex);
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

            const RHI::ShaderResourceGroupLayout* srgLayout = m_renderObjectSrg->GetLayout();
            RHI::ShaderInputConstantIndex constantIndex;
            RHI::ShaderInputImageIndex imageIndex;
            RHI::ShaderInputBufferIndex bufferIndex;

            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_gridData"));
            m_renderObjectSrg->SetBufferView(bufferIndex, m_gridDataBuffer->GetBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_modelToWorld"));
            AZ::Matrix3x4 modelToWorld = AZ::Matrix3x4::CreateFromTransform(m_transform) * AZ::Matrix3x4::CreateScale(m_renderExtents);
            m_renderObjectSrg->SetConstant(constantIndex, modelToWorld);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_modelToWorldInverse"));
            AZ::Matrix3x4 modelToWorldInverse = modelToWorld.GetInverseFull();
            m_renderObjectSrg->SetConstant(constantIndex, modelToWorldInverse);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_obbHalfLengths"));
            m_renderObjectSrg->SetConstant(constantIndex, m_obbWs.GetHalfLengths());

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_enableDiffuseGI"));
            m_renderObjectSrg->SetConstant(constantIndex, m_enabled);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_ambientMultiplier"));
            m_renderObjectSrg->SetConstant(constantIndex, m_ambientMultiplier);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_edgeBlendIbl"));
            m_renderObjectSrg->SetConstant(constantIndex, m_edgeBlendIbl);

            imageIndex = srgLayout->FindShaderInputImageIndex(Name("m_probeIrradiance"));
            m_renderObjectSrg->SetImageView(imageIndex, GetIrradianceImage()->GetImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(Name("m_probeDistance"));
            m_renderObjectSrg->SetImageView(imageIndex, GetDistanceImage()->GetImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(Name("m_probeData"));
            m_renderObjectSrg->SetImageView(imageIndex, GetProbeDataImage()->GetImageView(m_renderData->m_probeDataImageViewDescriptor).get());

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

            RHI::ShaderInputConstantIndex constantIndex;
            RHI::ShaderInputImageIndex imageIndex;
            RHI::ShaderInputBufferIndex bufferIndex;

            // TLAS instances
            bufferIndex = layout->FindShaderInputBufferIndex(AZ::Name("m_tlasInstances"));
            uint32_t tlasInstancesBufferByteCount = aznumeric_cast<uint32_t>(m_visualizationTlas->GetTlasInstancesBuffer()->GetDescriptor().m_byteCount);
            RHI::BufferViewDescriptor bufferViewDescriptor = RHI::BufferViewDescriptor::CreateStructured(0, tlasInstancesBufferByteCount / RayTracingTlasInstanceElementSize, RayTracingTlasInstanceElementSize);
            m_visualizationPrepareSrg->SetBufferView(bufferIndex, m_visualizationTlas->GetTlasInstancesBuffer()->GetBufferView(bufferViewDescriptor).get());

            // grid data
            bufferIndex = layout->FindShaderInputBufferIndex(AZ::Name("m_gridData"));
            m_visualizationPrepareSrg->SetBufferView(bufferIndex, m_gridDataBuffer->GetBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());

            // probe data
            imageIndex = layout->FindShaderInputImageIndex(AZ::Name("m_probeData"));
            m_visualizationPrepareSrg->SetImageView(imageIndex, GetProbeDataImage()->GetImageView(m_renderData->m_probeDataImageViewDescriptor).get());

            // probe sphere radius
            constantIndex = layout->FindShaderInputConstantIndex(Name("m_probeSphereRadius"));
            m_visualizationPrepareSrg->SetConstant(constantIndex, m_visualizationSphereRadius);
        }

        void DiffuseProbeGrid::UpdateVisualizationRayTraceSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout, const RHI::ImageView* outputImageView)
        {
            if (!m_visualizationRayTraceSrg)
            {
                m_visualizationRayTraceSrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
                AZ_Error("DiffuseProbeGrid", m_visualizationRayTraceSrg.get(), "Failed to create VisualizationRayTrace shader resource group");
            }

            RHI::ShaderInputConstantIndex constantIndex;
            RHI::ShaderInputImageIndex imageIndex;
            RHI::ShaderInputBufferIndex bufferIndex;

            // TLAS
            uint32_t tlasBufferByteCount = aznumeric_cast<uint32_t>(m_visualizationTlas->GetTlasBuffer()->GetDescriptor().m_byteCount);
            RHI::BufferViewDescriptor bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRayTracingTLAS(tlasBufferByteCount);

            bufferIndex = layout->FindShaderInputBufferIndex(AZ::Name("m_tlas"));
            m_visualizationRayTraceSrg->SetBufferView(bufferIndex, m_visualizationTlas->GetTlasBuffer()->GetBufferView(bufferViewDescriptor).get());

            // grid data
            bufferIndex = layout->FindShaderInputBufferIndex(AZ::Name("m_gridData"));
            m_visualizationRayTraceSrg->SetBufferView(bufferIndex, m_gridDataBuffer->GetBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());

            // probe irradiance
            imageIndex = layout->FindShaderInputImageIndex(AZ::Name("m_probeIrradiance"));
            m_visualizationRayTraceSrg->SetImageView(imageIndex, GetIrradianceImage()->GetImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());

            // probe distance
            imageIndex = layout->FindShaderInputImageIndex(AZ::Name("m_probeDistance"));
            m_visualizationRayTraceSrg->SetImageView(imageIndex, GetDistanceImage()->GetImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());

            // probe data
            imageIndex = layout->FindShaderInputImageIndex(AZ::Name("m_probeData"));
            m_visualizationRayTraceSrg->SetImageView(imageIndex, GetProbeDataImage()->GetImageView(m_renderData->m_probeDataImageViewDescriptor).get());

            // show inactive probes
            constantIndex = layout->FindShaderInputConstantIndex(Name("m_showInactiveProbes"));
            m_visualizationRayTraceSrg->SetConstant(constantIndex, m_visualizationShowInactiveProbes);

            // output
            imageIndex = layout->FindShaderInputImageIndex(AZ::Name("m_output"));
            m_visualizationRayTraceSrg->SetImageView(imageIndex, outputImageView);
        }

        void DiffuseProbeGrid::UpdateQuerySrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
        {
            if (!m_querySrg)
            {
                m_querySrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
                AZ_Error("DiffuseProbeGrid", m_querySrg.get(), "Failed to create Query shader resource group");
            }

            RHI::ShaderInputConstantIndex constantIndex;
            RHI::ShaderInputBufferIndex bufferIndex;
            RHI::ShaderInputImageIndex imageIndex;

            // grid data
            bufferIndex = layout->FindShaderInputBufferIndex(AZ::Name("m_gridData"));
            m_querySrg->SetBufferView(bufferIndex, m_gridDataBuffer->GetBufferView(m_renderData->m_gridDataBufferViewDescriptor).get());

            // probe irradiance
            imageIndex = layout->FindShaderInputImageIndex(AZ::Name("m_probeIrradiance"));
            m_querySrg->SetImageView(imageIndex, GetIrradianceImage()->GetImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());

            // probe distance
            imageIndex = layout->FindShaderInputImageIndex(AZ::Name("m_probeDistance"));
            m_querySrg->SetImageView(imageIndex, GetDistanceImage()->GetImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());

            // probe data
            imageIndex = layout->FindShaderInputImageIndex(AZ::Name("m_probeData"));
            m_querySrg->SetImageView(imageIndex, GetProbeDataImage()->GetImageView(m_renderData->m_probeDataImageViewDescriptor).get());

            // ambient multiplier
            constantIndex = layout->FindShaderInputConstantIndex(Name("m_ambientMultiplier"));
            m_querySrg->SetConstant(constantIndex, m_ambientMultiplier);
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
