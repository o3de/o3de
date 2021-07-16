/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneBuilder/ImportContexts/ImportContexts.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>

namespace AZ
{
    namespace AssImpSDKWrapper
    {
        class AssImpNodeWrapper;
        class AssImpSceneWrapper;
    }
    
    namespace SceneAPI
    {
        class SceneSystem;
        namespace SceneBuilder
        {
            class RenamedNodesMap;

            //  AssImpImportContext
            //  Base structure containing common data needed for all import contexts
            //  Member Variables:
            //      m_sourceNode - AssImp node being used for data processing.
            struct AssImpImportContext
            {
                AZ_RTTI(AssImpImportContext, "{B1076AFF-991B-423C-8D3E-D5C9230434AB}");

                AssImpImportContext(const AssImpSDKWrapper::AssImpSceneWrapper& sourceScene,
                    const SceneSystem& sourceSceneSystem,
                    AssImpSDKWrapper::AssImpNodeWrapper& sourceNode);

                const AssImpSDKWrapper::AssImpSceneWrapper& m_sourceScene;
                AssImpSDKWrapper::AssImpNodeWrapper& m_sourceNode;
                const SceneSystem& m_sourceSceneSystem; // Needed for unit and axis conversion
            };

            //  AssImpNodeEncounteredContext
            //  Context pushed to indicate that a new AssImp Node has been found and any
            //  importers that have means to process the contained data should do so
            struct AssImpNodeEncounteredContext
                : public AssImpImportContext
                , public NodeEncounteredContext
            {
                AZ_RTTI(AssImpNodeEncounteredContext, "{C2305BC5-EAEC-4515-BAD6-45E63C3FBD3D}", AssImpImportContext, NodeEncounteredContext);

                AssImpNodeEncounteredContext(Containers::Scene& scene,
                    Containers::SceneGraph::NodeIndex currentGraphPosition,
                    const AssImpSDKWrapper::AssImpSceneWrapper& sourceScene,
                    const SceneSystem& sourceSceneSystem,
                    RenamedNodesMap& nodeNameMap,
                    AssImpSDKWrapper::AssImpNodeWrapper& sourceNode);

                AssImpNodeEncounteredContext(Events::ImportEventContext& parent,
                    Containers::SceneGraph::NodeIndex currentGraphPosition,
                    const AssImpSDKWrapper::AssImpSceneWrapper& sourceScene,
                    const SceneSystem& sourceSceneSystem,
                    RenamedNodesMap& nodeNameMap,
                    AssImpSDKWrapper::AssImpNodeWrapper& sourceNode);
            };

            //  AssImpSceneDataPopulatedContext
            //  Context pushed to indicate that a piece of scene data has been fully 
            //  processed and any importers that wish to place it within the scene graph
            //  may now do so.
            struct AssImpSceneDataPopulatedContext
                : public AssImpImportContext
                , public SceneDataPopulatedContextBase
            {
                AZ_RTTI(AssImpSceneDataPopulatedContext, "{888DA37E-4234-4990-AD50-E6E54AFA9C35}", AssImpImportContext, SceneDataPopulatedContextBase);

                AssImpSceneDataPopulatedContext(AssImpNodeEncounteredContext& parent,
                    const AZStd::shared_ptr<DataTypes::IGraphObject>& nodeData,
                    const AZStd::string& dataName);

                AssImpSceneDataPopulatedContext(Containers::Scene& scene,
                    Containers::SceneGraph::NodeIndex currentGraphPosition,
                    const AssImpSDKWrapper::AssImpSceneWrapper& sourceScene,
                    const SceneSystem& sourceSceneSystem,
                    RenamedNodesMap& nodeNameMap,
                    AssImpSDKWrapper::AssImpNodeWrapper& sourceNode,
                    const AZStd::shared_ptr<DataTypes::IGraphObject>& nodeData,
                    const AZStd::string& dataName);
            };

