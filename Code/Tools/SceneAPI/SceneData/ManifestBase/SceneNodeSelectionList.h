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
            class SCENE_DATA_API SceneNodeSelectionList
                : public DataTypes::ISceneNodeSelectionList
            {
            public:
                AZ_RTTI(SceneNodeSelectionList, "{D0CE66CE-1BAD-42F5-86ED-3923573B3A02}", DataTypes::ISceneNodeSelectionList);
                ~SceneNodeSelectionList() override;

                size_t GetSelectedNodeCount() const override;
                void AddSelectedNode(const AZStd::string& name) override;
                void AddSelectedNode(AZStd::string&& name) override;
                void RemoveSelectedNode(const AZStd::string& name) override;
                void ClearSelectedNodes() override;
                bool IsSelectedNode(const AZStd::string& name) const override;
                void EnumerateSelectedNodes(const EnumerateNodesCallback& callback) const override;

                void ClearUnselectedNodes() override;
                void EnumerateUnselectedNodes(const EnumerateNodesCallback& callback) const override;

                AZStd::unique_ptr<DataTypes::ISceneNodeSelectionList> Copy() const override;
                void CopyTo(DataTypes::ISceneNodeSelectionList& other) const override;

                static void Reflect(AZ::ReflectContext* context);
                
            protected:
                AZStd::unordered_set<AZStd::string> m_selectedNodes;
                AZStd::unordered_set<AZStd::string> m_unselectedNodes;
            };
            
            inline SceneNodeSelectionList::~SceneNodeSelectionList() = default;
            
        } // SceneData
    } // SceneAPI
} // AZ
