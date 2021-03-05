#pragma once

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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }

        namespace Events
        {
            // Signals an import of the scene graph is about to happen.
            class PreImportEventContext
                : public ICallContext
            {
            public:
                AZ_RTTI(PreImportEventContext, "{89BA9931-E6B5-4096-B5AE-80E80A8B4DB2}", ICallContext);
                ~PreImportEventContext() override = default;
                SCENE_CORE_API PreImportEventContext(const char* inputDirectory);
                SCENE_CORE_API PreImportEventContext(const AZStd::string& inputDirectory);
                SCENE_CORE_API PreImportEventContext(AZStd::string&& inputDirectory);
                SCENE_CORE_API const AZStd::string& GetInputDirectory() const;

            private:
                const AZStd::string m_inputDirectory;
            };

            // Signals that the scene is ready to import the scene graph from source data
            class ImportEventContext
                : public ICallContext
            {
            public:
                AZ_RTTI(ImportEventContext, "{4E0C75C2-564F-4BDF-BFAA-B7E4683B24B9}", ICallContext);
                ~ImportEventContext() override = default;

                SCENE_CORE_API ImportEventContext(const char* inputDirectory, Containers::Scene& scene);
                SCENE_CORE_API ImportEventContext(const AZStd::string& inputDirectory, Containers::Scene& scene);
                SCENE_CORE_API ImportEventContext(AZStd::string&& inputDirectory, Containers::Scene& scene);

                SCENE_CORE_API const AZStd::string& GetInputDirectory() const;
                SCENE_CORE_API Containers::Scene& GetScene();

            private:
                AZStd::string m_inputDirectory;
                Containers::Scene& m_scene;
            };

            // Signals that an import has completed and the data should be ready to use (if there were no errors)
            class PostImportEventContext
                : public ICallContext
            {
            public:
                AZ_RTTI(PostImportEventContext, "{683D2E3E-0040-4E78-90BF-76FAFFD50767}", ICallContext);
                ~PostImportEventContext() override = default;

                SCENE_CORE_API PostImportEventContext(const Containers::Scene& scene);

                SCENE_CORE_API const Containers::Scene& GetScene() const;

            private:
                const Containers::Scene& m_scene;
            };
        } // Events
    } // SceneAPI
} // AZ