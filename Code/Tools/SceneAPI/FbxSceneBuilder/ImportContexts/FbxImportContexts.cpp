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

#include <SceneAPI/FbxSceneBuilder/ImportContexts/FbxImportContexts.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>
#include <SceneAPI/SceneCore/Events/ImportEventContext.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            FbxImportContext::FbxImportContext(const FbxSDKWrapper::FbxSceneWrapper& sourceScene, 
                const FbxSceneSystem& sourceSceneSystem, FbxSDKWrapper::FbxNodeWrapper& sourceNode)
                : m_sourceScene(sourceScene)
                , m_sourceSceneSystem(sourceSceneSystem)
                , m_sourceNode(sourceNode)
            {
            }

            FbxNodeEncounteredContext::FbxNodeEncounteredContext(Containers::Scene& scene,
                Containers::SceneGraph::NodeIndex currentGraphPosition, const FbxSDKWrapper::FbxSceneWrapper& sourceScene,
                const FbxSceneSystem& sourceSceneSystem, RenamedNodesMap& nodeNameMap, FbxSDKWrapper::FbxNodeWrapper& sourceNode)
                : FbxImportContext(sourceScene, sourceSceneSystem, sourceNode)
                , NodeEncounteredContext(scene, currentGraphPosition, nodeNameMap)
            {
            }

            FbxNodeEncounteredContext::FbxNodeEncounteredContext(
                Events::ImportEventContext& parent, Containers::SceneGraph::NodeIndex currentGraphPosition,
                const FbxSDKWrapper::FbxSceneWrapper& sourceScene, const FbxSceneSystem& sourceSceneSystem,
                RenamedNodesMap& nodeNameMap, FbxSDKWrapper::FbxNodeWrapper& sourceNode)
                : FbxImportContext(sourceScene, sourceSceneSystem, sourceNode)
                , NodeEncounteredContext(parent.GetScene(), currentGraphPosition, nodeNameMap)
            {
            }

            SceneDataPopulatedContext::SceneDataPopulatedContext(FbxNodeEncounteredContext& parent,
                const AZStd::shared_ptr<DataTypes::IGraphObject>& graphData, const AZStd::string& dataName)
                : FbxImportContext(parent.m_sourceScene, parent.m_sourceSceneSystem, parent.m_sourceNode)
                , SceneDataPopulatedContextBase(parent, graphData, dataName)
            {
            }

            SceneDataPopulatedContext::SceneDataPopulatedContext(Containers::Scene& scene,
                Containers::SceneGraph::NodeIndex currentGraphPosition, const FbxSDKWrapper::FbxSceneWrapper& sourceScene,
                const FbxSceneSystem& sourceSceneSystem, RenamedNodesMap& nodeNameMap, FbxSDKWrapper::FbxNodeWrapper& sourceNode,
                const AZStd::shared_ptr<DataTypes::IGraphObject>& nodeData, const AZStd::string& dataName)
                : FbxImportContext(sourceScene, sourceSceneSystem, sourceNode)
                , SceneDataPopulatedContextBase(scene, currentGraphPosition, nodeNameMap, nodeData, dataName)
            {
            }

            SceneNodeAppendedContext::SceneNodeAppendedContext(SceneDataPopulatedContext& parent,
                Containers::SceneGraph::NodeIndex newIndex)
                : FbxImportContext(parent.m_sourceScene, parent.m_sourceSceneSystem, parent.m_sourceNode)
                , SceneNodeAppendedContextBase(parent.m_scene, newIndex, parent.m_nodeNameMap)
            {
            }

            SceneNodeAppendedContext::SceneNodeAppendedContext(Containers::Scene& scene,
                Containers::SceneGraph::NodeIndex currentGraphPosition, const FbxSDKWrapper::FbxSceneWrapper& sourceScene,
                const FbxSceneSystem& sourceSceneSystem, RenamedNodesMap& nodeNameMap, FbxSDKWrapper::FbxNodeWrapper& sourceNode)
                : FbxImportContext(sourceScene, sourceSceneSystem, sourceNode)
                , SceneNodeAppendedContextBase(scene, currentGraphPosition, nodeNameMap)
            {
            }

            SceneAttributeDataPopulatedContext::SceneAttributeDataPopulatedContext(SceneNodeAppendedContext& parent,
                const AZStd::shared_ptr<DataTypes::IGraphObject>& nodeData,
                const Containers::SceneGraph::NodeIndex attributeNodeIndex, const AZStd::string& dataName)
                : FbxImportContext(parent.m_sourceScene, parent.m_sourceSceneSystem, parent.m_sourceNode)
                , SceneAttributeDataPopulatedContextBase(parent, nodeData, attributeNodeIndex, dataName)
            {
            }

            SceneAttributeNodeAppendedContext::SceneAttributeNodeAppendedContext(
                SceneAttributeDataPopulatedContext& parent, Containers::SceneGraph::NodeIndex newIndex)
                : FbxImportContext(parent.m_sourceScene, parent.m_sourceSceneSystem, parent.m_sourceNode)
                , SceneAttributeNodeAppendedContextBase(parent, newIndex)
            {
            }

            SceneNodeAddedAttributesContext::SceneNodeAddedAttributesContext(SceneNodeAppendedContext& parent)
                : FbxImportContext(parent.m_sourceScene, parent.m_sourceSceneSystem, parent.m_sourceNode)
                , SceneNodeAddedAttributesContextBase(parent)
            {
            }

            SceneNodeFinalizeContext::SceneNodeFinalizeContext(SceneNodeAddedAttributesContext& parent)
                : FbxImportContext(parent.m_sourceScene, parent.m_sourceSceneSystem, parent.m_sourceNode)
                , SceneNodeFinalizeContextBase(parent)
            {
            }

            FinalizeSceneContext::FinalizeSceneContext(Containers::Scene& scene, const FbxSDKWrapper::FbxSceneWrapper& sourceScene,
                const FbxSceneSystem& sourceSceneSystem, RenamedNodesMap& nodeNameMap)
                : FinalizeSceneContextBase(scene, nodeNameMap)
                , m_sourceScene(sourceScene)
                , m_sourceSceneSystem(sourceSceneSystem)
            {
            }
        } // namespace SceneAPI
    } // namespace FbxSceneBuilder
} // namespace AZ