/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorModeFeedbackFeatureProcessor.h>
#include <Pass/State/FocusedEntityState.h>
#include <Pass/State/SelectedEntityState.h>

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
                serializeContext
                    ->Class<EditorModeFeatureProcessor, RPI::FeatureProcessor>()
                    ->Version(1);
            }
        }

        void EditorModeFeatureProcessor::Activate()
        {
            EnableSceneNotification();

            if (!m_maskMaterial)
            {
                m_maskMaterial = CreateMaskMaterial();
            }

            EditorStateList editorStates;
            editorStates.push_back(AZStd::make_unique<FocusedEntityState>());
            editorStates.push_back(AZStd::make_unique<SelectedEntityState>());
            m_editorStatePassSystem = AZStd::make_unique<EditorStatePassSystem>(AZStd::move(editorStates));
        }

        void EditorModeFeatureProcessor::Deactivate()
        {
            m_editorStatePassSystem.reset();
            DisableSceneNotification();
        }

        void EditorModeFeatureProcessor::OnRenderPipelineRemoved(RPI::RenderPipeline* pipeline)
        {
            if (!m_editorStatePassSystem)
            {
                return;
            }

            m_editorStatePassSystem->RemoveStatePassesForPipeline(pipeline);
        }

        void EditorModeFeatureProcessor::OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline)
        {
            if (!m_editorStatePassSystem)
            {
                return;
            }

            m_editorStatePassSystem->ConfigureStatePassesForPipeline(pipeline.get());
        }

        void EditorModeFeatureProcessor::OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline)
        {
            if (!m_editorStatePassSystem)
            {
                return;
            }

            m_editorStatePassSystem->ConfigureStatePassesForPipeline(renderPipeline);
        }

        void EditorModeFeatureProcessor::ApplyRenderPipelineChange(RPI::RenderPipeline* renderPipeline)
        {
            if (!m_editorStatePassSystem)
            {
                return;
            }

            m_editorStatePassSystem->AddPassesToRenderPipeline(renderPipeline);

            if(!m_maskRenderers.empty())
            {
                return;
            }

            for (const auto& mask : m_editorStatePassSystem->GetMasks())
            {
                // Emplaces the mask key and mask renderer value in place for the mask renderers map
                m_maskRenderers.emplace(
                    AZStd::piecewise_construct, AZStd::forward_as_tuple(mask), AZStd::forward_as_tuple(mask, m_maskMaterial));
            }
        }

        void EditorModeFeatureProcessor::Render([[maybe_unused]] const RenderPacket& packet)
        {
            if (!m_editorStatePassSystem)
            {
                return;
            }

            const auto entityMaskMap = m_editorStatePassSystem->GetEntitiesForEditorStates();
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
            if (!m_editorStatePassSystem)
            {
                return;
            }

            m_editorStatePassSystem->Update();
        }
    } // namespace Render
} // namespace AZ