            //  AssImpSceneNodeAppendedContext
            //  Context pushed to indicate that data has been added to the scene graph. 
            //  Generally created due to the insertion of a node during SceneDataPopulatedContext
            //  processing.
            struct AssImpSceneNodeAppendedContext
                : public AssImpImportContext
                , public SceneNodeAppendedContextBase
            {
                AZ_RTTI(AssImpSceneNodeAppendedContext, "{9C8B688E-8ECD-4EF0-9AC6-21BBCFE8F5A3}", AssImpImportContext, SceneNodeAppendedContextBase);

                AssImpSceneNodeAppendedContext(AssImpSceneDataPopulatedContext& parent, Containers::SceneGraph::NodeIndex newIndex);
                AssImpSceneNodeAppendedContext(Containers::Scene& scene,
                    Containers::SceneGraph::NodeIndex currentGraphPosition,
                    const AssImpSDKWrapper::AssImpSceneWrapper& sourceScene,
                    const SceneSystem& sourceSceneSystem,
                    RenamedNodesMap& nodeNameMap,
                    AssImpSDKWrapper::AssImpNodeWrapper& sourceNode);
            };

            // AssImpSceneAttributeDataPopulatedContext
            // Context pushed to indicate that attribute data has been found and processed
            struct AssImpSceneAttributeDataPopulatedContext
                : public AssImpImportContext
                , public SceneAttributeDataPopulatedContextBase
            {
                AZ_RTTI(AssImpSceneAttributeDataPopulatedContext, "{A5EFB485-2F36-4214-972B-0EFF4EFBF33D}", AssImpImportContext, SceneAttributeDataPopulatedContextBase);

                AssImpSceneAttributeDataPopulatedContext(AssImpSceneNodeAppendedContext& parent,
                    const AZStd::shared_ptr<DataTypes::IGraphObject>& nodeData,
                    const Containers::SceneGraph::NodeIndex attributeNodeIndex,const AZStd::string& dataName);
            };

            // AssImpSceneAttributeNodeAppendedContext
            // Context pushed to indicate that an attribute node has been added to the scene graph
            struct AssImpSceneAttributeNodeAppendedContext
                : public AssImpImportContext
                , public SceneAttributeNodeAppendedContextBase
            {
                AZ_RTTI(AssImpSceneAttributeNodeAppendedContext, "{96FDC405-2D3B-4030-A301-B3A2B5432498}", AssImpImportContext, SceneAttributeNodeAppendedContextBase);

                AssImpSceneAttributeNodeAppendedContext(AssImpSceneAttributeDataPopulatedContext& parent, Containers::SceneGraph::NodeIndex newIndex);
            };

            //  AssImpSceneNodeAddedAttributesContext
            //  Context pushed to indicate that all attribute processors have completed their
            //  work for a specific data node.
            struct AssImpSceneNodeAddedAttributesContext
                : public AssImpImportContext
                , public SceneNodeAddedAttributesContextBase
            {
                AZ_RTTI(AssImpSceneNodeAddedAttributesContext, "{D305EAA5-5F16-4AAD-805D-DF07A1B355B9}", AssImpImportContext, SceneNodeAddedAttributesContextBase);

                AssImpSceneNodeAddedAttributesContext(AssImpSceneNodeAppendedContext& parent);
            };

            //  AssImpSceneNodeFinalizeContext
            //  Context pushed last after all other contexts for a scene node to allow any
            //  post-processing needed for an importer.
            struct AssImpSceneNodeFinalizeContext
                : public AssImpImportContext
                , public SceneNodeFinalizeContextBase
            {
                AZ_RTTI(AssImpSceneNodeFinalizeContext, "{FD8B4AD5-3735-4D55-9455-504AB1DCA655}", AssImpImportContext, SceneNodeFinalizeContextBase);

                AssImpSceneNodeFinalizeContext(AssImpSceneNodeAddedAttributesContext& parent);
            };

            //  AssImpFinalizeSceneContext
            //  Context pushed after the scene has been fully created. This can be used to finalize pending work
            //  such as resolving named links.
            struct AssImpFinalizeSceneContext
                : public FinalizeSceneContextBase
            {
                AZ_RTTI(AssImpFinalizeSceneContext, "{6B23A54A-44BF-4661-A130-6B4D06A57B9F}", FinalizeSceneContextBase);

                AssImpFinalizeSceneContext(
                    Containers::Scene& scene,
                    const AssImpSDKWrapper::AssImpSceneWrapper& sourceScene,
                    const SceneSystem& sourceSceneSystem,
                    RenamedNodesMap& nodeNameMap);

                const AssImpSDKWrapper::AssImpSceneWrapper& m_sourceScene;
                const SceneSystem& m_sourceSceneSystem; // Needed for unit and axis conversion
            };
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ

