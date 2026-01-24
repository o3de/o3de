/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/SkyAtmosphereFeatureProcessor.h>
#include <Render/SkyAtmosphereParentPass.h>

#include <AzCore/Name/NameDictionary.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Pass/PassSystem.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>

namespace SkyAtmosphere
{
    void SkyAtmosphereFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext
                ->Class<SkyAtmosphereFeatureProcessor, FeatureProcessor>()
                ->Version(0);
        }
    }
    
    void SkyAtmosphereFeatureProcessor::Activate()
    {
        EnableSceneNotification();
    }
    
    void SkyAtmosphereFeatureProcessor::Deactivate()
    {
        DisableSceneNotification();

        m_atmospheres.Clear();
        m_renderPipelineToSkyAtmosphereParentPass.clear();
    }

    SkyAtmosphereFeatureProcessor::AtmosphereId SkyAtmosphereFeatureProcessor::CreateAtmosphere()
    {
        size_t index = m_atmospheres.Reserve();
        if (index >= std::numeric_limits<AtmosphereId::IndexType>::max())
        {
            m_atmospheres.Release(index);
            return AtmosphereId::Null;
        }

        AtmosphereId id = AtmosphereId(aznumeric_cast<AtmosphereId::IndexType>(index));
        InitializeAtmosphere(id);

        return id;
    }

    void SkyAtmosphereFeatureProcessor::ReleaseAtmosphere(AtmosphereId id)
    {
        if (id.IsValid())
        {
            m_atmospheres.Release(id.GetIndex());
        }

        for (auto& [_, pass] : m_renderPipelineToSkyAtmosphereParentPass)
            if (pass)
            {
                pass->ReleaseAtmospherePass(id);
            }
    }

    void SkyAtmosphereFeatureProcessor::SetAtmosphereParams(AtmosphereId id, const SkyAtmosphereParams& params)
    {
        auto& atmosphere = m_atmospheres.GetElement(id.GetIndex());
        atmosphere.m_params = params;
        atmosphere.m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetAtmosphereEnabled(AtmosphereId id, bool enabled)
    {
        if (id.IsValid())
        {
            auto& atmosphere = m_atmospheres.GetElement(id.GetIndex());
            atmosphere.m_enabled = enabled;
        }
    }

    bool SkyAtmosphereFeatureProcessor::GetAtmosphereEnabled(AtmosphereId id)
    {
        if (id.IsValid())
        {
            auto& atmosphere = m_atmospheres.GetElement(id.GetIndex());
            return atmosphere.m_enabled;
        }

        return false;
    }


    void SkyAtmosphereFeatureProcessor::InitializeAtmosphere(AtmosphereId id)
    {
        auto& atmosphere = m_atmospheres.GetElement(id.GetIndex());
        atmosphere.m_id = id;
        atmosphere.m_passNeedsUpdate = true;
        atmosphere.m_enabled = true;

        for (auto& [_, pass] : m_renderPipelineToSkyAtmosphereParentPass)
        {
            if (pass)
            {
                pass->CreateAtmospherePass(id);
            }
        }
    }

    void SkyAtmosphereFeatureProcessor::AddRenderPasses(AZ::RPI::RenderPipeline* renderPipeline)
    {
        if (m_renderPipelineToSkyAtmosphereParentPass.find(renderPipeline) != m_renderPipelineToSkyAtmosphereParentPass.end())
        {
            m_renderPipelineToSkyAtmosphereParentPass.erase(renderPipeline);
        }
        m_renderPipelineToSkyAtmosphereParentPass[renderPipeline] = nullptr;

        AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithTemplateName(AZ::Name("SkyAtmosphereParentTemplate"), renderPipeline);
        AZ::RPI::PassSystemInterface::Get()->ForEachPass(
            passFilter,
            [](AZ::RPI::Pass* pass) -> AZ::RPI::PassFilterExecutionFlow
            {
                AZ::RPI::PassSystemInterface::Get()->RemovePassFromLibrary(pass);
                return AZ::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
            });

        const char* passRequestAssetFilePath = "Passes/SkyAtmospherePassRequest.azasset";
        AZ::Data::Asset<AZ::RPI::AnyAsset> passRequestAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(
            passRequestAssetFilePath, AZ::RPI::AssetUtils::TraceLevel::Warning);

        const AZ::RPI::PassRequest *passRequest = nullptr;
        if (passRequestAsset->IsReady())
        {
            passRequest = passRequestAsset->GetDataAs<AZ::RPI::PassRequest>();
        }
        // Create the pass
        AZ::RPI::Ptr<AZ::RPI::Pass> parentPass = AZ::RPI::PassSystemInterface::Get()->CreatePassFromRequest(passRequest);
        if (!parentPass)
        {
            AZ_Error("SkyAtmosphere", false, "Create SkyAtmosphere parent pass from pass request failed");
            return;
        }

        // Insert the SkyAtmosphereParentPass after SkyBoxPass
        bool success = renderPipeline->AddPassAfter(parentPass, AZ::Name("SkyBoxPass"));
        // only create pass resources if it was success
        if (!success)
        {
            AZ_Error("SkyAtmosphere", false, "Add the SkyAtmosphere parent pass to render pipeline [%s] failed",
                renderPipeline->GetId().GetCStr());
        }
        m_renderPipelineToSkyAtmosphereParentPass[renderPipeline] = static_cast<SkyAtmosphereParentPass*>(parentPass.get());

        // make sure atmospheres are created if needed
        for (size_t i = 0; i < m_atmospheres.GetSize(); ++i)
        {
            auto& atmosphere = m_atmospheres.GetElement(i);
            if (atmosphere.m_id.IsValid() && atmosphere.m_enabled)
            {
                InitializeAtmosphere(atmosphere.m_id);
            }
        }
    }

    void SkyAtmosphereFeatureProcessor::OnRenderPipelineChanged([[maybe_unused]] AZ::RPI::RenderPipeline* pipeline,
        AZ::RPI::SceneNotification::RenderPipelineChangeType changeType)
    {
        if (changeType == AZ::RPI::SceneNotification::RenderPipelineChangeType::Added
            || changeType == AZ::RPI::SceneNotification::RenderPipelineChangeType::PassChanged)
        {
            UpdateBackgroundClearColor();
        }

        if (changeType == AZ::RPI::SceneNotification::RenderPipelineChangeType::Removed)
        {
            m_renderPipelineToSkyAtmosphereParentPass.erase(pipeline);
        }
    }
    
    void SkyAtmosphereFeatureProcessor::Render([[maybe_unused]] const FeatureProcessor::RenderPacket& packet)
    {
        AZ_PROFILE_SCOPE(RPI, "SkyAtmosphereFeatureProcessor: Render");

        for (size_t i = 0; i < m_atmospheres.GetSize(); ++i)
        {
            auto& atmosphere = m_atmospheres.GetElement(i);
            if (atmosphere.m_id.IsValid() && atmosphere.m_enabled && atmosphere.m_passNeedsUpdate)
            {
                // update every atmosphere parent pass (per-pipeline)
                for (auto& [_, pass] : m_renderPipelineToSkyAtmosphereParentPass)
                {
                    if (pass)
                    {
                        pass->UpdateAtmospherePassSRG(atmosphere.m_id, atmosphere.m_params);
                    }
                }

                atmosphere.m_passNeedsUpdate = false;
            }
        }
    }

    bool SkyAtmosphereFeatureProcessor::HasValidAtmosphere()
    {
        for (size_t i = 0; i < m_atmospheres.GetSize(); ++i)
        {
            const auto& atmosphere = m_atmospheres.GetElement(i);
            if (atmosphere.m_id.IsValid() && atmosphere.m_enabled)
            {
                return true;
            }
        }

        return false;
    }
    void SkyAtmosphereFeatureProcessor::UpdateBackgroundClearColor()
    {
        // don't update the background unless we have valid atmospheres
        if (!HasValidAtmosphere())
        {
            return;
        }

        // This function is only necessary for now because the default clear value
        // color is not black, and is set in various .pass files in places a user
        // is unlikely to find.  Unfortunately, the viewport will revert to the
        // grey color when resizing momentarily.
        const AZ::RHI::ClearValue blackClearValue = AZ::RHI::ClearValue::CreateVector4Float(0.f, 0.f, 0.f, 0.f);
        AZ::RPI::PassFilter passFilter;
        AZStd::string slot;

        auto setClearValue = [&](AZ::RPI::Pass* pass)-> AZ::RPI::PassFilterExecutionFlow
        {
            AZ::Name slotName = AZ::Name::FromStringLiteral(slot, AZ::Interface<AZ::NameDictionary>::Get());
            if (auto binding = pass->FindAttachmentBinding(slotName))
            {
                binding->m_unifiedScopeDesc.m_loadStoreAction.m_clearValue = blackClearValue;
            }
            return AZ::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
        };

        slot = "SpecularOutput";
        passFilter= AZ::RPI::PassFilter::CreateWithTemplateName(AZ::Name("ForwardPassTemplate"), GetParentScene());
        AZ::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, setClearValue);
        passFilter = AZ::RPI::PassFilter::CreateWithTemplateName(AZ::Name("ForwardMSAAPassTemplate"), GetParentScene());
        AZ::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, setClearValue);

        slot = "ReflectionOutput";
        passFilter = AZ::RPI::PassFilter::CreateWithTemplateName(AZ::Name("ReflectionGlobalFullscreenPassTemplate"), GetParentScene());
        AZ::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, setClearValue);
    }
}
