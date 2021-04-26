/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>
#include <Thumbnails/Rendering/ThumbnailRendererContext.h>
#include <Thumbnails/Rendering/ThumbnailRendererData.h>
#include <Thumbnails/Rendering/ThumbnailRendererSteps/ReleaseResourcesStep.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            ReleaseResourcesStep::ReleaseResourcesStep(ThumbnailRendererContext* context)
                : ThumbnailRendererStep(context)
            {
            }

            void ReleaseResourcesStep::Start()
            {
                m_context->GetData()->m_defaultMaterialAsset.Release();
                m_context->GetData()->m_defaultModelAsset.Release();
                m_context->GetData()->m_materialAsset.Release();
                m_context->GetData()->m_modelAsset.Release();

                if (m_context->GetData()->m_modelEntity)
                {
                    AzFramework::EntityContextRequestBus::Event(m_context->GetData()->m_entityContext->GetContextId(),
                        &AzFramework::EntityContextRequestBus::Events::DestroyEntity, m_context->GetData()->m_modelEntity);
                    m_context->GetData()->m_modelEntity = nullptr;
                }

                m_context->GetData()->m_frameworkScene->UnsetSubsystem<RPI::Scene>();

                m_context->GetData()->m_scene->Deactivate();
                m_context->GetData()->m_scene->RemoveRenderPipeline(m_context->GetData()->m_renderPipeline->GetId());
                RPI::RPISystemInterface::Get()->UnregisterScene(m_context->GetData()->m_scene);

                auto sceneSystem = AzFramework::SceneSystemInterface::Get();
                AZ_Assert(sceneSystem, "Thumbnail system failed to get scene system implementation.");
                [[maybe_unused]] bool sceneRemovedSuccessfully = sceneSystem->RemoveScene(m_context->GetData()->m_sceneName);
                AZ_Assert(
                    sceneRemovedSuccessfully, "Thumbnail system was unable to remove scene '%' from the scene system.",
                    m_context->GetData()->m_sceneName.c_str());
                m_context->GetData()->m_scene = nullptr;
                m_context->GetData()->m_renderPipeline = nullptr;
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
