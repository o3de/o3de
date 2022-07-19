/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorModeFeedbackFeatureProcessor.h>
#include <Pass/State/FocusedEntityParentPass.h>
#include <Pass/State/SelectedEntityParentPass.h>

#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/Utils/Utils.h>

namespace AZ
{
    namespace Render
    {
        namespace
        {
            [[maybe_unused]] const char* const Window = "EditorModeFeedback";
        }

        // Creates the material for the mask pass shader
        static Data::Instance<RPI::Material> CreateMaskMaterial()
        {
            const AZStd::string path = "shaders/editormodemask.azmaterial";
            const auto materialAsset = GetAssetFromPath<RPI::MaterialAsset>(path, Data::AssetLoadBehavior::PreLoad, true);
            const auto maskMaterial = RPI::Material::FindOrCreate(materialAsset);
            return maskMaterial;
        }

        void EditorModeFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<EditorModeFeatureProcessor, RPI::FeatureProcessor>()->Version(0);
            }
        }

        void EditorModeFeatureProcessor::Activate()
        {
            EnableSceneNotification();

            if (!m_maskMaterial)
            {
                m_maskMaterial = CreateMaskMaterial();
            }

            EditorStateParentPassList editorStatePasses;
            editorStatePasses.push_back(AZStd::make_unique<FocusedEntityParentPass>());
            editorStatePasses.push_back(AZStd::make_unique<SelectedEntityParentPass>());
            m_editorStatePassSystem = AZStd::make_unique<EditorStatePassSystem>(AZStd::move(editorStatePasses));
        }

        void EditorModeFeatureProcessor::Deactivate()
        {
            m_editorStatePassSystem.reset();
            DisableSceneNotification();
        }

        void EditorModeFeatureProcessor::OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline)
        {
            InitPasses(pipeline.get());
            
        }

        void EditorModeFeatureProcessor::OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline)
        {
            InitPasses(renderPipeline);
        }

        void EditorModeFeatureProcessor::InitPasses(RPI::RenderPipeline* renderPipeline)
        {
            m_editorStatePassSystem->InitPasses(renderPipeline);
        }

        void EditorModeFeatureProcessor::ApplyRenderPipelineChange(RPI::RenderPipeline* renderPipeline)
        {
            m_editorStatePassSystem->AddPassesToRenderPipeline(renderPipeline);

            for (const auto& mask : m_editorStatePassSystem->GetMasks())
            {
                m_maskRenderers.emplace(
                    AZStd::piecewise_construct, AZStd::forward_as_tuple(mask), AZStd::forward_as_tuple(mask, m_maskMaterial));
            }
        }

        void EditorModeFeatureProcessor::Render(const RenderPacket&)
        {
            const auto entityMaskMap = m_editorStatePassSystem->GetEntitiesForEditorStatePasses();
            for (const auto& [mask, entities] : entityMaskMap)
            {
                if(auto it = m_maskRenderers.find(mask);
                    it != m_maskRenderers.end())
                {
                    it->second.RenderMaskEntities(entities);
                } 
            }
        }

        void EditorModeFeatureProcessor::Simulate([[maybe_unused]] const SimulatePacket& packet)
        {
            m_editorStatePassSystem->Update();
        }
    } // namespace Render
} // namespace AZ
