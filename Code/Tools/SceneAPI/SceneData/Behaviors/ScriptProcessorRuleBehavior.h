/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>

namespace AzToolsFramework
{
    class EditorPythonEventsInterface;
}

namespace AZ::SceneAPI::Events
{
    class PreExportEventContext;
}

namespace AZ::SceneAPI::Containers
{
    class Scene;
}

namespace AZ::SceneAPI::Behaviors
{
    class SCENE_DATA_CLASS ScriptProcessorRuleBehavior
        : public SceneCore::BehaviorComponent
        , public Events::AssetImportRequestBus::Handler
    {
    public:
        
        AZ_COMPONENT(ScriptProcessorRuleBehavior, "{24054E73-1B92-43B0-AC13-174B2F0E3F66}", SceneCore::BehaviorComponent);

        ~ScriptProcessorRuleBehavior() override = default;

        SCENE_DATA_API void Activate() override;
        SCENE_DATA_API void Deactivate() override;
        SCENE_DATA_API static void Reflect(ReflectContext* context);
        
        // AssetImportRequestBus::Handler
        SCENE_DATA_API Events::ProcessingResult UpdateManifest(
            Containers::Scene& scene,
            ManifestAction action,
            RequestingApplication requester) override;

        SCENE_DATA_API void GetManifestDependencyPaths(AZStd::vector<AZStd::string>& paths) override;

        SCENE_DATA_API void GetPolicyName(AZStd::string& result) const override
        {
            result = "ScriptProcessorRuleBehavior";
        }
    protected:
        bool LoadPython(const AZ::SceneAPI::Containers::Scene& scene, AZStd::string& scriptPath, Events::ProcessingResult& fallbackResult);
        void UnloadPython();
        bool DoPrepareForExport(Events::PreExportEventContext& context);

    private:
        AzToolsFramework::EditorPythonEventsInterface* m_editorPythonEventsInterface = nullptr;

        struct EventHandler;
        AZStd::shared_ptr<EventHandler> m_eventHandler;
    };
} // namespace AZ::SceneAPI::Behaviors
