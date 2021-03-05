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

#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            // Scenes are used to store the scene's graph the hierarchy and the manifest for meta data as well
            //      as a history of the files used to construct both.
            class SCENE_CORE_API Scene
            {
            public:
                AZ_TYPE_INFO(Scene, "{1F2E6142-B0D8-42C6-A6E5-CD726DAA9EF0}");
                
                explicit Scene(const AZStd::string& name);
                explicit Scene(AZStd::string&& name);

                void SetSource(const AZStd::string& filename, const Uuid& guid);
                void SetSource(AZStd::string&& filename, const Uuid& guid);
                const AZStd::string& GetSourceFilename() const;
                const Uuid& GetSourceGuid() const;

                void SetManifestFilename(const AZStd::string& name);
                void SetManifestFilename(AZStd::string&& name);
                const AZStd::string& GetManifestFilename() const;

                SceneGraph& GetGraph();
                const SceneGraph& GetGraph() const;

                SceneManifest& GetManifest();
                const SceneManifest& GetManifest() const;

                const AZStd::string& GetName() const;

                enum class SceneOrientation {YUp, ZUp, XUp, NegYUp, NegZUp, NegXUp};

                void SetOriginalSceneOrientation(SceneOrientation orientation);
                SceneOrientation GetOriginalSceneOrientation() const;

                static void Reflect(ReflectContext* context);

            private:
            // Disabling export warnings for private methods, clients wont have access to them
            AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
                AZStd::string m_name;
                AZStd::string m_manifestFilename;
                AZStd::string m_sourceFilename;
                Uuid m_sourceGuid;
                SceneGraph m_graph;
                SceneManifest m_manifest;
                SceneOrientation m_originalOrientation = SceneOrientation::YUp;
            AZ_POP_DISABLE_OVERRIDE_WARNING
            };
        } // Containers
    } // SceneAPI
} // AZ
