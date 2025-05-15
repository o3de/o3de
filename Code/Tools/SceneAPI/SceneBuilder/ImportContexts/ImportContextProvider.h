/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ImportContexts.h"

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SDKWrapper/NodeWrapper.h>
#include <SceneAPI/SceneBuilder/SceneSystem.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>

namespace AZ::SDKScene
{
    class SceneWrapperBase;
}
namespace AZ::SDKNode
{
    class NodeWrapper;
}
namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }

        namespace SceneBuilder
        {
            class RenamedNodesMap;
            struct NodeEncounteredContext;
            struct SceneDataPopulatedContextBase;
            struct SceneNodeAppendedContextBase;
            struct SceneAttributeDataPopulatedContextBase;
            struct SceneAttributeNodeAppendedContextBase;
            struct SceneNodeAddedAttributesContextBase;
            struct SceneNodeFinalizeContextBase;
            struct FinalizeSceneContextBase;
            // ImportContextProvider realizes factory pattern and provides classes specialized for particular Scene Import library.
            struct ImportContextProvider
            {
                AZ_RTTI(ImportContextProvider, "{5df22f6c-8a43-417d-b735-9d9d7d069efc}");

                virtual ~ImportContextProvider() = default;

                virtual AZStd::shared_ptr<NodeEncounteredContext> CreateNodeEncounteredContext(
                    Containers::Scene& scene,
                    Containers::SceneGraph::NodeIndex currentGraphPosition,
                    const SceneSystem& sourceSceneSystem,
                    RenamedNodesMap& nodeNameMap,
                    SDKScene::SceneWrapperBase& sourceScene,
                    AZ::SDKNode::NodeWrapper& sourceNode) = 0;

                virtual AZStd::shared_ptr<SceneDataPopulatedContextBase> CreateSceneDataPopulatedContext(
                    NodeEncounteredContext& parent,
                    AZStd::shared_ptr<DataTypes::IGraphObject> graphData,
                    const AZStd::string& dataName) = 0;

                 virtual AZStd::shared_ptr<SceneNodeAppendedContextBase> CreateSceneNodeAppendedContext(
                    SceneDataPopulatedContextBase& parent, Containers::SceneGraph::NodeIndex newIndex) = 0;

                virtual AZStd::shared_ptr<SceneAttributeDataPopulatedContextBase> CreateSceneAttributeDataPopulatedContext(
                    SceneNodeAppendedContextBase& parent,
                    AZStd::shared_ptr<DataTypes::IGraphObject> nodeData,
                    const Containers::SceneGraph::NodeIndex attributeNodeIndex,
                    const AZStd::string& dataName) = 0;

                virtual AZStd::shared_ptr<SceneAttributeNodeAppendedContextBase> CreateSceneAttributeNodeAppendedContext(
                    SceneAttributeDataPopulatedContextBase& parent, Containers::SceneGraph::NodeIndex newIndex) = 0;

                virtual AZStd::shared_ptr<SceneNodeAddedAttributesContextBase> CreateSceneNodeAddedAttributesContext(
                    SceneNodeAppendedContextBase& parent) = 0;

                virtual AZStd::shared_ptr<SceneNodeFinalizeContextBase> CreateSceneNodeFinalizeContext(
                    SceneNodeAddedAttributesContextBase& parent) = 0;

                virtual AZStd::shared_ptr<FinalizeSceneContextBase> CreateFinalizeSceneContext(
                    Containers::Scene& scene,
                    const SceneSystem& sourceSceneSystem,
                    SDKScene::SceneWrapperBase& sourceScene,
                    RenamedNodesMap& nodeNameMap) = 0;

                // Creates an instance of the scene wrapper
                virtual AZStd::unique_ptr<SDKScene::SceneWrapperBase> CreateSceneWrapper() const = 0;

                // Checks if this provider can handle the given file extension
                virtual bool CanHandleExtension(AZStd::string_view fileExtension) const = 0;

                // Get a descriptive name for Context Provider
                virtual AZStd::string_view GetImporterName() const { return "Unknown Importer"; }
            };
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
