/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <DiffuseGlobalIllumination/DiffuseProbeGrid.h>
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
            m_relocationImageAttachmentId = AZStd::string::format("ProbeRelocationImageAttachmentId_%s", uuidString.c_str());
            m_classificationImageAttachmentId = AZStd::string::format("ProbeClassificationImageAttachmentId_%s", uuidString.c_str());

            // setup culling
            m_cullable.m_cullData.m_scene = m_scene;
            m_cullable.SetDebugName(AZ::Name("DiffuseProbeGrid Volume"));
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

            m_probeRayRotationTransform = AZ::Matrix4x4::CreateIdentity();
        }

        bool DiffuseProbeGrid::ValidateProbeSpacing(const AZ::Vector3& newSpacing)
        {
            return ValidateProbeCount(m_extents, newSpacing);
        }

        void DiffuseProbeGrid::SetProbeSpacing(const AZ::Vector3& probeSpacing)
        {
            m_probeSpacing = probeSpacing;

            // recompute the number of probes since the spacing changed
            UpdateProbeCount();

            // probes need to be relocated since the grid density changed
            m_remainingRelocationIterations = DefaultNumRelocationIterations;

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

        void DiffuseProbeGrid::SetTransform(const AZ::Transform& transform)
        {
            m_transform = transform;
            m_obbWs = Obb::CreateFromPositionRotationAndHalfLengths(m_transform.GetTranslation(), m_transform.GetRotation(), m_extents / 2.0f);

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
            m_obbWs = Obb::CreateFromPositionRotationAndHalfLengths(m_transform.GetTranslation(), m_transform.GetRotation(), m_extents / 2.0f);

            // recompute the number of probes since the extents changed
            UpdateProbeCount();

            // probes need to be relocated since the grid extents changed
            m_remainingRelocationIterations = DefaultNumRelocationIterations;

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

        void DiffuseProbeGrid::SetBakedTextures(const DiffuseProbeGridBakedTextures& bakedTextures)
        {
            AZ_Assert(bakedTextures.m_irradianceImage.get(), "Invalid Irradiance image passed to SetBakedTextures");
            AZ_Assert(bakedTextures.m_distanceImage.get(), "Invalid Distance image passed to SetBakedTextures");
            AZ_Assert(bakedTextures.m_relocationImageData.size() > 0, "Invalid Relocation image data passed to SetBakedTextures");
            AZ_Assert(bakedTextures.m_classificationImageData.size() > 0, "Invalid Classification image data passed to SetBakedTextures");

            m_bakedIrradianceImage = bakedTextures.m_irradianceImage;
            m_bakedDistanceImage = bakedTextures.m_distanceImage;

            m_bakedIrradianceRelativePath = bakedTextures.m_irradianceImageRelativePath;
            m_bakedDistanceRelativePath = bakedTextures.m_distanceImageRelativePath;
            m_bakedRelocationRelativePath = bakedTextures.m_relocationImageRelativePath;
            m_bakedClassificationRelativePath = bakedTextures.m_classificationImageRelativePath;

            m_bakedRelocationImageData.resize(bakedTextures.m_relocationImageData.size());
            memcpy(m_bakedRelocationImageData.data(), bakedTextures.m_relocationImageData.data(), bakedTextures.m_relocationImageData.size());

            m_bakedClassificationImageData.resize(bakedTextures.m_classificationImageData.size());
            memcpy(m_bakedClassificationImageData.data(), bakedTextures.m_classificationImageData.data(), bakedTextures.m_classificationImageData.size());

            // create the relocation and distance RW textures now, these are needed for shader compatibility
            // (image data is copied in UpdateTextures)
            {
                m_bakedRelocationImage = RHI::Factory::Get().CreateImage();
                RHI::ImageInitRequest initRequest;
                initRequest.m_image = m_bakedRelocationImage.get();
                initRequest.m_descriptor = RHI::ImageDescriptor::Create2D(
                    RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyRead,
                    bakedTextures.m_relocationImageDescriptor.m_size.m_width,
                    bakedTextures.m_relocationImageDescriptor.m_size.m_height,
                    bakedTextures.m_relocationImageDescriptor.m_format);

                [[maybe_unused]] RHI::ResultCode result = m_renderData->m_imagePool->InitImage(initRequest);
                AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize Relocation image");
            }
            
            {
                m_bakedClassificationImage = RHI::Factory::Get().CreateImage();
                RHI::ImageInitRequest initRequest;
                initRequest.m_image = m_bakedClassificationImage.get();
                initRequest.m_descriptor = RHI::ImageDescriptor::Create2D(
                    RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyRead,
                    bakedTextures.m_classificationImageDescriptor.m_size.m_width,
                    bakedTextures.m_classificationImageDescriptor.m_size.m_height,
                    bakedTextures.m_classificationImageDescriptor.m_format);

                [[maybe_unused]] RHI::ResultCode result = m_renderData->m_imagePool->InitImage(initRequest);
                AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize Classification image");
            }

            m_updateTextures = true;
        }

        bool DiffuseProbeGrid::HasValidBakedTextures() const
        {
            return m_bakedIrradianceImage.get() &&
                m_bakedDistanceImage.get() &&
                m_bakedRelocationImage.get() &&
                m_bakedClassificationImage.get();
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
                    uint32_t width = m_numRaysPerProbe;
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

                // probe relocation
                {
                    uint32_t width = probeCountX;
                    uint32_t height = probeCountY;

                    m_relocationImage[m_currentImageIndex] = RHI::Factory::Get().CreateImage();

                    RHI::ImageInitRequest request;
                    request.m_image = m_relocationImage[m_currentImageIndex].get();
                    request.m_descriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyRead, width, height, DiffuseProbeGridRenderData::RelocationImageFormat);
                    [[maybe_unused]] RHI::ResultCode result = m_renderData->m_imagePool->InitImage(request);
                    AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize m_probeRelocationImage image");
                }

                // probe classification
                {
                    uint32_t width = probeCountX;
                    uint32_t height = probeCountY;

                    m_classificationImage[m_currentImageIndex] = RHI::Factory::Get().CreateImage();

                    RHI::ImageInitRequest request;
                    request.m_image = m_classificationImage[m_currentImageIndex].get();
                    request.m_descriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyRead, width, height, DiffuseProbeGridRenderData::ClassificationImageFormat);
                    [[maybe_unused]] RHI::ResultCode result = m_renderData->m_imagePool->InitImage(request);
                    AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize m_probeClassificationImage image");
                }
            }
            else if (m_mode == DiffuseProbeGridMode::Baked && HasValidBakedTextures())
            {
                // copy the baked relocation and classification texture data to the RW textures
                // (these need to be RW for shader compatibility)
                RHI::ImageSubresourceRange range{ 0, 0, 0 ,0 };
                RHI::ImageSubresourceLayoutPlaced layout;

                // relocation
                {
                    m_bakedRelocationImage->GetSubresourceLayouts(range, &layout, nullptr);

                    RHI::ImageUpdateRequest updateRequest;
                    updateRequest.m_image = m_bakedRelocationImage.get();
                    updateRequest.m_sourceSubresourceLayout = layout;
                    updateRequest.m_sourceData = m_bakedRelocationImageData.data();
                    updateRequest.m_imageSubresourcePixelOffset = RHI::Origin(0, 0, 0);
                    m_renderData->m_imagePool->UpdateImageContents(updateRequest);
                }

                // classification
                {
                    m_bakedClassificationImage->GetSubresourceLayouts(range, &layout, nullptr);

                    RHI::ImageUpdateRequest updateRequest;
                    updateRequest.m_image = m_bakedClassificationImage.get();
                    updateRequest.m_sourceSubresourceLayout = layout;
                    updateRequest.m_sourceData = m_bakedClassificationImageData.data();
                    updateRequest.m_imageSubresourcePixelOffset = RHI::Origin(0, 0, 0);
                    m_renderData->m_imagePool->UpdateImageContents(updateRequest);
                }
            }

            m_updateTextures = false;

            // textures have changed so we need to update the render Srg to bind the new ones
            m_updateRenderObjectSrg = true;

            // we need to clear the irradiance texture
            m_irradianceClearRequired = true;
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

        void DiffuseProbeGrid::SetGridConstants(Data::Instance<RPI::ShaderResourceGroup>& srg)
        {
            const RHI::ShaderResourceGroupLayout* srgLayout = srg->GetLayout();
            RHI::ShaderInputConstantIndex constantIndex;

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.origin"));
            srg->SetConstant(constantIndex, m_transform.GetTranslation());

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.rotation"));
            srg->SetConstant(constantIndex, m_transform.GetRotation());

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.numRaysPerProbe"));
            srg->SetConstant(constantIndex, m_numRaysPerProbe);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeGridSpacing"));
            srg->SetConstant(constantIndex, m_probeSpacing);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeMaxRayDistance"));
            srg->SetConstant(constantIndex, m_probeMaxRayDistance);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeGridCounts"));
            uint32_t probeGridCounts[3];
            probeGridCounts[0] = m_probeCountX;
            probeGridCounts[1] = m_probeCountY;
            probeGridCounts[2] = m_probeCountZ;
            srg->SetConstantRaw(constantIndex, &probeGridCounts[0], sizeof(probeGridCounts));

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeDistanceExponent"));
            srg->SetConstant(constantIndex, m_probeDistanceExponent);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeHysteresis"));
            srg->SetConstant(constantIndex, m_probeHysteresis);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeChangeThreshold"));
            srg->SetConstant(constantIndex, m_probeChangeThreshold);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeBrightnessThreshold"));
            srg->SetConstant(constantIndex, m_probeBrightnessThreshold);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeIrradianceEncodingGamma"));
            srg->SetConstant(constantIndex, m_probeIrradianceEncodingGamma);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeInverseIrradianceEncodingGamma"));
            srg->SetConstant(constantIndex, m_probeInverseIrradianceEncodingGamma);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeNumIrradianceTexels"));
            srg->SetConstant(constantIndex, DefaultNumIrradianceTexels);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeNumDistanceTexels"));
            srg->SetConstant(constantIndex, DefaultNumDistanceTexels);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.normalBias"));
            srg->SetConstant(constantIndex, m_normalBias);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.viewBias"));
            srg->SetConstant(constantIndex, m_viewBias);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeMinFrontfaceDistance"));
            srg->SetConstant(constantIndex, m_probeMinFrontfaceDistance);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeBackfaceThreshold"));
            srg->SetConstant(constantIndex, m_probeBackfaceThreshold);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeRayRotationTransform"));
            srg->SetConstant(constantIndex, m_probeRayRotationTransform);
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

            // probe raytrace
            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeRayTrace"));
            m_rayTraceSrg->SetImageView(imageIndex, m_rayTraceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeRayTraceImageViewDescriptor).get());

            // probe irradiance
            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeIrradiance"));
            m_rayTraceSrg->SetImageView(imageIndex, m_irradianceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());

            // probe distance
            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeDistance"));
            m_rayTraceSrg->SetImageView(imageIndex, m_distanceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());

            // probe relocation
            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeOffsets"));
            m_rayTraceSrg->SetImageView(imageIndex, m_relocationImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeRelocationImageViewDescriptor).get());

            // probe classification
            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeStates"));
            m_rayTraceSrg->SetImageView(imageIndex, m_classificationImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeClassificationImageViewDescriptor).get());

            // grid settings
            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_ambientMultiplier"));
            m_rayTraceSrg->SetConstant(constantIndex, m_ambientMultiplier);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_giShadows"));
            m_rayTraceSrg->SetConstant(constantIndex, m_giShadows);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_useDiffuseIbl"));
            m_rayTraceSrg->SetConstant(constantIndex, m_useDiffuseIbl);

            SetGridConstants(m_rayTraceSrg);
        }

        void DiffuseProbeGrid::UpdateBlendIrradianceSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
        {
            if (!m_blendIrradianceSrg)
            {
                m_blendIrradianceSrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
                AZ_Error("DiffuseProbeGrid", m_blendIrradianceSrg.get(), "Failed to create BlendIrradiance shader resource group");
            }

            const RHI::ShaderResourceGroupLayout* srgLayout = m_blendIrradianceSrg->GetLayout();
            RHI::ShaderInputImageIndex imageIndex;
        
            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeRayTrace"));
            m_blendIrradianceSrg->SetImageView(imageIndex, m_rayTraceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeRayTraceImageViewDescriptor).get());
                    
            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeIrradiance"));
            m_blendIrradianceSrg->SetImageView(imageIndex, m_irradianceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeStates"));
            m_blendIrradianceSrg->SetImageView(imageIndex, m_classificationImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeClassificationImageViewDescriptor).get());

            SetGridConstants(m_blendIrradianceSrg);
        }

        void DiffuseProbeGrid::UpdateBlendDistanceSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
        {
            if (!m_blendDistanceSrg)
            {
                m_blendDistanceSrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
                AZ_Error("DiffuseProbeGrid", m_blendDistanceSrg.get(), "Failed to create BlendDistance shader resource group");
            }

            const RHI::ShaderResourceGroupLayout* srgLayout = m_blendDistanceSrg->GetLayout();
            RHI::ShaderInputImageIndex imageIndex;

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeRayTrace"));
            m_blendDistanceSrg->SetImageView(imageIndex, m_rayTraceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeRayTraceImageViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeDistance"));
            m_blendDistanceSrg->SetImageView(imageIndex, m_distanceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeStates"));
            m_blendDistanceSrg->SetImageView(imageIndex, m_classificationImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeClassificationImageViewDescriptor).get());

            SetGridConstants(m_blendDistanceSrg);
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

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeRayTrace"));
            m_relocationSrg->SetImageView(imageIndex, m_rayTraceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeRayTraceImageViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeRelocation"));
            m_relocationSrg->SetImageView(imageIndex, m_relocationImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeRelocationImageViewDescriptor).get());

            float probeDistanceScale = (aznumeric_cast<float>(m_remainingRelocationIterations) / DefaultNumRelocationIterations);
            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeDistanceScale"));
            m_relocationSrg->SetConstant(constantIndex, probeDistanceScale);

            SetGridConstants(m_relocationSrg);
        }

        void DiffuseProbeGrid::UpdateClassificationSrg(const Data::Instance<RPI::Shader>& shader, const RHI::Ptr<RHI::ShaderResourceGroupLayout>& layout)
        {
            if (!m_classificationSrg)
            {
                m_classificationSrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
                AZ_Error("DiffuseProbeGrid", m_classificationSrg.get(), "Failed to create Classification shader resource group");
            }

            const RHI::ShaderResourceGroupLayout* srgLayout = m_classificationSrg->GetLayout();
            RHI::ShaderInputImageIndex imageIndex;

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeRayTrace"));
            m_classificationSrg->SetImageView(imageIndex, m_rayTraceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeRayTraceImageViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeStates"));
            m_classificationSrg->SetImageView(imageIndex, m_classificationImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeClassificationImageViewDescriptor).get());

            SetGridConstants(m_classificationSrg);
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

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_modelToWorld"));
            AZ::Matrix3x4 modelToWorld = AZ::Matrix3x4::CreateFromTransform(m_transform) * AZ::Matrix3x4::CreateScale(m_extents);
            m_renderObjectSrg->SetConstant(constantIndex, modelToWorld);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_modelToWorldInverse"));
            AZ::Matrix3x4 modelToWorldInverse = AZ::Matrix3x4::CreateFromTransform(m_transform).GetInverseFull();
            m_renderObjectSrg->SetConstant(constantIndex, modelToWorldInverse);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_obbHalfLengths"));
            m_renderObjectSrg->SetConstant(constantIndex, m_obbWs.GetHalfLengths());

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_enableDiffuseGI"));
            m_renderObjectSrg->SetConstant(constantIndex, m_enabled);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_ambientMultiplier"));
            m_renderObjectSrg->SetConstant(constantIndex, m_ambientMultiplier);

            imageIndex = srgLayout->FindShaderInputImageIndex(Name("m_probeIrradiance"));
            m_renderObjectSrg->SetImageView(imageIndex, GetIrradianceImage()->GetImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(Name("m_probeDistance"));
            m_renderObjectSrg->SetImageView(imageIndex, GetDistanceImage()->GetImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(Name("m_probeOffsets"));
            m_renderObjectSrg->SetImageView(imageIndex, GetRelocationImage()->GetImageView(m_renderData->m_probeRelocationImageViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(Name("m_probeStates"));
            m_renderObjectSrg->SetImageView(imageIndex, GetClassificationImage()->GetImageView(m_renderData->m_probeClassificationImageViewDescriptor).get());

            SetGridConstants(m_renderObjectSrg);

            m_updateRenderObjectSrg = false;

            // update culling now since the position and/or extents may have changed
            UpdateCulling();
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
