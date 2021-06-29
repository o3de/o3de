/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/FbxSceneBuilder/ImportContexts/ImportContexts.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>

namespace AZ
{
    struct Uuid;

    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            struct FbxImportContext;

            using CoreScene = Containers::Scene;
            using CoreSceneGraph = Containers::SceneGraph;
            using CoreGraphNodeIndex = Containers::SceneGraph::NodeIndex;
            using CoreProcessingResult = Events::ProcessingResult;

            inline bool NodeIsOfType(const CoreSceneGraph& graph, CoreGraphNodeIndex nodeIndex, const AZ::Uuid& uuid);
            inline bool NodeParentIsOfType(const CoreSceneGraph& graph, CoreGraphNodeIndex nodeIndex, 
                const AZ::Uuid& uuid);
            inline bool NodeHasAncestorOfType(const CoreSceneGraph& graph, CoreGraphNodeIndex nodeIndex,
                const AZ::Uuid& uuid);
            CoreProcessingResult AddDataNodeWithContexts(SceneDataPopulatedContextBase& dataContext);
            CoreProcessingResult AddAttributeDataNodeWithContexts(SceneAttributeDataPopulatedContextBase& dataContext);
            bool AreSceneGraphsEqual(const CoreSceneGraph& lhsGraph, const CoreSceneGraph& rhsGraph);
            inline bool AreScenesEqual(const CoreScene& lhs, const CoreScene& rhs);

            bool IsGraphDataEqual(const AZStd::shared_ptr<const DataTypes::IGraphObject>& lhs,
                const AZStd::shared_ptr<const DataTypes::IGraphObject>& rhs);

        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ

#include <SceneAPI/FbxSceneBuilder/Importers/ImporterUtilities.inl>
