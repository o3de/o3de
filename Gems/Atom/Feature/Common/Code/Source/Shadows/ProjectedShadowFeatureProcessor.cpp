/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Shadows/ProjectedShadowFeatureProcessor.h>

#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Math/MatrixUtils.h>
#include <Math/GaussianMathFilter.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Pass/PassSystem.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
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
        
        m_shadowmapAtlasSizeIndex = viewSrgLayout->FindShaderInputConstantIndex(Name("m_shadowmapAtlasSize"));
        m_invShadowmapAtlasSizeIndex = viewSrgLayout->FindShaderInputConstantIndex(Name("m_invShadowmapAtlasSize"));

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
        m_esmShadowmapsPasses.clear();
        
        for (EsmShadowmapsPass* esmPass : m_esmShadowmapsPasses)
        {
            esmPass->SetEnabledComputation(false);
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

    void ProjectedShadowFeatureProcessor::SetShadowProperties(ShadowId id, const ProjectedShadowDescriptor& descriptor)
    {
        AZ_Assert(id.IsValid(), "Invalid ShadowId passed to ProjectedShadowFeatureProcessor::SetShadowProperties().");
        ShadowProperty& shadowProperty = GetShadowPropertyFromShadowId(id);
        shadowProperty.m_desc = descriptor;
        UpdateShadowView(shadowProperty);
        m_shadowmapPassNeedsUpdate = true;
        m_filterParameterNeedsUpdate = true;
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
        
        for (EsmShadowmapsPass* esmPass : m_esmShadowmapsPasses)
        {
            esmPass->SetEnabledComputation(esmData.m_isEnabled);
        }

        // Set depth bias matrix.
        const Matrix4x4& worldToLightClipMatrix = view->GetWorldToClipMatrix();
        const Matrix4x4 depthBiasMatrix = Shadow::GetClipToShadowmapTextureMatrix() * worldToLightClipMatrix;
        shadowData.m_depthBiasMatrix = depthBiasMatrix;
        
        shadowData.m_unprojectConstants[0] = view->GetViewToClipMatrix().GetRow(2).GetElement(2);
        shadowData.m_unprojectConstants[1] = view->GetViewToClipMatrix().GetRow(2).GetElement(3);

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
    }

    void ProjectedShadowFeatureProcessor::OnRenderPipelinePassesChanged(RPI::RenderPipeline* /*renderPipeline*/)
    {
        CachePasses();
    }

    void ProjectedShadowFeatureProcessor::OnRenderPipelineAdded(RPI::RenderPipelinePtr /*renderPipeline*/)
    {
        CachePasses();
    }

    void ProjectedShadowFeatureProcessor::OnRenderPipelineRemoved( RPI::RenderPipeline* /*renderPipeline*/)
    {
        CachePasses();
    }
    
    void ProjectedShadowFeatureProcessor::CachePasses()
    {
        CacheProjectedShadowmapsPass();
        CacheEsmShadowmapsPass();
        m_shadowmapPassNeedsUpdate = true;
    }
    
    void ProjectedShadowFeatureProcessor::CacheProjectedShadowmapsPass()
    {
        m_projectedShadowmapsPasses.clear();
        RPI::PassFilter passFilter = RPI::PassFilter::CreateWithTemplateName(Name("ProjectedShadowmapsTemplate"), GetParentScene());
        RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
            {
                ProjectedShadowmapsPass* shadowPass = static_cast<ProjectedShadowmapsPass*>(pass);
                m_projectedShadowmapsPasses.emplace_back(shadowPass);
                return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
            });
    }

    void ProjectedShadowFeatureProcessor::CacheEsmShadowmapsPass()
    {
        const Name LightTypeName = Name("projected");
                
        m_esmShadowmapsPasses.clear();
        RPI::PassFilter passFilter = RPI::PassFilter::CreateWithTemplateName(Name("EsmShadowmapsTemplate"), GetParentScene());
        RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this, LightTypeName](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
            {
                EsmShadowmapsPass* esmPass = static_cast<EsmShadowmapsPass*>(pass);
                if (esmPass->GetLightTypeName() == LightTypeName)
                {
                    m_esmShadowmapsPasses.emplace_back(esmPass);
                }
                return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
            });
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
        if (m_esmShadowmapsPasses.empty())
        {
            AZ_Error("ProjectedShadowFeatureProcessor", false, "Cannot find a required pass.");
            return;
        }

        if (m_shadowProperties.GetDataCount() == 0)
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
        }
    }

    void ProjectedShadowFeatureProcessor::SetFilterParameterToPass()
    {
        static uint32_t nameIndex = 0;
        if (m_projectedShadowmapsPasses.empty() || m_esmShadowmapsPasses.empty())
        {
            AZ_Error("ProjectedShadowFeatureProcessor", false, "Cannot find a required pass.");
            return;
        }

        // Create index table buffer.
        // [GFX TODO ATOM-14851] Should not be creating a new buffer here, just map the data or orphan with new data.
        const AZStd::string indexTableBufferName = AZStd::string::format("IndexTableBuffer(Projected) %d", nameIndex++);
        const ShadowmapAtlas& atlas = m_projectedShadowmapsPasses.front()->GetShadowmapAtlas();
        const Data::Instance<RPI::Buffer> indexTableBuffer = atlas.CreateShadowmapIndexTableBuffer(indexTableBufferName);

        m_filterParamBufferHandler.UpdateBuffer(m_shadowData.GetRawData<FilterParamIndex>(), static_cast<uint32_t>(m_shadowData.GetSize()));

        // Set index table buffer and ESM parameter buffer to ESM pass.
        for (EsmShadowmapsPass* esmPass : m_esmShadowmapsPasses)
        {
            esmPass->SetShadowmapIndexTableBuffer(indexTableBuffer);
            esmPass->SetFilterParameterBuffer(m_filterParamBufferHandler.GetBuffer());
        }
    }

    void ProjectedShadowFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& /*packet*/)
    {
        AZ_PROFILE_SCOPE(RPI, "ProjectedShadowFeatureProcessor: Simulate");

        if (m_shadowmapPassNeedsUpdate)
        {
            // Rebuild the shadow map sizes 
            AZStd::vector<ProjectedShadowmapsPass::ShadowmapSizeWithIndices> shadowmapSizes;
            shadowmapSizes.reserve(m_shadowProperties.GetDataCount());
            
            auto& shadowProperties = m_shadowProperties.GetDataVector();
            for (uint32_t i = 0; i < shadowProperties.size(); ++i)
            {
                ShadowProperty& shadowProperty = shadowProperties.at(i);
                uint16_t shadowIndex = shadowProperty.m_shadowId.GetIndex();
                FilterParameter& filterData = m_shadowData.GetElement<FilterParamIndex>(shadowIndex);

                shadowmapSizes.push_back();
                ProjectedShadowmapsPass::ShadowmapSizeWithIndices& sizeWithIndices = shadowmapSizes.back();
                sizeWithIndices.m_size = static_cast<ShadowmapSize>(filterData.m_shadowmapSize);
                sizeWithIndices.m_shadowIndexInSrg = shadowIndex;
            }

            for (ProjectedShadowmapsPass* shadowPass : m_projectedShadowmapsPasses)
            {
                shadowPass->UpdateShadowmapSizes(shadowmapSizes);
            }

            for (EsmShadowmapsPass* esmPass : m_esmShadowmapsPasses)
            {
                esmPass->QueueForBuildAndInitialization();
            }

            if (!m_projectedShadowmapsPasses.empty())
            {
                const ProjectedShadowmapsPass* shadowPass = m_projectedShadowmapsPasses.front();
                for (const auto& shadowProperty : shadowProperties)
                {
                    const int16_t shadowIndexInSrg = shadowProperty.m_shadowId.GetIndex();
                    ShadowData& shadowData = m_shadowData.GetElement<ShadowDataIndex>(shadowIndexInSrg);
                    FilterParameter& filterData = m_shadowData.GetElement<FilterParamIndex>(shadowIndexInSrg);
                    const ShadowmapAtlas::Origin origin = shadowPass->GetOriginInAtlas(shadowIndexInSrg);
                    
                    shadowData.m_shadowmapArraySlice = origin.m_arraySlice;
                    filterData.m_shadowmapOriginInSlice = origin.m_originInSlice;
                    m_deviceBufferNeedsUpdate = true;
                }
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
    }
    
    void ProjectedShadowFeatureProcessor::PrepareViews(const PrepareViewsPacket&, AZStd::vector<AZStd::pair<RPI::PipelineViewTag, RPI::ViewPtr>>& outViews)
    {
        if (!m_projectedShadowmapsPasses.empty())
        {
            ProjectedShadowmapsPass* pass = m_projectedShadowmapsPasses.front();
            RPI::RenderPipeline* renderPipeline = pass->GetRenderPipeline();
            if (renderPipeline)
            {
                auto& shadowProperties = m_shadowProperties.GetDataVector();
                for (uint32_t i = 0; i < shadowProperties.size(); ++i)
                {
                    ShadowProperty& shadowProperty = shadowProperties.at(i);
                    uint16_t shadowIndex = shadowProperty.m_shadowId.GetIndex();
                    const FilterParameter& filterData = m_shadowData.GetElement<FilterParamIndex>(shadowIndex);
                    if (filterData.m_shadowmapSize == aznumeric_cast<uint32_t>(ShadowmapSize::None))
                    {
                        continue;
                    }

                    const RPI::PipelineViewTag& viewTag = pass->GetPipelineViewTagOfChild(i);
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
    
    void ProjectedShadowFeatureProcessor::Render(const ProjectedShadowFeatureProcessor::RenderPacket& packet)
    {
        AZ_PROFILE_SCOPE(RPI, "ProjectedShadowFeatureProcessor: Render");

        if (!m_projectedShadowmapsPasses.empty())
        {
            const ProjectedShadowmapsPass* pass = m_projectedShadowmapsPasses.front();
            for (const RPI::ViewPtr& view : packet.m_views)
            {
                if (view->GetUsageFlags() & RPI::View::UsageFlags::UsageCamera)
                {
                    RPI::ShaderResourceGroup* srg = view->GetShaderResourceGroup().get();

                    srg->SetConstant(m_shadowmapAtlasSizeIndex, static_cast<float>(pass->GetShadowmapAtlasSize()));
                    const float invShadowmapSize = 1.0f / static_cast<float>(pass->GetShadowmapAtlasSize());
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

}
