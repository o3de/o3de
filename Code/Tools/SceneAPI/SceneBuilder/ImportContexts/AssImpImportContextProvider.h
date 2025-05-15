/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ImportContextProvider.h"

#include <AzCore/RTTI/RTTIMacros.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <SceneAPI/SceneBuilder/ImportContexts/ImportContextProvider.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            // Concrete provider for creating AssImp-specific import classes.
            struct AssImpImportContextProvider : public ImportContextProvider
            {
                AZ_RTTI(AssImpImportContextProvider, "{6c263adb-e73c-4017-955a-9c212ded3637}");

                AssImpImportContextProvider() = default;

                AZStd::shared_ptr<NodeEncounteredContext> CreateNodeEncounteredContext(
                    Containers::Scene& scene,
                    Containers::SceneGraph::NodeIndex currentGraphPosition,
                    const SceneSystem& sourceSceneSystem,
                    RenamedNodesMap& nodeNameMap,
                    SDKScene::SceneWrapperBase& sourceScene,
                    SDKNode::NodeWrapper& sourceNode) override;

                AZStd::shared_ptr<SceneDataPopulatedContextBase> CreateSceneDataPopulatedContext(
                    NodeEncounteredContext& parent,
                    AZStd::shared_ptr<DataTypes::IGraphObject> graphData,
                    const AZStd::string& dataName) override;

                AZStd::shared_ptr<SceneNodeAppendedContextBase> CreateSceneNodeAppendedContext(
                    SceneDataPopulatedContextBase& parent, Containers::SceneGraph::NodeIndex newIndex) override;

                AZStd::shared_ptr<SceneAttributeDataPopulatedContextBase> CreateSceneAttributeDataPopulatedContext(
                    SceneNodeAppendedContextBase& parent,
                    AZStd::shared_ptr<DataTypes::IGraphObject> nodeData,
                    const Containers::SceneGraph::NodeIndex attributeNodeIndex,
                    const AZStd::string& dataName) override;

                AZStd::shared_ptr<SceneAttributeNodeAppendedContextBase> CreateSceneAttributeNodeAppendedContext(
                    SceneAttributeDataPopulatedContextBase& parent, Containers::SceneGraph::NodeIndex newIndex) override;

                AZStd::shared_ptr<SceneNodeAddedAttributesContextBase> CreateSceneNodeAddedAttributesContext(
                    SceneNodeAppendedContextBase& parent) override;

                AZStd::shared_ptr<SceneNodeFinalizeContextBase> CreateSceneNodeFinalizeContext(
                    SceneNodeAddedAttributesContextBase& parent) override;

                AZStd::shared_ptr<FinalizeSceneContextBase> CreateFinalizeSceneContext(
                    Containers::Scene& scene,
                    const SceneSystem& sourceSceneSystem,
                    SDKScene::SceneWrapperBase& sourceScene,
                    RenamedNodesMap& nodeNameMap) override;

                bool CanHandleExtension(AZStd::string_view fileExtension) const override
                {
                    // The AssImp is our default provider and returns true for all registered extensions.
                    return true;
                }

                AZStd::unique_ptr<AZ::SDKScene::SceneWrapperBase> CreateSceneWrapper() const override
                {
                    return AZStd::make_unique<AZ::AssImpSDKWrapper::AssImpSceneWrapper>();
                }

                AZStd::string_view GetImporterName() const override
                {
                    return "AssImp";
                }
            };
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
