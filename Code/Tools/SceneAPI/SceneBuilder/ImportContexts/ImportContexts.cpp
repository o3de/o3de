/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneBuilder/ImportContexts/ImportContexts.h>
#include <SceneAPI/SceneCore/Events/ImportEventContext.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            ImportContext::ImportContext(Containers::Scene& scene,
                Containers::SceneGraph::NodeIndex currentGraphPosition,
                RenamedNodesMap& nodeNameMap)
                : m_scene(scene)
                , m_currentGraphPosition(currentGraphPosition)
                , m_nodeNameMap(nodeNameMap)
            {
            }

            ImportContext::ImportContext(Containers::Scene& scene, RenamedNodesMap& nodeNameMap)
                : m_scene(scene)
                , m_nodeNameMap(nodeNameMap)
            {
                m_currentGraphPosition = Containers::SceneGraph::NodeIndex();
            }

            NodeEncounteredContext::NodeEncounteredContext(Containers::Scene& scene,
                Containers::SceneGraph::NodeIndex currentGraphPosition,
                RenamedNodesMap& nodeNameMap)
                : ImportContext(scene, currentGraphPosition, nodeNameMap)
            {
            }

            NodeEncounteredContext::NodeEncounteredContext(
                Events::ImportEventContext& parent, Containers::SceneGraph::NodeIndex currentGraphPosition,
                RenamedNodesMap& nodeNameMap)
                : ImportContext(parent.GetScene(), currentGraphPosition, nodeNameMap)
            {
            }

            SceneDataPopulatedContextBase::SceneDataPopulatedContextBase(NodeEncounteredContext& parent,
                const AZStd::shared_ptr<DataTypes::IGraphObject>& graphData, const AZStd::string& dataName)
                : ImportContext(parent.m_scene, parent.m_currentGraphPosition, parent.m_nodeNameMap)
                , m_graphData(graphData)
                , m_dataName(dataName)
            {
            }

            SceneDataPopulatedContextBase::SceneDataPopulatedContextBase(Containers::Scene& scene,
                Containers::SceneGraph::NodeIndex currentGraphPosition,
                RenamedNodesMap& nodeNameMap,
                const AZStd::shared_ptr<DataTypes::IGraphObject>& nodeData, const AZStd::string& dataName)
                : ImportContext(scene, currentGraphPosition, nodeNameMap)
                , m_graphData(nodeData)
                , m_dataName(dataName)
            {
            }

            SceneNodeAppendedContextBase::SceneNodeAppendedContextBase(SceneDataPopulatedContextBase& parent,
                Containers::SceneGraph::NodeIndex newIndex)
                : ImportContext(parent.m_scene, newIndex, parent.m_nodeNameMap)
            {
            }

            SceneNodeAppendedContextBase::SceneNodeAppendedContextBase(Containers::Scene& scene,
                Containers::SceneGraph::NodeIndex currentGraphPosition, RenamedNodesMap& nodeNameMap)
                : ImportContext(scene, currentGraphPosition, nodeNameMap)
            {
            }

            SceneAttributeDataPopulatedContextBase::SceneAttributeDataPopulatedContextBase(SceneNodeAppendedContextBase& parent,
                const AZStd::shared_ptr<DataTypes::IGraphObject>& nodeData,
                const Containers::SceneGraph::NodeIndex attributeNodeIndex, const AZStd::string& dataName)
                : ImportContext(parent.m_scene, attributeNodeIndex, parent.m_nodeNameMap)
                , m_graphData(nodeData)
                , m_dataName(dataName)
            {
            }

            SceneAttributeNodeAppendedContextBase::SceneAttributeNodeAppendedContextBase(SceneAttributeDataPopulatedContextBase& parent, Containers::SceneGraph::NodeIndex newIndex)
                : ImportContext(parent.m_scene, newIndex, parent.m_nodeNameMap)
            {
            }

            SceneNodeAddedAttributesContextBase::SceneNodeAddedAttributesContextBase(SceneNodeAppendedContextBase& parent)
                : ImportContext(parent.m_scene, parent.m_currentGraphPosition, parent.m_nodeNameMap)
            {
            }

            SceneNodeFinalizeContextBase::SceneNodeFinalizeContextBase(SceneNodeAddedAttributesContextBase& parent)
                : ImportContext(parent.m_scene, parent.m_currentGraphPosition, parent.m_nodeNameMap)
            {
            }

            FinalizeSceneContextBase::FinalizeSceneContextBase(Containers::Scene& scene, RenamedNodesMap& nodeNameMap)
                : ImportContext(scene, nodeNameMap)
            {
            }
        } // namespace SceneAPI
    } // namespace SceneBuilder
} // namespace AZ
