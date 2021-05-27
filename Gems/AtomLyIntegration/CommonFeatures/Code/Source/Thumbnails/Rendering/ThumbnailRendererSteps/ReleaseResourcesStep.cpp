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
                auto data = m_context->GetData();
                
                data->m_defaultMaterialAsset.Release();
                data->m_defaultModelAsset.Release();
                data->m_materialAsset.Release();
                data->m_modelAsset.Release();
                data->m_lightingPresetAsset.Release();

                if (data->m_modelEntity)
                {
                    AzFramework::EntityContextRequestBus::Event(data->m_entityContext->GetContextId(),
                        &AzFramework::EntityContextRequestBus::Events::DestroyEntity, data->m_modelEntity);
                    data->m_modelEntity = nullptr;
                }

                data->m_scene->Deactivate();
                data->m_scene->RemoveRenderPipeline(data->m_renderPipeline->GetId());
                RPI::RPISystemInterface::Get()->UnregisterScene(data->m_scene);
                data->m_frameworkScene->UnsetSubsystem(data->m_scene);
                data->m_frameworkScene->UnsetSubsystem(data->m_entityContext.get());
                data->m_scene = nullptr;
                data->m_frameworkScene = nullptr;
                data->m_renderPipeline = nullptr;
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
