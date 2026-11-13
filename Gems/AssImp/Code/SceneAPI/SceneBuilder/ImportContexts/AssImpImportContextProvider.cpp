/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssImpImportContextProvider.h"
#include <SceneAPI/SceneBuilder/ImportContexts/AssImpImportContexts.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Asset/AssetUtils.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            AssImpImportContextProvider::~AssImpImportContextProvider() = default;

            void AssImpImportContextProvider::GetSupportedFileExtensions(AZStd::unordered_set<AZStd::string>& extensions) const
            {
                auto* settingsRegistry = AZ::SettingsRegistry::Get();
                if (!settingsRegistry)
                {
                    return;
                }

                const auto extensionsKey = AZ::SettingsRegistryInterface::FixedValueString::format(
                    "%s/%s", AzToolsFramework::AssetUtils::AssetImporterProviderFileTypesKey, GetImporterName().c_str());

                auto collectExtension = [settingsRegistry, &extensions](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
                {
                    if (AZStd::string extension; settingsRegistry->Get(extension, visitArgs.m_jsonKeyPath))
                    {
                        extensions.insert(AZStd::move(extension));
                    }
                    return AZ::SettingsRegistryInterface::VisitResponse::Continue;
                };

                if (!AZ::SettingsRegistryVisitorUtils::VisitArray(*settingsRegistry, collectExtension, extensionsKey))
                {
                    AZ_Warning(
                        "AssImp", false,
                        "No AssImp file extensions found at '%s'. AssImp will not advertise any scene formats.",
                        extensionsKey.c_str());
                }
            }

            AZStd::shared_ptr<NodeEncounteredContext> AssImpImportContextProvider::CreateNodeEncounteredContext(
                Containers::Scene& scene,
                Containers::SceneGraph::NodeIndex currentGraphPosition,
                const SceneSystem& sourceSceneSystem,
                RenamedNodesMap& nodeNameMap,
                SDKScene::SceneWrapperBase& sourceScene,
                SDKNode::NodeWrapper& sourceNode)
            {
                // need to cast NodeWrapper
                auto assImpNode = azrtti_cast<AZ::AssImpSDKWrapper::AssImpNodeWrapper*>(&sourceNode);
                auto assImpScene = azrtti_cast<AZ::AssImpSDKWrapper::AssImpSceneWrapper*>(&sourceScene);

                if (!assImpNode)
                {
                    // Handle error: parent is not of the expected type
                    AZ_Error("SceneBuilder", false, "Incorrect node type. Cannot create NodeEncounteredContext");
                    return nullptr;
                }
                if (!assImpScene)
                {
                    AZ_Error("SceneBuilder", false, "Incorrect scene type. Cannot create NodeEncounteredContext");
                    return nullptr;
                }
                auto context = AZStd::make_shared<AssImpNodeEncounteredContext>(
                    scene, currentGraphPosition, *assImpScene, sourceSceneSystem, nodeNameMap, *assImpNode);
                context->m_contextProvider = this;
                return context;
            }

            AZStd::shared_ptr<SceneDataPopulatedContextBase> AssImpImportContextProvider::CreateSceneDataPopulatedContext(
                NodeEncounteredContext& parent, AZStd::shared_ptr<DataTypes::IGraphObject> graphData, const AZStd::string& dataName)
            {
                // Downcast the parent to the AssImp-specific type to access AssImpImportContext members
                AssImpNodeEncounteredContext* assImpParent = azrtti_cast<AssImpNodeEncounteredContext*>(&parent);
                if (!assImpParent)
                {
                    AZ_Error("SceneBuilder", false, "Incorrect type of parent. Cannot create SceneDataPopulatedContext");
                    return nullptr;
                }

                auto context = AZStd::make_shared<AssImpSceneDataPopulatedContext>(*assImpParent, AZStd::move(graphData), dataName);

                context->m_contextProvider = this;
                return context;
            }

            AZStd::shared_ptr<SceneNodeAppendedContextBase> AssImpImportContextProvider::CreateSceneNodeAppendedContext(
                SceneDataPopulatedContextBase& parent, Containers::SceneGraph::NodeIndex newIndex)
            {
                // Downcast the parent to the AssImp-specific type
                AssImpSceneDataPopulatedContext* assImpParent = azrtti_cast<AssImpSceneDataPopulatedContext*>(&parent);
                if (!assImpParent)
                {
                    AZ_Error("SceneBuilder", false, "Incorrect type of parent. Cannot create SceneNodeAppendedContext");
                    return nullptr;
                }

                auto context = AZStd::make_shared<AssImpSceneNodeAppendedContext>(*assImpParent, newIndex);
                context->m_contextProvider = this;
                return context;
            }

            AZStd::shared_ptr<SceneAttributeDataPopulatedContextBase> AssImpImportContextProvider::CreateSceneAttributeDataPopulatedContext(
                SceneNodeAppendedContextBase& parent,
                AZStd::shared_ptr<DataTypes::IGraphObject> nodeData,
                const Containers::SceneGraph::NodeIndex attributeNodeIndex,
                const AZStd::string& dataName)
            {
                // Downcast the parent to the AssImp-specific type
                AssImpSceneNodeAppendedContext* assImpParent = azrtti_cast<AssImpSceneNodeAppendedContext*>(&parent);
                if (!assImpParent)
                {
                    AZ_Error("SceneBuilder", false, "Incorrect type of parent. Cannot create SceneAttributeDataPopulatedContext");
                    return nullptr;
                }
                auto context = AZStd::make_shared<AssImpSceneAttributeDataPopulatedContext>(
                    *assImpParent, AZStd::move(nodeData), attributeNodeIndex, dataName);
                context->m_contextProvider = this;
                return context;
            }

            AZStd::shared_ptr<SceneAttributeNodeAppendedContextBase> AssImpImportContextProvider::CreateSceneAttributeNodeAppendedContext(
                SceneAttributeDataPopulatedContextBase& parent, Containers::SceneGraph::NodeIndex newIndex)
            {
                // Downcast the parent to the AssImp-specific type
                AssImpSceneAttributeDataPopulatedContext* assImpParent = azrtti_cast<AssImpSceneAttributeDataPopulatedContext*>(&parent);
                if (!assImpParent)
                {
                    AZ_Error("SceneBuilder", false, "Incorrect type of parent. Cannot create SceneAttributeNodeAppendedContext");
                    return nullptr;
                }
                auto context = AZStd::make_shared<AssImpSceneAttributeNodeAppendedContext>(*assImpParent, newIndex);
                context->m_contextProvider = this;
                return context;
            }

            AZStd::shared_ptr<SceneNodeAddedAttributesContextBase> AssImpImportContextProvider::CreateSceneNodeAddedAttributesContext(
                SceneNodeAppendedContextBase& parent)
            {
                // Downcast the parent to the AssImp-specific type
                AssImpSceneNodeAppendedContext* assImpParent = azrtti_cast<AssImpSceneNodeAppendedContext*>(&parent);
                if (!assImpParent)
                {
                    AZ_Error("SceneBuilder", false, "Incorrect type of parent. Cannot create SceneNodeAddedAttributesContext");
                    return nullptr;
                }
                auto context = AZStd::make_shared<AssImpSceneNodeAddedAttributesContext>(*assImpParent);
                context->m_contextProvider = this;
                return context;
            }

            AZStd::shared_ptr<SceneNodeFinalizeContextBase> AssImpImportContextProvider::CreateSceneNodeFinalizeContext(
                SceneNodeAddedAttributesContextBase& parent)
            {
                // Downcast the parent to the AssImp-specific type
                AssImpSceneNodeAddedAttributesContext* assImpParent = azrtti_cast<AssImpSceneNodeAddedAttributesContext*>(&parent);
                if (!assImpParent)
                {
                    AZ_Error("SceneBuilder", false, "Incorrect type of parent. Cannot create SceneNodeFinalizeContext");
                    return nullptr;
                }
                auto context = AZStd::make_shared<AssImpSceneNodeFinalizeContext>(*assImpParent);
                context->m_contextProvider = this;
                return context;
            }

            AZStd::shared_ptr<FinalizeSceneContextBase> AssImpImportContextProvider::CreateFinalizeSceneContext(
                Containers::Scene& scene,
                const SceneSystem& sourceSceneSystem,
                SDKScene::SceneWrapperBase& sourceScene,
                RenamedNodesMap& nodeNameMap)
            {
                AssImpSDKWrapper::AssImpSceneWrapper* assImpScene = azrtti_cast<AssImpSDKWrapper::AssImpSceneWrapper*>(&sourceScene);
                if (!assImpScene)
                {
                    AZ_Error("SceneBuilder", false, "Incorrect scene type. Cannot create FinalizeSceneContext");
                    return nullptr;
                }
                auto context = AZStd::make_shared<AssImpFinalizeSceneContext>(scene, *assImpScene, sourceSceneSystem, nodeNameMap);
                context->m_contextProvider = this;
                return context;
            }
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
