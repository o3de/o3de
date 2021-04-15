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

#pragma once

#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>

namespace AzToolsFramework
{
    class EditorPythonEventsInterface;
}

namespace AZ
{
    namespace SceneAPI
    {
        namespace Behaviors
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
                static void Reflect(ReflectContext* context);

                // AssetImportRequestBus::Handler
                SCENE_DATA_API Events::ProcessingResult UpdateManifest(
                    Containers::Scene& scene,
                    ManifestAction action,
                    RequestingApplication requester) override;

            private:
                AzToolsFramework::EditorPythonEventsInterface* m_editorPythonEventsInterface = nullptr;
            };
        } // namespace SceneData
    } // namespace SceneAPI
} // namespace AZ
