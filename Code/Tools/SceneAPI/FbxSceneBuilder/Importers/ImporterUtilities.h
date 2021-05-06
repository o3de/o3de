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
