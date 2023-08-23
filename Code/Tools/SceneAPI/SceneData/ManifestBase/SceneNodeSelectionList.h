#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/JSON/document.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>


namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
        namespace DataTypes
        {
            class IManifestObject;
        }
        namespace SceneData
        {
            class SceneNodeSelectionList
                : public DataTypes::ISceneNodeSelectionList
            {
            public:
                AZ_RTTI(SceneNodeSelectionList, "{D0CE66CE-1BAD-42F5-86ED-3923573B3A02}", DataTypes::ISceneNodeSelectionList);
                ~SceneNodeSelectionList() override;

                SCENE_DATA_API size_t GetSelectedNodeCount() const override;
                SCENE_DATA_API void AddSelectedNode(const AZStd::string& name) override;
                SCENE_DATA_API void AddSelectedNode(AZStd::string&& name) override;
                SCENE_DATA_API void RemoveSelectedNode(const AZStd::string& name) override;
                SCENE_DATA_API void ClearSelectedNodes() override;
                SCENE_DATA_API bool IsSelectedNode(const AZStd::string& name) const override;
                SCENE_DATA_API void EnumerateSelectedNodes(const EnumerateNodesCallback& callback) const override;

                SCENE_DATA_API void ClearUnselectedNodes() override;
                SCENE_DATA_API void EnumerateUnselectedNodes(const EnumerateNodesCallback& callback) const override;

                SCENE_DATA_API AZStd::unique_ptr<DataTypes::ISceneNodeSelectionList> Copy() const override;
                SCENE_DATA_API void CopyTo(DataTypes::ISceneNodeSelectionList& other) const override;

                static void Reflect(AZ::ReflectContext* context);
                
            protected:
                AZStd::unordered_set<AZStd::string> m_selectedNodes;
                AZStd::unordered_set<AZStd::string> m_unselectedNodes;
            };
            
            inline SceneNodeSelectionList::~SceneNodeSelectionList() = default;
            
        } // SceneData
    } // SceneAPI
} // AZ
