/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: MIT
 */

#include "SceneColorViewFeatureProcessor.h"
#include "SceneColorViewCapturePass.h"
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Reflect/Pass/PassRequest.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace SceneColorView
{
    void SceneColorViewFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* sc = azrtti_cast<AZ::SerializeContext*>(context))
        {
            sc->Class<SceneColorViewFeatureProcessor, AZ::RPI::FeatureProcessor>()->Version(0);
        }
    }

    void SceneColorViewFeatureProcessor::Activate() {}

    void SceneColorViewFeatureProcessor::Deactivate()
    {
        for (const auto& pass : m_passes)
        {
            if (pass && pass->GetParent()) { pass->QueueForRemoval(); }
        }
        m_passes.clear();
        m_captures.clear();
        m_configuration = {};
    }

    bool SceneColorViewFeatureProcessor::SetConfiguration(AZ::EntityId owner, const Configuration& configuration)
    {
        if (m_owner.IsValid() && m_owner != owner) { return false; }
        m_owner = owner;
        m_configuration = configuration;
        return true;
    }

    void SceneColorViewFeatureProcessor::ReleaseConfiguration(AZ::EntityId owner)
    {
        if (m_owner == owner)
        {
            m_owner = AZ::EntityId{};
            m_configuration = {};
        }
    }

    void SceneColorViewFeatureProcessor::AddRenderPasses(AZ::RPI::RenderPipeline* pipeline)
    {
        // Narrow initial support: standard desktop TransparentParentTemplate.
        // Find the internal raster child instead of assuming the root's display name.
        if (!pipeline || pipeline->FindFirstPass(AZ::Name("SceneColorViewCapture"))) { return; }
        auto raster = pipeline->FindFirstPass(AZ::Name("TransparentPass"));
        if (raster && azrtti_cast<AZ::RPI::ParentPass*>(raster.get()))
        {
            raster = azrtti_cast<AZ::RPI::ParentPass*>(raster.get())->FindChildPass(AZ::Name("TransparentPass"));
        }
        if (!raster || !raster->GetParent()) { return; }
        auto* parent = raster->GetParent();
        if (!parent->FindAttachmentBinding(AZ::Name("InputOutput"))) { return; }

        AZ::RPI::PassRequest request;
        request.m_passName = AZ::Name("SceneColorViewCapture");
        request.m_templateName = AZ::Name("SceneColorViewCaptureTemplate");
        AZ::RPI::PassConnection connection;
        connection.m_localSlot = AZ::Name("Source");
        connection.m_attachmentRef.m_pass = AZ::Name("Parent");
        connection.m_attachmentRef.m_attachment = AZ::Name("InputOutput");
        request.m_connections.push_back(connection);
        auto pass = AZ::RPI::PassSystemInterface::Get()->CreatePassFromRequest(&request);
        auto* capture = pass ? azrtti_cast<SceneColorViewCapturePass*>(pass.get()) : nullptr;
        if (!capture) { return; }
        capture->SetConsumer(raster);
        if (parent->InsertChild(pass, raster->GetParentChildIndex()))
        {
            m_passes.push_back(pass);
            AZ_Printf("SceneColorView", "Inserted optional capture before %s.\n", raster->GetPathName().GetCStr());
        }
    }

    void SceneColorViewFeatureProcessor::OnEndCulling(const RenderPacket& packet)
    {
        // Scene invokes this serially after the parallel Render/culling jobs.
        // This avoids concurrent updates to the shared view SRG's resource data.
        for (auto it = m_passes.begin(); it != m_passes.end();)
        {
            // Drop orphaned passes after pipeline rebuild/removal so their
            // persistent images and retained consumer passes can be released.
            if (!(*it)->GetParent()) { it = m_passes.erase(it); }
            else { ++it; }
        }
        m_captures.clear();
        auto* images = AZ::RPI::ImageSystemInterface::Get();
        if (!images) { return; }
        for (const auto& view : packet.m_views)
        {
            auto srg = view->GetShaderResourceGroup();
            if (!srg) { continue; }
            const auto imageIndex = srg->FindShaderInputImageIndex(AZ::Name("m_sceneColorViewImage"));
            const auto validIndex = srg->FindShaderInputConstantIndex(AZ::Name("m_sceneColorViewAvailable"));
            if (!imageIndex.IsValid() || !validIndex.IsValid())
            {
                if (!m_reportedMissingLayout)
                {
                    AZ_Error("SceneColorView", false,
                        "View SRG layout is missing SceneColorView fields. Run Tools/integrate_viewsrg.py for this project and let SceneAndViewSrgs rebuild.");
                    m_reportedMissingLayout = true;
                }
                continue;
            }
            srg->SetImage(imageIndex, images->GetSystemImage(AZ::RPI::SystemImage::Black));
            srg->SetConstant(validIndex, AZ::u32(0));
            // Do not Compile here: the engine's View::UpdateSrg compiles once,
            // after the capture pass has updated these fields in FrameBegin.
        }
    }

    const SceneColorViewFeatureProcessor::CaptureRecord* SceneColorViewFeatureProcessor::FindCapture(const AZ::RPI::View* view) const
    {
        auto it = m_captures.find(view);
        return it == m_captures.end() ? nullptr : &it->second;
    }

    void SceneColorViewFeatureProcessor::PublishCapture(const AZ::RPI::View* view, CaptureRecord record)
    {
        m_captures[view] = AZStd::move(record);
    }
}
