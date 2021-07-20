/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
