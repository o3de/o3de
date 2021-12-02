/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            Scene::Scene(const AZStd::string& name)
                : m_name(name)
            {
            }

            Scene::Scene(AZStd::string&& name)
                : m_name(AZStd::move(name))
            {
            }

            void Scene::SetSource(const AZStd::string& filename, const Uuid& guid)
            {
                m_sourceFilename = filename;
                m_sourceGuid = guid;
            }

            void Scene::SetSource(AZStd::string&& filename, const Uuid& guid)
            {
                m_sourceFilename = AZStd::move(filename);
                m_sourceGuid = guid;
            }

            const AZStd::string& Scene::GetSourceFilename() const
            {
                return m_sourceFilename;
            }

            const Uuid& Scene::GetSourceGuid() const
            {
                return m_sourceGuid;
            }

            void Scene::SetWatchFolder(const AZStd::string& watchFolder)
            {
                m_watchFolder = watchFolder;
            }

            const AZStd::string& Scene::GetWatchFolder() const
            {
                return m_watchFolder;
            }

            void Scene::SetManifestFilename(const AZStd::string& name)
            {
                m_manifestFilename = name;
            }

            void Scene::SetManifestFilename(AZStd::string&& name)
            {
                m_manifestFilename = AZStd::move(name);
            }

            const AZStd::string& Scene::GetManifestFilename() const
            {
                return m_manifestFilename;
            }

            SceneGraph& Scene::GetGraph()
            {
                return m_graph;
            }

            const SceneGraph& Scene::GetGraph() const
            {
                return m_graph;
            }

            SceneManifest& Scene::GetManifest()
            {
                return m_manifest;
            }

            const SceneManifest& Scene::GetManifest() const
            {
                return m_manifest;
            }

            const AZStd::string& Scene::GetName() const
            {
                return m_name;
            }

            void Scene::SetOriginalSceneOrientation(SceneOrientation orientation)
            {
                m_originalOrientation = orientation;
            }

            Scene::SceneOrientation Scene::GetOriginalSceneOrientation() const
            {
                return m_originalOrientation;
            }

            void Scene::Reflect(ReflectContext* context)
            {
                AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
                if (behaviorContext)
                {
                    behaviorContext->Class<Scene>()
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene")
                        ->Property("name", BehaviorValueGetter(&Scene::m_name), nullptr)
                        ->Property("manifestFilename", BehaviorValueGetter(&Scene::m_manifestFilename), nullptr)
                        ->Property("sourceFilename", BehaviorValueGetter(&Scene::m_sourceFilename), nullptr)
                        ->Property("sourceGuid", BehaviorValueGetter(&Scene::m_sourceGuid), nullptr)
                        ->Property("graph", BehaviorValueGetter(&Scene::m_graph), nullptr)
                        ->Property("manifest", BehaviorValueGetter(&Scene::m_manifest), nullptr)
                        ->Property("watchFolder", BehaviorValueGetter(&Scene::m_watchFolder), nullptr)
                        ->Constant("SceneOrientation_YUp", BehaviorConstant(SceneOrientation::YUp))
                        ->Constant("SceneOrientation_ZUp", BehaviorConstant(SceneOrientation::ZUp))
                        ->Constant("SceneOrientation_XUp", BehaviorConstant(SceneOrientation::XUp))
                        ->Constant("SceneOrientation_NegXUp", BehaviorConstant(SceneOrientation::NegXUp))
                        ->Constant("SceneOrientation_NegYUp", BehaviorConstant(SceneOrientation::NegYUp))
                        ->Constant("SceneOrientation_NegZUp", BehaviorConstant(SceneOrientation::NegZUp))
                        ->Method("GetOriginalSceneOrientation", [](Scene* self) -> int
                         {
                            return aznumeric_cast<int>(self->GetOriginalSceneOrientation());
                         })
                        ;
                }
            }

        } // Containers
    } // SceneAPI
} // AZ
