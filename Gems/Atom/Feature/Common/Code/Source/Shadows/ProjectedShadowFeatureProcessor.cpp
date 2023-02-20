/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Shadows/ProjectedShadowFeatureProcessor.h>

#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Name/NameDictionary.h>
#include <Math/GaussianMathFilter.h>
#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassSystem.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/Feature/Mesh/MeshCommon.h>
#include <CoreLights/Shadow.h>

namespace AZ::Render
{
    void ProjectedShadowFeatureProcessor::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext
                ->Class<ProjectedShadowFeatureProcessor, FeatureProcessor>()
                ->Version(0);
        }
    }
    
    void ProjectedShadowFeatureProcessor::Activate()
    {
        const RHI::ShaderResourceGroupLayout* viewSrgLayout = RPI::RPISystemInterface::Get()->GetViewSrgLayout().get();

        GpuBufferHandler::Descriptor desc;

        desc.m_bufferName = "ProjectedShadowBuffer";
        desc.m_bufferSrgName = "m_projectedShadows";
        desc.m_elementCountSrgName = "";
        desc.m_elementSize = sizeof(ShadowData);
        desc.m_srgLayout = viewSrgLayout;

        m_shadowBufferHandler = GpuBufferHandler(desc);
        
        desc.m_bufferName = "ProjectedFilterParamsBuffer";
        desc.m_bufferSrgName = "m_projectedFilterParams";
        desc.m_elementCountSrgName = "";
        desc.m_elementSize = sizeof(EsmShadowmapsPass::FilterParameter);
        desc.m_srgLayout = viewSrgLayout;

        m_filterParamBufferHandler = GpuBufferHandler(desc);
        
        CachePasses();
        EnableSceneNotification();
    }
    
    void ProjectedShadowFeatureProcessor::Deactivate()
    {
        DisableSceneNotification();

        m_shadowData.Clear();
        m_shadowBufferHandler.Release();
        m_filterParamBufferHandler.Release();

        m_shadowProperties.Clear();

        m_projectedShadowmapsPasses.clear();
        m_primaryProjectedShadowmapsPass = nullptr;
        if (m_esmShadowmapsPass)
        {
            m_esmShadowmapsPass->SetEnabledComputation(false);
            m_esmShadowmapsPass = nullptr;
        }
    }

    ProjectedShadowFeatureProcessor::ShadowId ProjectedShadowFeatureProcessor::AcquireShadow()
    {
        // Reserve a new slot in m_shadowData
        size_t index = m_shadowData.Reserve();
        if (index >= std::numeric_limits<ShadowId::IndexType>::max())
        {
            m_shadowData.Release(index);
            return ShadowId::Null;
        }

        ShadowId id = ShadowId(aznumeric_cast<ShadowId::IndexType>(index));
        InitializeShadow(id);

        return id;
    }

    void ProjectedShadowFeatureProcessor::ReleaseShadow(ShadowId id)
    {
        if (id.IsValid())
        {
            m_shadowProperties.RemoveIndex(m_shadowData.GetElement<ShadowPropertyIdIndex>(id.GetIndex()));
            auto& shadowmapPass = m_shadowData.GetElement<ShadowPassIndex>(id.GetIndex());
            m_primaryProjectedShadowmapsPass->QueueRemoveChild(shadowmapPass);
            AZ_Printf("ProjectedShadowFeatureProcessor", "Removing %s, %p", shadowmapPass->GetName().GetCStr(), shadowmapPass.get());
            m_shadowData.Release(id.GetIndex());
        }

        m_filterParameterNeedsUpdate = true;
        m_shadowmapPassNeedsUpdate = true;
    }

    void ProjectedShadowFeatureProcessor::SetShadowTransform(ShadowId id, Transform transform)
    {
        ShadowProperty& shadowProperty = GetShadowPropertyFromShadowId(id);
        shadowProperty.m_desc.m_transform = transform;
        UpdateShadowView(shadowProperty);
    }
    
    void ProjectedShadowFeatureProcessor::SetNearFarPlanes(ShadowId id, float nearPlaneDistance, float farPlaneDistance)
    {
        AZ_Assert(id.IsValid(), "Invalid ShadowId passed to ProjectedShadowFeatureProcessor::SetFrontBackPlanes().");
        
        ShadowProperty& shadowProperty = GetShadowPropertyFromShadowId(id);
        shadowProperty.m_desc.m_nearPlaneDistance = GetMax(nearPlaneDistance, 0.0001f);
        shadowProperty.m_desc.m_farPlaneDistance = GetMax(farPlaneDistance, nearPlaneDistance + 0.0001f);
        UpdateShadowView(shadowProperty);
    }
    
    void ProjectedShadowFeatureProcessor::SetAspectRatio(ShadowId id, float aspectRatio)
    {
        AZ_Assert(id.IsValid(), "Invalid ShadowId passed to ProjectedShadowFeatureProcessor::SetAspectRatio().");
        
        ShadowProperty& shadowProperty = GetShadowPropertyFromShadowId(id);
        shadowProperty.m_desc.m_aspectRatio = aspectRatio;
        UpdateShadowView(shadowProperty);
    }

    void ProjectedShadowFeatureProcessor::SetFieldOfViewY(ShadowId id, float fieldOfViewYRadians)
    {
        AZ_Assert(id.IsValid(), "Invalid ShadowId passed to ProjectedShadowFeatureProcessor::SetFieldOfViewY().");
        
        ShadowProperty& shadowProperty = GetShadowPropertyFromShadowId(id);
        shadowProperty.m_desc.m_fieldOfViewYRadians = fieldOfViewYRadians;
        UpdateShadowView(shadowProperty);
    }

    void ProjectedShadowFeatureProcessor::SetShadowBias(ShadowId id, float bias)
    {
        AZ_Assert(id.IsValid(), "Invalid ShadowId passed to ProjectedShadowFeatureProcessor::SetShadowBias().");
        
        ShadowProperty& shadowProperty = GetShadowPropertyFromShadowId(id);
        shadowProperty.m_bias = bias;
    }

    void ProjectedShadowFeatureProcessor::SetNormalShadowBias(ShadowId id, float normalShadowBias)
    {
        AZ_Assert(id.IsValid(), "Invalid ShadowId passed to ProjectedShadowFeatureProcessor::SetNormalShadowBias().");

        ShadowData& shadowData = m_shadowData.GetElement<ShadowDataIndex>(id.GetIndex());
        shadowData.m_normalShadowBias = normalShadowBias;
        m_deviceBufferNeedsUpdate = true;
    }

    void ProjectedShadowFeatureProcessor::SetShadowmapMaxResolution(ShadowId id, ShadowmapSize size)
    {
        AZ_Assert(id.IsValid(), "Invalid ShadowId passed to ProjectedShadowFeatureProcessor::SetShadowmapMaxResolution().");
        AZ_Assert(size != ShadowmapSize::None, "Shadowmap size cannot be set to None, remove the shadow instead.");
        
        FilterParameter& esmData = m_shadowData.GetElement<FilterParamIndex>(id.GetIndex());
        esmData.m_shadowmapSize = aznumeric_cast<uint32_t>(size);
        
        m_deviceBufferNeedsUpdate = true;
        m_shadowmapPassNeedsUpdate = true;
        m_filterParameterNeedsUpdate = true;
    }
    
    void ProjectedShadowFeatureProcessor::SetEsmExponent(ShadowId id, float exponent)
    {
        AZ_Assert(id.IsValid(), "Invalid ShadowId passed to ProjectedShadowFeatureProcessor::SetEsmExponent().");
        ShadowData& shadowData = m_shadowData.GetElement<ShadowDataIndex>(id.GetIndex());
        shadowData.m_esmExponent = exponent;
        m_deviceBufferNeedsUpdate = true;
    }

    void ProjectedShadowFeatureProcessor::SetShadowFilterMethod(ShadowId id, ShadowFilterMethod method)
    {
        AZ_Assert(id.IsValid(), "Invalid ShadowId passed to ProjectedShadowFeatureProcessor::SetShadowFilterMethod().");
        
        ShadowProperty& shadowProperty = GetShadowPropertyFromShadowId(id);
        ShadowData& shadowData = m_shadowData.GetElement<ShadowDataIndex>(id.GetIndex());
        shadowData.m_shadowFilterMethod = aznumeric_cast<uint32_t>(method);

        UpdateShadowView(shadowProperty);
        
        m_shadowmapPassNeedsUpdate = true;
        m_filterParameterNeedsUpdate = true;
    }
    
    void ProjectedShadowFeatureProcessor::SetFilteringSampleCount(ShadowId id, uint16_t count)
    {
        AZ_Assert(id.IsValid(), "Invalid ShadowId passed to ProjectedShadowFeatureProcessor::SetFilteringSampleCount().");
        
        AZ_Warning("ProjectedShadowFeatureProcessor", count <= Shadow::MaxPcfSamplingCount, "Sampling count exceed the limit.");
        count = GetMin(count, Shadow::MaxPcfSamplingCount);
        
        ShadowData& shadowData = m_shadowData.GetElement<ShadowDataIndex>(id.GetIndex());
        shadowData.m_filteringSampleCount = count;

        m_deviceBufferNeedsUpdate = true;
    }

    void ProjectedShadowFeatureProcessor::SetUseCachedShadows(ShadowId id, bool useCachedShadows)
    {
        AZ_Assert(id.IsValid(), "Invalid ShadowId passed to ProjectedShadowFeatureProcessor::SetUseCachedShadows().");
        ShadowProperty& shadowProperty = GetShadowPropertyFromShadowId(id);
        shadowProperty.m_useCachedShadows = useCachedShadows;
        m_shadowmapPassNeedsUpdate = true;
    }

    void ProjectedShadowFeatureProcessor::SetShadowProperties(ShadowId id, const ProjectedShadowDescriptor& descriptor)
    {
        AZ_Assert(id.IsValid(), "Invalid ShadowId passed to ProjectedShadowFeatureProcessor::SetShadowProperties().");
        ShadowProperty& shadowProperty = GetShadowPropertyFromShadowId(id);

        if (shadowProperty.m_desc != descriptor)
        {
            shadowProperty.m_desc = descriptor;
            UpdateShadowView(shadowProperty);
            // Don't set m_shadowmapPassNeedsUpdate=true here because that would cause the pass to rebuild every time a light moves
            // Don't set m_filterParameterNeedsUpdate=true here because that's handled by UpdateShadowView(), and only when filtering is relevant
        }
    }
    
    auto ProjectedShadowFeatureProcessor::GetShadowProperties(ShadowId id) -> const ProjectedShadowDescriptor&
    {
        AZ_Assert(id.IsValid(), "Invalid ShadowId passed to ProjectedShadowFeatureProcessor::GetShadowProperties().");
        return GetShadowPropertyFromShadowId(id).m_desc;
    }

    void ProjectedShadowFeatureProcessor::UpdateShadowView(ShadowProperty& shadowProperty)
    {
        const ProjectedShadowDescriptor& desc = shadowProperty.m_desc;
        float nearDist = desc.m_nearPlaneDistance;
        float farDist = desc.m_farPlaneDistance;

        // Adjust the near plane if it's too close to ensure accuracy.
        constexpr float NearFarRatio = 1000.0f;
        const float minDist = desc.m_farPlaneDistance / NearFarRatio;
        nearDist = GetMax(minDist, nearDist); 

        Matrix4x4 viewToClipMatrix;
        MakePerspectiveFovMatrixRH(
            viewToClipMatrix,
            GetMax(desc.m_fieldOfViewYRadians, MinimumFieldOfView),
            desc.m_aspectRatio,
            nearDist,
            farDist);

        RPI::ViewPtr view = shadowProperty.m_shadowmapView;
        view->SetViewToClipMatrix(viewToClipMatrix);
        view->SetCameraTransform(Matrix3x4::CreateFromTransform(desc.m_transform));

        ShadowData& shadowData = m_shadowData.GetElement<ShadowDataIndex>(shadowProperty.m_shadowId.GetIndex());

        // Adjust the manually set bias to a more appropriate range for the shader. Scale the bias by the
        // near plane so that the bias appears consistent as other light properties change.
        shadowData.m_bias = nearDist * shadowProperty.m_bias * 0.01f;
        
        FilterParameter& esmData = m_shadowData.GetElement<FilterParamIndex>(shadowProperty.m_shadowId.GetIndex());
        
        // Set parameters to calculate linear depth if ESM is used.
        esmData.m_n_f_n = nearDist / (farDist - nearDist);
        esmData.m_n_f = nearDist - farDist;
        esmData.m_f = farDist;
        esmData.m_isEnabled = FilterMethodIsEsm(shadowData);
        m_filterParameterNeedsUpdate = m_filterParameterNeedsUpdate || esmData.m_isEnabled;

        // Set depth bias matrix.
        const Matrix4x4& worldToLightClipMatrix = view->GetWorldToClipMatrix();
        const Matrix4x4 depthBiasMatrix = Shadow::GetClipToShadowmapTextureMatrix() * worldToLightClipMatrix;
        shadowData.m_depthBiasMatrix = depthBiasMatrix;
        
        shadowData.m_unprojectConstants[0] = view->GetViewToClipMatrix().GetRow(2).GetElement(2);
        shadowData.m_unprojectConstants[1] = view->GetViewToClipMatrix().GetRow(2).GetElement(3);

        if (shadowProperty.m_useCachedShadows && m_primaryProjectedShadowmapsPass)
        {
            m_shadowData.GetElement<ShadowPassIndex>(shadowProperty.m_shadowId.GetIndex())->ForceRenderNextFrame();
        }

        m_deviceBufferNeedsUpdate = true;
    }

    void ProjectedShadowFeatureProcessor::InitializeShadow(ShadowId shadowId)
    {
        m_deviceBufferNeedsUpdate = true;
        m_shadowmapPassNeedsUpdate = true;
        
        // Reserve a slot in m_shadowProperties, and store that index in m_shadowData's second vector
        uint16_t shadowPropertyIndex = m_shadowProperties.GetFreeSlotIndex();
        m_shadowData.GetElement<ShadowPropertyIdIndex>(shadowId.GetIndex()) = shadowPropertyIndex;

        ShadowProperty& shadowProperty = m_shadowProperties.GetData(shadowPropertyIndex);
        shadowProperty.m_shadowId = shadowId;

        AZ::Name viewName(AZStd::string::format("ProjectedShadowView (shadowId:%d)", shadowId.GetIndex()));
        shadowProperty.m_shadowmapView = RPI::View::CreateView(viewName, RPI::View::UsageShadow);

        UpdateShadowView(shadowProperty);

        RPI::Ptr<ShadowmapPass> shadowmapPass = CreateShadowmapPass(shadowId.GetIndex());
        m_shadowData.GetElement<ShadowPassIndex>(shadowId.GetIndex()) = shadowmapPass;
        m_primaryProjectedShadowmapsPass->QueueAddChild(shadowmapPass);
        AZ_Printf("ProjectedShadowFeatureProcessor", "Adding %s, %p", shadowmapPass->GetName().GetCStr(), shadowmapPass.get());
    }
        
    void ProjectedShadowFeatureProcessor::OnRenderPipelineChanged([[maybe_unused]] RPI::RenderPipeline* renderPipeline,
            [[maybe_unused]] RPI::SceneNotification::RenderPipelineChangeType changeType)
    {
        CachePasses();
    }
    
    void ProjectedShadowFeatureProcessor::CachePasses()
    {
        {
            // Projected shadow maps pass
            m_projectedShadowmapsPasses.clear();

            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithTemplateName(AZ_NAME_LITERAL("ProjectedShadowmapsTemplate"), GetParentScene());
            RPI::PassSystemInterface::Get()->ForEachPass(passFilter,
                [&](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                {
                    ProjectedShadowmapsPass* shadowmapPass = static_cast<ProjectedShadowmapsPass*>(pass);
                    m_projectedShadowmapsPasses.push_back(shadowmapPass);
                    shadowmapPass->SetAtlasAttachmentImage(m_atlasImage);
                    shadowmapPass->QueueForBuildAndInitialization();
                    return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                }
            );

            if (!m_projectedShadowmapsPasses.empty())
            {
                if (m_primaryProjectedShadowmapsPass == nullptr)
                {
                    m_primaryProjectedShadowmapsPass = m_projectedShadowmapsPasses.front();
                }
                else
                {
                    auto it = AZStd::find(m_projectedShadowmapsPasses.begin(), m_projectedShadowmapsPasses.end(), m_primaryProjectedShadowmapsPass);
                    if (it == m_projectedShadowmapsPasses.end())
                    {
                        m_primaryProjectedShadowmapsPass = m_projectedShadowmapsPasses.front();

                        for (const auto& shadowProperty : m_shadowProperties.GetDataVector())
                        {
                            size_t shadowIndex = shadowProperty.m_shadowId.GetIndex();
                            auto& shadowmapPass = m_shadowData.GetElement<ShadowPassIndex>(shadowIndex);
                            m_primaryProjectedShadowmapsPass->QueueAddChild(shadowmapPass);
                        }
                    }
                }
            }
            else
            {
                m_primaryProjectedShadowmapsPass = nullptr;
            }
        }

        // ESM shadowmap pass

        RPI::PassAttachmentBinding* esmOutputBinding = nullptr;
        m_esmShadowmapsPass = nullptr;
        if (m_primaryProjectedShadowmapsPass)
        {
            AZStd::vector<EsmShadowmapsPass*> esmPasses;

            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithTemplateName(AZ_NAME_LITERAL("EsmShadowmapsTemplate"), GetParentScene());
            RPI::PassSystemInterface::Get()->ForEachPass(passFilter,
                [&](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                {
                    EsmShadowmapsPass* esmShadowmapsPass = static_cast<EsmShadowmapsPass*>(pass);
                    if (esmShadowmapsPass->GetLightTypeName() == AZ::Name("projected"))
                    {
                        if (esmShadowmapsPass->GetRenderPipeline() == m_primaryProjectedShadowmapsPass->GetRenderPipeline())
                        {
                            m_esmShadowmapsPass = esmShadowmapsPass;
                            esmOutputBinding = &m_esmShadowmapsPass->GetOutputBinding(0);
                            m_filterParameterNeedsUpdate = m_shadowProperties.GetDataCount() > 0;
                        }
                        else
                        {
                            esmPasses.push_back(esmShadowmapsPass);
                            esmShadowmapsPass->SetEnabledComputation(false);
                        }
                    }
                    return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                }
            );

            if (m_esmShadowmapsPass && esmOutputBinding)
            {
                for (EsmShadowmapsPass* otherEsmPass : esmPasses)
                {
                    otherEsmPass->SetOutputOverride(esmOutputBinding->GetAttachment());
                }
            }
        }

        if (m_primaryProjectedShadowmapsPass && !m_clearShadowDrawPacket)
        {
            CreateClearShadowDrawPacket();
        }

        m_shadowmapPassNeedsUpdate = true;
    }
    
    void ProjectedShadowFeatureProcessor::UpdateFilterParameters()
    {
        if (m_filterParameterNeedsUpdate)
        {
            UpdateEsmPassEnabled();
            SetFilterParameterToPass();
            m_filterParameterNeedsUpdate = false;
        }
    }
    
    void ProjectedShadowFeatureProcessor::UpdateEsmPassEnabled()
    {
        if (m_esmShadowmapsPass == nullptr)
        {
            AZ_Error("ProjectedShadowFeatureProcessor", false, "Cannot find a required pass.");
            return;
        }

        bool anyShadowsUseEsm = false;
        for (const auto& shadowProperty : m_shadowProperties.GetDataVector())
        {
            FilterParameter& esmData = m_shadowData.GetElement<FilterParamIndex>(shadowProperty.m_shadowId.GetIndex());
            if (esmData.m_isEnabled)
            {
                anyShadowsUseEsm = true;
                break;
            }
        }

        m_esmShadowmapsPass->SetEnabledComputation(anyShadowsUseEsm);
    }

    void ProjectedShadowFeatureProcessor::SetFilterParameterToPass()
    {
        static uint32_t nameIndex = 0;
        if (m_primaryProjectedShadowmapsPass == nullptr || m_esmShadowmapsPass == nullptr)
        {
            AZ_Error("ProjectedShadowFeatureProcessor", false, "Cannot find a required pass.");
            return;
        }

        // Create index table buffer.
        // [GFX TODO ATOM-14851] Should not be creating a new buffer here, just map the data or orphan with new data.
        const AZStd::string indexTableBufferName = AZStd::string::format("IndexTableBuffer(Projected) %d", nameIndex++);
        const Data::Instance<RPI::Buffer> indexTableBuffer = m_atlas.CreateShadowmapIndexTableBuffer(indexTableBufferName);

        m_filterParamBufferHandler.UpdateBuffer(m_shadowData.GetRawData<FilterParamIndex>(), static_cast<uint32_t>(m_shadowData.GetSize()));

        m_esmShadowmapsPass->SetShadowmapIndexTableBuffer(indexTableBuffer);
        m_esmShadowmapsPass->SetFilterParameterBuffer(m_filterParamBufferHandler.GetBuffer());
    }

    void ProjectedShadowFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& /*packet*/)
    {
        AZ_PROFILE_SCOPE(RPI, "ProjectedShadowFeatureProcessor: Simulate");

        if (m_shadowmapPassNeedsUpdate && m_primaryProjectedShadowmapsPass)
        {
            UpdateAtlas();
            UpdateShadowPasses();

            auto& shadowProperties = m_shadowProperties.GetDataVector();
            for (const auto& shadowProperty : shadowProperties)
            {
                const int16_t shadowIndexInSrg = shadowProperty.m_shadowId.GetIndex();
                ShadowData& shadowData = m_shadowData.GetElement<ShadowDataIndex>(shadowIndexInSrg);
                FilterParameter& filterData = m_shadowData.GetElement<FilterParamIndex>(shadowIndexInSrg);
                const ShadowmapAtlas::Origin origin = m_atlas.GetOrigin(shadowIndexInSrg);

                shadowData.m_shadowmapArraySlice = origin.m_arraySlice;
                filterData.m_shadowmapOriginInSlice = origin.m_originInSlice;
                m_deviceBufferNeedsUpdate = true;
            }

            if (m_esmShadowmapsPass != nullptr)
            {
                m_esmShadowmapsPass->QueueForBuildAndInitialization();
            }

            m_shadowmapPassNeedsUpdate = false;
        }

        // This has to be called after UpdateShadowmapSizes().
        UpdateFilterParameters();

        if (m_deviceBufferNeedsUpdate)
        {
            m_shadowBufferHandler.UpdateBuffer(m_shadowData.GetRawData<ShadowDataIndex>(), static_cast<uint32_t>(m_shadowData.GetSize()));
            m_deviceBufferNeedsUpdate = false;
        }

        // Turn off cached esm shadow maps for next frame
        for (const auto& shadowProperty : m_shadowProperties.GetDataVector())
        {
            if (shadowProperty.m_useCachedShadows)
            {
                FilterParameter& esmData = m_shadowData.GetElement<FilterParamIndex>(shadowProperty.m_shadowId.GetIndex());
                if (esmData.m_isEnabled != 0)
                {
                    esmData.m_isEnabled = false;
                    m_filterParameterNeedsUpdate = true;
                }
            }
        }
    }
    
    void ProjectedShadowFeatureProcessor::PrepareViews(const PrepareViewsPacket&, AZStd::vector<AZStd::pair<RPI::PipelineViewTag, RPI::ViewPtr>>& outViews)
    {
        if (m_primaryProjectedShadowmapsPass != nullptr)
        {
            RPI::RenderPipeline* renderPipeline = m_primaryProjectedShadowmapsPass->GetRenderPipeline();
            if (renderPipeline)
            {
                auto& shadowProperties = m_shadowProperties.GetDataVector();
                for (ShadowProperty& shadowProperty : shadowProperties)
                {
                    uint16_t shadowIndex = shadowProperty.m_shadowId.GetIndex();
                    const FilterParameter& filterData = m_shadowData.GetElement<FilterParamIndex>(shadowIndex);
                    if (filterData.m_shadowmapSize == aznumeric_cast<uint32_t>(ShadowmapSize::None))
                    {
                        continue;
                    }

                    const RPI::PipelineViewTag& viewTag = m_shadowData.GetElement<ShadowPassIndex>(shadowIndex)->GetPipelineViewTag();
                    const RHI::DrawListMask drawListMask = renderPipeline->GetDrawListMask(viewTag);
                    if (shadowProperty.m_shadowmapView->GetDrawListMask() != drawListMask)
                    {
                        shadowProperty.m_shadowmapView->Reset();
                        shadowProperty.m_shadowmapView->SetDrawListMask(drawListMask);
                    }

                    outViews.emplace_back(AZStd::make_pair(viewTag, shadowProperty.m_shadowmapView));
                }
            }
        }
    }
    
    void ProjectedShadowFeatureProcessor::Render(const FeatureProcessor::RenderPacket& packet)
    {
        AZ_PROFILE_SCOPE(RPI, "ProjectedShadowFeatureProcessor: Render");

        if (m_primaryProjectedShadowmapsPass != nullptr)
        {
            for (const RPI::ViewPtr& view : packet.m_views)
            {
                if (view->GetUsageFlags() & RPI::View::UsageFlags::UsageCamera)
                {
                    RPI::ShaderResourceGroup* srg = view->GetShaderResourceGroup().get();

                    float shadowMapAtlasSize = static_cast<float>(m_atlas.GetBaseShadowmapSize());

                    srg->SetConstant(m_shadowmapAtlasSizeIndex, shadowMapAtlasSize);
                    const float invShadowmapSize = 1.0f / shadowMapAtlasSize;
                    srg->SetConstant(m_invShadowmapAtlasSizeIndex, invShadowmapSize);

                    m_shadowBufferHandler.UpdateSrg(srg);
                    m_filterParamBufferHandler.UpdateSrg(srg);
                }
            }
        }
    }

    bool ProjectedShadowFeatureProcessor::FilterMethodIsEsm(const ShadowData& shadowData) const
    {
        return
            aznumeric_cast<ShadowFilterMethod>(shadowData.m_shadowFilterMethod) == ShadowFilterMethod::Esm ||
            aznumeric_cast<ShadowFilterMethod>(shadowData.m_shadowFilterMethod) == ShadowFilterMethod::EsmPcf;
    }
    
    auto ProjectedShadowFeatureProcessor::GetShadowPropertyFromShadowId(ShadowId id) -> ShadowProperty&
    {
        AZ_Assert(id.IsValid(), "Error: Invalid ShadowId");
        uint16_t shadowPropertyId = m_shadowData.GetElement<ShadowPropertyIdIndex>(id.GetIndex());
        return m_shadowProperties.GetData(shadowPropertyId);
    }

    void ProjectedShadowFeatureProcessor::CreateClearShadowDrawPacket()
    {
        // Force load of shader to clear shadow maps.
        const AZStd::string clearShadowShaderFilePath = "Shaders/Shadow/ClearShadow.azshader";
        Data::Asset<RPI::ShaderAsset> shaderAsset = RPI::AssetUtils::LoadCriticalAsset<RPI::ShaderAsset>
            (clearShadowShaderFilePath, RPI::AssetUtils::TraceLevel::Assert);

        m_clearShadowShader = AZ::RPI::Shader::FindOrCreate(shaderAsset);
        const AZ::RPI::ShaderVariant& variant = m_clearShadowShader->GetRootVariant();

        AZ::RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
        variant.ConfigurePipelineState(pipelineStateDescriptor);

        bool foundPipelineState = GetParentScene()->ConfigurePipelineState(m_clearShadowShader->GetDrawListTag(), pipelineStateDescriptor);
        AZ_Assert(foundPipelineState, "Could not find pipeline state for ClearShadow shader's draw list '%s'", shaderAsset->GetDrawListName().GetCStr())

        RHI::InputStreamLayoutBuilder layoutBuilder;
        pipelineStateDescriptor.m_inputStreamLayout = layoutBuilder.End();

        const AZ::RHI::PipelineState* pipelineState = m_clearShadowShader->AcquirePipelineState(pipelineStateDescriptor);
        if (!pipelineState)
        {
            AZ_Assert(false, "Shader '%s'. Failed to acquire default pipeline state", shaderAsset->GetName().GetCStr());
            return;
        }

        AZ::RHI::DrawPacketBuilder drawPacketBuilder;
        drawPacketBuilder.Begin(nullptr);
        drawPacketBuilder.SetDrawArguments(AZ::RHI::DrawLinear(1, 0, 3, 0));

        AZ::RHI::DrawPacketBuilder::DrawRequest drawRequest;
        drawRequest.m_listTag = m_clearShadowShader->GetDrawListTag();
        drawRequest.m_pipelineState = pipelineState;
        drawRequest.m_sortKey = AZStd::numeric_limits<RHI::DrawItemSortKey>::min();

        drawPacketBuilder.AddDrawItem(drawRequest);
        m_clearShadowDrawPacket = drawPacketBuilder.End();
    }

    void ProjectedShadowFeatureProcessor::UpdateAtlas()
    {
        // Currently when something changes, the atlas is completely reset. This is ok when most shadows are dynamic,
        // but isn't ideal for cached shadows which will need to re-render on the next frame.

        m_atlas.Initialize();
        auto& shadowProperties = m_shadowProperties.GetDataVector();
        for (const auto& shadowProperty : shadowProperties)
        {
            uint16_t shadowIndex = shadowProperty.m_shadowId.GetIndex();
            FilterParameter& filterData = m_shadowData.GetElement<FilterParamIndex>(shadowIndex);
            m_atlas.SetShadowmapSize(shadowIndex, static_cast<ShadowmapSize>(filterData.m_shadowmapSize));
        }
        m_atlas.Finalize();

        m_atlasImage = {};
        RHI::ImageDescriptor imageDescriptor;
        const uint32_t shadowmapSize = static_cast<uint32_t>(m_atlas.GetBaseShadowmapSize());
        imageDescriptor.m_size = RHI::Size(shadowmapSize, shadowmapSize, 1);
        imageDescriptor.m_format = RHI::Format::D32_FLOAT;
        imageDescriptor.m_arraySize = m_atlas.GetArraySliceCount();
        imageDescriptor.m_bindFlags |= RHI::ImageBindFlags::Depth;
        imageDescriptor.m_sharedQueueMask = RHI::HardwareQueueClassMask::Graphics;

        // The ImageViewDescriptor must be specified to make sure the frame graph compiler doesn't treat this as a transient image.
        RHI::ImageViewDescriptor viewDesc = RHI::ImageViewDescriptor::Create(imageDescriptor.m_format, 0, 0);
        viewDesc.m_aspectFlags = RHI::ImageAspectFlags::Depth;

        RPI::CreateAttachmentImageRequest createImageRequest;
        createImageRequest.m_imagePool = RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool().get();
        createImageRequest.m_imageDescriptor = imageDescriptor;
        createImageRequest.m_imageName = AZStd::string::format("ProjectedShadowAtlas.%s", GetParentScene()->GetName().GetCStr());
        createImageRequest.m_imageViewDescriptor = &viewDesc;
        m_atlasImage = RPI::AttachmentImage::Create(createImageRequest);

        for (auto& projectedShadowmapsPass : m_projectedShadowmapsPasses)
        {
            projectedShadowmapsPass->SetAtlasAttachmentImage(m_atlasImage);
            projectedShadowmapsPass->QueueForBuildAndInitialization();
        }
    }

    RPI::Ptr<ShadowmapPass> ProjectedShadowFeatureProcessor::CreateShadowmapPass(size_t childIndex)
    {
        const Name passName{ AZStd::string::format("ProjectedShadowmapPass.%zu", childIndex) };

        RHI::RHISystemInterface* rhiSystem = RHI::RHISystemInterface::Get();
        auto passData = AZStd::make_shared<RPI::RasterPassData>();
        passData->m_drawListTag = rhiSystem->GetDrawListTagRegistry()->GetName(m_primaryProjectedShadowmapsPass->GetDrawListTag());
        passData->m_pipelineViewTag = AZStd::string::format("%s.%zu", m_primaryProjectedShadowmapsPass->GetPipelineViewTag().GetCStr(), childIndex);

        return ShadowmapPass::CreateWithPassRequest(passName, passData);
    }

    void ProjectedShadowFeatureProcessor::UpdateShadowPasses()
    {
        struct SliceInfo
        {
            bool m_hasStaticShadows = false;
            AZStd::vector<ShadowmapPass*> m_shadowPasses;
        };

        AZStd::vector<SliceInfo> sliceInfo(m_atlas.GetArraySliceCount());
        for (const auto& it : m_shadowProperties.GetDataVector())
        {

            // This index indicates the execution order of the passes.
            // The first pass to render a slice should clear the slice.
            size_t shadowIndex = it.m_shadowId.GetIndex();
            auto* pass = m_shadowData.GetElement<ShadowPassIndex>(shadowIndex).get();

            const ShadowmapAtlas::Origin origin = m_atlas.GetOrigin(shadowIndex);
            pass->SetArraySlice(origin.m_arraySlice);
            pass->SetIsStatic(it.m_useCachedShadows);
            pass->ForceRenderNextFrame();

            const auto& filterData = m_shadowData.GetElement<FilterParamIndex>(shadowIndex);
            if (filterData.m_shadowmapSize != static_cast<uint32_t>(ShadowmapSize::None))
            {
                const RHI::Viewport viewport(
                    origin.m_originInSlice[0] * 1.f,
                    (origin.m_originInSlice[0] + filterData.m_shadowmapSize) * 1.f,
                    origin.m_originInSlice[1] * 1.f,
                    (origin.m_originInSlice[1] + filterData.m_shadowmapSize) * 1.f);
                const RHI::Scissor scissor(
                    origin.m_originInSlice[0],
                    origin.m_originInSlice[1],
                    origin.m_originInSlice[0] + filterData.m_shadowmapSize,
                    origin.m_originInSlice[1] + filterData.m_shadowmapSize);
                pass->SetViewportScissor(viewport, scissor);
                pass->SetClearEnabled(false);

                SliceInfo& sliceInfoItem = sliceInfo.at(origin.m_arraySlice);
                sliceInfoItem.m_shadowPasses.push_back(pass);
                sliceInfoItem.m_hasStaticShadows = sliceInfoItem.m_hasStaticShadows || it.m_useCachedShadows;
            }
        }

        RHI::Handle<uint32_t> casterMovedBit = GetParentScene()->GetViewTagBitRegistry().FindTag(MeshCommon::MeshMovedName);

        for (const auto& it : sliceInfo)
        {
            if (!it.m_hasStaticShadows)
            {
                if (!it.m_shadowPasses.empty())
                {
                    // no static shadows in this slice, so have the first pass clear the atlas on load.
                    it.m_shadowPasses.at(0)->SetClearEnabled(true);
                }
            }
            else
            {
                // There's at least one static shadow in this slice, so passes need to clear themselves using a draw.
                for (auto* pass : it.m_shadowPasses)
                {
                    pass->SetClearShadowDrawPacket(m_clearShadowDrawPacket);
                    pass->SetCasterMovedBit(casterMovedBit);
                }
            }
        }
    }

}
