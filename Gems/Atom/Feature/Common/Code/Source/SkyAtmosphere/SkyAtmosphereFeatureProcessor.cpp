/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkyAtmosphere/SkyAtmosphereFeatureProcessor.h>
#include <SkyAtmosphere/SkyAtmosphereParentPass.h>

#include <AzCore/Name/NameDictionary.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Pass/PassSystem.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>

namespace AZ::Render
{
    void SkyAtmosphereFeatureProcessor::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
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
        m_renderPipelineToSkyAtmosphereParentPasses.clear();
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

        for (auto& [_, skyAtmosphereParentPasses] : m_renderPipelineToSkyAtmosphereParentPasses)
            for (auto pass : skyAtmosphereParentPasses)
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

        for (auto& [_, skyAtmosphereParentPasses] : m_renderPipelineToSkyAtmosphereParentPasses)
        {
            for (auto pass : skyAtmosphereParentPasses)
            {
                pass->CreateAtmospherePass(id);
            }
        }
    }

    void SkyAtmosphereFeatureProcessor::AddRenderPasses(RPI::RenderPipeline* renderPipeline)
    {
        if (m_renderPipelineToSkyAtmosphereParentPasses.find(renderPipeline) != m_renderPipelineToSkyAtmosphereParentPasses.end())
        {
            m_renderPipelineToSkyAtmosphereParentPasses.erase(renderPipeline);
        }
        m_renderPipelineToSkyAtmosphereParentPasses[renderPipeline] = {};

        auto& skyAtmosphereParentPasses = m_renderPipelineToSkyAtmosphereParentPasses[renderPipeline];

        RPI::PassFilter passFilter = RPI::PassFilter::CreateWithTemplateName(Name("SkyAtmosphereParentTemplate"), renderPipeline);
        RPI::PassSystemInterface::Get()->ForEachPass(
            passFilter,
            [&skyAtmosphereParentPasses](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
            {
                SkyAtmosphereParentPass* parentPass = static_cast<SkyAtmosphereParentPass*>(pass);
                skyAtmosphereParentPasses.emplace_back(parentPass);
                return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
            });

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

    void SkyAtmosphereFeatureProcessor::OnRenderPipelineChanged([[maybe_unused]] RPI::RenderPipeline* pipeline,
        RPI::SceneNotification::RenderPipelineChangeType changeType)
    {
        if (changeType == RPI::SceneNotification::RenderPipelineChangeType::Added
            || changeType == RPI::SceneNotification::RenderPipelineChangeType::PassChanged)
        {
            UpdateBackgroundClearColor();
        }

        if (changeType == RPI::SceneNotification::RenderPipelineChangeType::Removed)
        {
            m_renderPipelineToSkyAtmosphereParentPasses.erase(pipeline);
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
                for (auto& [_, skyAtmosphereParentPasses] : m_renderPipelineToSkyAtmosphereParentPasses)
                {
                    for (auto pass : skyAtmosphereParentPasses)
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
        const RHI::ClearValue blackClearValue = RHI::ClearValue::CreateVector4Float(0.f, 0.f, 0.f, 0.f);
        RPI::PassFilter passFilter;
        AZStd::string slot;

        auto setClearValue = [&](RPI::Pass* pass)-> RPI::PassFilterExecutionFlow
        {
            Name slotName = Name::FromStringLiteral(slot, AZ::Interface<AZ::NameDictionary>::Get());
            if (auto binding = pass->FindAttachmentBinding(slotName))
            {
                binding->m_unifiedScopeDesc.m_loadStoreAction.m_clearValue = blackClearValue;
            }
            return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
        };

        slot = "SpecularOutput";
        passFilter= RPI::PassFilter::CreateWithTemplateName(Name("ForwardPassTemplate"), GetParentScene());
        RPI::PassSystemInterface::Get()->ForEachPass(passFilter, setClearValue);
        passFilter = RPI::PassFilter::CreateWithTemplateName(Name("ForwardMSAAPassTemplate"), GetParentScene());
        RPI::PassSystemInterface::Get()->ForEachPass(passFilter, setClearValue);

        slot = "ReflectionOutput";
        passFilter = RPI::PassFilter::CreateWithTemplateName(Name("ReflectionGlobalFullscreenPassTemplate"), GetParentScene());
        RPI::PassSystemInterface::Get()->ForEachPass(passFilter, setClearValue);
    }
}
