/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CryCommon/IConsole.h>
#include <SkinnedMesh/SkinnedMeshDebugDisplay.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshStatsBus.h>
#include <Atom/RPI.Public/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include <ISystem.h>

namespace AZ
{
    namespace Render
    {
        SkinnedMeshDebugDisplay::SkinnedMeshDebugDisplay()
        {
            CrySystemEventBus::Handler::BusConnect();
            AZ::Render::Bootstrap::NotificationBus::Handler::BusConnect();
        }

        SkinnedMeshDebugDisplay::~SkinnedMeshDebugDisplay()
        {
            AZ::Render::Bootstrap::NotificationBus::Handler::BusDisconnect();
            CrySystemEventBus::Handler::BusDisconnect();
            AzFramework::ViewportDebugDisplayEventBus::Handler::BusDisconnect();
        }

        void SkinnedMeshDebugDisplay::OnCrySystemInitialized(ISystem& system, [[maybe_unused]] const SSystemInitParams& systemInitParams)
        {
            // Once CrySystem is initialized, we can register cvars
            if (system.GetGlobalEnvironment() && system.GetGlobalEnvironment()->pConsole)
            {
                system.GetGlobalEnvironment()->pConsole->Register("r_skinnedMeshDisplaySceneStats", &r_skinnedMeshDisplaySceneStats, 0, VF_NULL,
                    CVARHELP("Enable debug display of skinned mesh stats\n"
                        "  1 = 'Skinned Mesh Scene Stats': This represents all lods of all the skinned meshes in the scene, not just what's in view."
                        "    Effectively, this is everything that is created and uploaded to the GPU, though only the visible subset of meshes/lods will be skinned.\n"));
            }
            else
            {
                AZ_Assert(false, "Attempting to register r_skinnedMeshDisplaySceneStats before the cvar system has been initialized");
            }
        }

        void SkinnedMeshDebugDisplay::OnCrySystemShutdown(ISystem& system)
        {
            if (system.GetGlobalEnvironment() && system.GetGlobalEnvironment()->pConsole)
            {
                system.GetGlobalEnvironment()->pConsole->UnregisterVariable("r_skinnedMeshDisplaySceneStats");
            }
            else
            {
                AZ_Assert(false, "Attempting to unregister r_skinnedMeshDisplaySceneStats after the cvar system has been shut down");
            }
        }

        void SkinnedMeshDebugDisplay::OnCryEditorInitialized()
        {
            // Once the editor is initialized, listen to ViewportDebugDisplayEventBus
            AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(editorEntityContextId, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorEntityContextId);

            AzFramework::ViewportDebugDisplayEventBus::Handler::BusConnect(editorEntityContextId);
        }
        void SkinnedMeshDebugDisplay::DisplayViewport2d([[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
        {
            if (r_skinnedMeshDisplaySceneStats == 1)
            {
                // Get scene id when DisplayViewport2d is called. The RPI Scene may not be ready when OnCryEditorInitialized was called
                SkinnedMeshSceneStats stats{};
                SkinnedMeshStatsRequestBus::EventResult(stats, m_sceneId, &SkinnedMeshStatsRequestBus::Events::GetSceneStats);

                float x = 0;
                float y = 16.0f; // By default, the editor's debug display text shows the number of entities in the top left corner. Offset so the skinned mesh stats don't overlap
                float size = 1.25f;
                bool center = false;

                AZStd::string debugString = AZStd::string::format(
                    "Skinned Mesh Scene Stats:\n"
                    "  SkinnedMeshRenderProxy count: %zu\n"
                    "  DispatchItem count: %zu\n"
                    "  Bone count: %zu\n"
                    "  Vertex count: %zu\n",
                    stats.skinnedMeshRenderProxyCount, stats.dispatchItemCount, stats.boneCount, stats.vertexCount
                );

                debugDisplay.Draw2dTextLabel(x, y, size, debugString.c_str(), center);
            }
        }

        void SkinnedMeshDebugDisplay::OnBootstrapSceneReady(AZ::RPI::Scene* bootstrapScene)
        {
            m_sceneId = bootstrapScene->GetId();
        }

    }// namespace Render
}// namespace AZ
