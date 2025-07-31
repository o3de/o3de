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

            //! @brief Abstract Factory Pattern interface for scene import context providers.
            //!
            //! The ImportContextProvider allows different scene import libraries to be integrated as Gems
            //! by providing a standard interface for creating scene wrapper, import contexts (@see ImportContexts.h) and handling
            //! file extensions. This enables the Scene API pipeline to work with multiple import libraries
            //! while maintaining a consistent interface.
            //!
            //! Key responsibilities:
            //! - Provides factory methods for creating a family of related import contexts @see ImportContexts.h and a SceneWrapper
            //! - Provides the list of handled file extensions
            //! - Offers abstraction layer between SceneAPI and import library implementations
            //!
            //! @par Usage
            //! Implement this interface in your custom import library Gem and register with:
            //! @code{.cpp}
            //! if (auto* registry = AZ::SceneAPI::SceneBuilder::ImportContextRegistryInterface::Get())
            //! {
            //!     // Create and register the new Context Provider
            //!     auto contextProvider = aznew AZ::SceneAPI::SceneBuilder::AwesomeLibImportContextProvider();
            //!     registry->RegisterContextProvider(contextProvider);
            //!     AZ_Printf("AwesomeLibImporter", "Awesome Lib import context provider registered.\n");
            //! }
            //! @endcode
            //!
            //! @note Ensure your Component inherits from SceneCore::SceneSystemComponent.
            //!       This is a different hierarchy than regular SystemComponents.
            //!
            //! @see AssImpImportContextProvider.h for reference implementation.
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

                //! Creates an instance of the scene wrapper
                virtual AZStd::unique_ptr<SDKScene::SceneWrapperBase> CreateSceneWrapper() const = 0;

                //! Checks if this provider can handle the given file extension
                virtual bool CanHandleExtension(AZStd::string_view fileExtension) const = 0;

                //! Get a descriptive name for Context Provider
                virtual AZStd::string GetImporterName() const { return "Unknown Importer"; }
            };
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
