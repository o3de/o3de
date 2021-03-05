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

#include <SceneAPI/FbxSceneBuilder/ImportContexts/FbxImportContexts.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>


namespace AZ
{
    struct Uuid;

    namespace FbxSDKWrapper
    {
        class FbxNodeWrapper;
        class FbxSceneWrapper;
    }

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
            inline bool IsSkinnedMesh(const FbxSDKWrapper::FbxNodeWrapper& sourceNode);
            CoreProcessingResult AddDataNodeWithContexts(SceneDataPopulatedContextBase& dataContext);
            CoreProcessingResult AddAttributeDataNodeWithContexts(SceneAttributeDataPopulatedContextBase& dataContext);
            bool AreSceneGraphsEqual(const CoreSceneGraph& lhsGraph, const CoreSceneGraph& rhsGraph);
            inline bool AreScenesEqual(const CoreScene& lhs, const CoreScene& rhs);

            bool IsGraphDataEqual(const AZStd::shared_ptr<const DataTypes::IGraphObject>& lhs,
                const AZStd::shared_ptr<const DataTypes::IGraphObject>& rhs);

            // If the scene contains bindpose information for the node, returns true and sets "xf" to the local transform
            // of the node in bindpose. Returns false if bindpose info is not available for the node.
            bool GetBindPoseLocalTransform(const FbxSDKWrapper::FbxSceneWrapper& sceneWrapper,
                FbxSDKWrapper::FbxNodeWrapper& nodeWrapper, DataTypes::MatrixType& xf);
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ

#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.inl>
