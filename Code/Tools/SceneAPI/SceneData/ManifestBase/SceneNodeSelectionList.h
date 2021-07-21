#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/JSON/document.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
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
                SCENE_DATA_API const AZStd::string& GetSelectedNode(size_t index) const override;
                SCENE_DATA_API size_t AddSelectedNode(const AZStd::string& name) override;
                SCENE_DATA_API size_t AddSelectedNode(AZStd::string&& name) override;
                SCENE_DATA_API void RemoveSelectedNode(size_t index) override;
                SCENE_DATA_API void RemoveSelectedNode(const AZStd::string& name) override;
                SCENE_DATA_API void ClearSelectedNodes() override;

                SCENE_DATA_API size_t GetUnselectedNodeCount() const override;
                SCENE_DATA_API const AZStd::string& GetUnselectedNode(size_t index) const override;
                SCENE_DATA_API void ClearUnselectedNodes() override;

                SCENE_DATA_API AZStd::unique_ptr<DataTypes::ISceneNodeSelectionList> Copy() const override;
                SCENE_DATA_API void CopyTo(DataTypes::ISceneNodeSelectionList& other) const override;

                static void Reflect(AZ::ReflectContext* context);
                
            protected:
                AZStd::vector<AZStd::string> m_selectedNodes;
                AZStd::vector<AZStd::string> m_unselectedNodes;
            };
            
            inline SceneNodeSelectionList::~SceneNodeSelectionList() = default;
            
        } // SceneData
    } // SceneAPI
} // AZ
