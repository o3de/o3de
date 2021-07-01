/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>


namespace AZ
{    
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }

        namespace DataTypes
        {
            class IGraphObject;
        }

        namespace Events
        {
            class ImportEventContext;
        }

        namespace SceneBuilder
        {
            class RenamedNodesMap;

            //  ImportContext
            //  Base structure containing common data needed for all import contexts
            struct ImportContext
                : public Events::ICallContext
            {
                AZ_RTTI(ImportContext, "{68E546D5-9B79-4293-AD37-4A4BA688892F}", Events::ICallContext);

                ImportContext(Containers::Scene& scene, Containers::SceneGraph::NodeIndex currentGraphPosition, 
                    RenamedNodesMap& nodeNameMap);
                ImportContext(Containers::Scene& scene, RenamedNodesMap& nodeNameMap);

                Containers::Scene& m_scene;
                Containers::SceneGraph::NodeIndex m_currentGraphPosition;
                RenamedNodesMap& m_nodeNameMap; // Map of the nodes that have received a new name.
            };

            //  NodeEncounteredContext
            //  Context pushed to indicate that a new Node has been found and any
            //  importers that have means to process the contained data should do so
            //  Member Variables:
            //      m_createdData - out container that importers must add their created data
            //          to.
            struct NodeEncounteredContext
                : public ImportContext
            {
                AZ_RTTI(NodeEncounteredContext, "{40C31D76-7101-4ACD-8849-0D6D0AF62855}", ImportContext);

                NodeEncounteredContext(Containers::Scene& scene,
                    Containers::SceneGraph::NodeIndex currentGraphPosition,
                    RenamedNodesMap& nodeNameMap);

                NodeEncounteredContext(Events::ImportEventContext& parent,
                    Containers::SceneGraph::NodeIndex currentGraphPosition,
                    RenamedNodesMap& nodeNameMap);

                AZStd::vector<AZStd::shared_ptr<DataTypes::IGraphObject>> m_createdData;
            };

            //  SceneDataPopulatedContextBase
            //  Context pushed to indicate that a piece of scene data has been fully 
            //  processed and any importers that wish to place it within the scene graph
            //  may now do so.
            //  Member Variables:
            //      m_graphData - the piece of data that should be inserted in the graph
            //      m_dataName - the name that should be used as the basis for the scene node
            //          name
            struct SceneDataPopulatedContextBase
                : public ImportContext
            {
                AZ_RTTI(SceneDataPopulatedContextBase, "{5F4CE8D2-EEAC-49F7-8065-0B6372162D6F}", ImportContext);

                SceneDataPopulatedContextBase(NodeEncounteredContext& parent,
                    const AZStd::shared_ptr<DataTypes::IGraphObject>& nodeData,
                    const AZStd::string& dataName);

                SceneDataPopulatedContextBase(Containers::Scene& scene,
                    Containers::SceneGraph::NodeIndex currentGraphPosition,
                    RenamedNodesMap& nodeNameMap,
                    const AZStd::shared_ptr<DataTypes::IGraphObject>& nodeData, const AZStd::string& dataName);

                const AZStd::shared_ptr<DataTypes::IGraphObject>& m_graphData;
                const AZStd::string m_dataName;
            };

            //  SceneNodeAppendedContextBase
            //  Context pushed to indicate that data has been added to the scene graph. 
            //  Generally created due to the insertion of a node during SceneDataPopulatedContextBase
            //  processing.
            struct SceneNodeAppendedContextBase
                : public ImportContext
            {
                AZ_RTTI(SceneNodeAppendedContextBase, "{0A69FB6C-2B1B-46E7-AEC3-C4B8ABBFDD69}", ImportContext);

                SceneNodeAppendedContextBase(SceneDataPopulatedContextBase& parent, Containers::SceneGraph::NodeIndex newIndex);
                SceneNodeAppendedContextBase(Containers::Scene& scene,
                    Containers::SceneGraph::NodeIndex currentGraphPosition, RenamedNodesMap& nodeNameMap);
            };

            // SceneAttributeDataPopulatedContextBase
            // Context pushed to indicate that attribute data has been found and processed
            struct SceneAttributeDataPopulatedContextBase
                : public ImportContext
            {
                AZ_RTTI(SceneAttributeDataPopulatedContextBase, "{DA133E14-0770-435B-9A4E-38679367F56C}", ImportContext);

                SceneAttributeDataPopulatedContextBase(SceneNodeAppendedContextBase& parent,
                    const AZStd::shared_ptr<DataTypes::IGraphObject>& nodeData,
                    const Containers::SceneGraph::NodeIndex attributeNodeIndex, const AZStd::string& dataName);

                const AZStd::shared_ptr<DataTypes::IGraphObject>& m_graphData;
                const AZStd::string m_dataName;
            };

            // SceneAttributeNodeAppendedContextBase
            // Context pushed to indicate that an attribute node has been added to the scene graph
            struct SceneAttributeNodeAppendedContextBase
                : public ImportContext
            {
                AZ_RTTI(SceneAttributeNodeAppendedContextBase, "{8A382A1E-CFE7-47D2-BA5B-CFDF1FB9F03D}", ImportContext);

                SceneAttributeNodeAppendedContextBase(SceneAttributeDataPopulatedContextBase& parent,
                    Containers::SceneGraph::NodeIndex newIndex);
            };

            //  SceneNodeAddedAttributesContextBase
            //  Context pushed to indicate that all attribute processors have completed their
            //  work for a specific data node.
            struct SceneNodeAddedAttributesContextBase
                : public ImportContext
            {
                AZ_RTTI(SceneNodeAddedAttributesContextBase, "{65B97E48-16A0-4BBD-B364-CFDA9E3600B6}", ImportContext);

                SceneNodeAddedAttributesContextBase(SceneNodeAppendedContextBase& parent);
            };

            //  SceneNodeFinalizeContextBase
            //  Context pushed last after all other contexts for a scene node to allow any
            //  post-processing needed for an importer.
            struct SceneNodeFinalizeContextBase
                : public ImportContext
            {
                AZ_RTTI(SceneNodeFinalizeContextBase, "{F2C7D1BC-8065-423E-9212-241EB426A2BB}", ImportContext);

                SceneNodeFinalizeContextBase(SceneNodeAddedAttributesContextBase& parent);
            };

            //  FinalizeSceneContextBase
            //  Context pushed after the scene has been fully created. This can be used to finalize pending work
            //  such as resolving named links.
            struct FinalizeSceneContextBase
                : public ImportContext
            {
                AZ_RTTI(FinalizeSceneContextBase, "{91C54F51-9B4D-4C61-956C-9D530725D737}", ImportContext);

                FinalizeSceneContextBase(Containers::Scene& scene, RenamedNodesMap& nodeNameMap);
            };
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ

