/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneBuilder/SceneImporter.h>
#include <SceneAPI/SceneBuilder/ImportContexts/AssImpImportContexts.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpMaterialImporter.h>
#include <SceneAPI/SceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/SceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IImportGroup.h>
#include <SceneAPI/SceneCore/Import/ManifestImportRequestHandler.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/GraphData/TransformData.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>


namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            struct QueueNode
            {
                std::shared_ptr<SDKNode::NodeWrapper> m_node;
                Containers::SceneGraph::NodeIndex m_parent;

                QueueNode() = delete;
                QueueNode(std::shared_ptr<SDKNode::NodeWrapper>&& node, Containers::SceneGraph::NodeIndex parent)
                    : m_node(std::move(node))
                    , m_parent(parent)
                {
                }
            };

            SceneImporter::SceneImporter()
                : m_sceneSystem(new SceneSystem())
            {
                m_sceneWrapper = AZStd::make_unique<AssImpSDKWrapper::AssImpSceneWrapper>();
                BindToCall(&SceneImporter::ImportProcessing);
            }

            void SceneImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<SceneImporter, SceneCore::LoadingComponent>()->Version(2); // SPEC-5776
                }
            }

            SceneAPI::SceneImportSettings SceneImporter::GetSceneImportSettings(const AZStd::string& sourceAssetPath) const
            {
                // Start with a default set of import settings.
                SceneAPI::SceneImportSettings importSettings;

                // Try to read in any global settings from the settings registry.
                if (AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry)
                {
                    settingsRegistry->GetObject(importSettings, AZ::SceneAPI::DataTypes::IImportGroup::SceneImportSettingsRegistryKey);
                }

                // Try reading in the scene manifest (.assetinfo file), which contains the import settings if they've been
                // changed from the defaults.
                Containers::Scene scene;
                Import::ManifestImportRequestHandler manifestHandler;
                manifestHandler.LoadAsset(
                    scene, sourceAssetPath,
                    Uuid::CreateNull(),
                    Events::AssetImportRequest::RequestingApplication::AssetProcessor);

                // Search for the ImportGroup. If it's there, get the new import settings. If not, we'll just use the defaults.
                size_t count = scene.GetManifest().GetEntryCount();
                for (size_t index = 0; index < count; index++)
                {
                    if (auto* importGroup = azrtti_cast<DataTypes::IImportGroup*>(scene.GetManifest().GetValue(index).get()); importGroup)
                    {
                        importSettings = importGroup->GetImportSettings();
                        break;
                    }
                }

                return importSettings;
            }

            Events::ProcessingResult SceneImporter::ImportProcessing(Events::ImportEventContext& context)
            {
                SceneAPI::SceneImportSettings importSettings = GetSceneImportSettings(context.GetInputDirectory());

                m_sceneWrapper->Clear();

                if (!m_sceneWrapper->LoadSceneFromFile(context.GetInputDirectory().c_str(), importSettings))
                {
                    return Events::ProcessingResult::Failure;
                }

                m_sceneSystem->Set(m_sceneWrapper.get());
                if (!azrtti_istypeof<AssImpSDKWrapper::AssImpSceneWrapper>(m_sceneWrapper.get()))
                {
                    return Events::ProcessingResult::Failure;
                }

                if (ConvertScene(context.GetScene()))
                {
                    return Events::ProcessingResult::Success;
                }
                else
                {
                    return Events::ProcessingResult::Failure;
                }
            }

            bool SceneImporter::ConvertScene(Containers::Scene& scene) const
            {
                std::shared_ptr<SDKNode::NodeWrapper> sceneRoot = m_sceneWrapper->GetRootNode();
                if (!sceneRoot)
                {
                    return false;
                }

                const AssImpSDKWrapper::AssImpSceneWrapper* assImpSceneWrapper = azrtti_cast <AssImpSDKWrapper::AssImpSceneWrapper*>(m_sceneWrapper.get());

                AZStd::pair<AssImpSDKWrapper::AssImpSceneWrapper::AxisVector, int32_t> upAxisAndSign = assImpSceneWrapper->GetUpVectorAndSign();

                const aiAABB& aabb = assImpSceneWrapper->GetAABB();
                aiVector3t dimension = aabb.mMax - aabb.mMin;
                Vector3 t{ dimension.x, dimension.y, dimension.z };
                scene.SetSceneDimension(t);
                scene.SetSceneVertices(assImpSceneWrapper->GetVertices());

                if (upAxisAndSign.second <= 0)
                {
                    AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Negative scene orientation is not a currently supported orientation.");
                    return false;
                }
                switch (upAxisAndSign.first)
                {
                case AssImpSDKWrapper::AssImpSceneWrapper::AxisVector::X:
                    scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::XUp);
                    break;
                case AssImpSDKWrapper::AssImpSceneWrapper::AxisVector::Y:
                    scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::YUp);
                    break;
                case AssImpSDKWrapper::AssImpSceneWrapper::AxisVector::Z:
                    scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::ZUp);
                    break;
                default:
                    AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Unknown scene orientation, %d.", upAxisAndSign.first);
                    AZ_Assert(false, "Unknown scene orientation, %d.", upAxisAndSign.first);
                    break;
                }

                AZStd::queue<SceneBuilder::QueueNode> nodes;
                nodes.emplace(AZStd::move(sceneRoot), scene.GetGraph().GetRoot());
                RenamedNodesMap nodeNameMap;

                while (!nodes.empty())
                {
                    SceneBuilder::QueueNode& node = nodes.front();
                    AZ_Assert(node.m_node, "Empty asset importer node queued");
                    if (!nodeNameMap.RegisterNode(node.m_node, scene.GetGraph(), node.m_parent))
                    {
                        AZ_TracePrintf(Utilities::ErrorWindow, "Failed to register asset importer node in name table.");
                        // Skip this node since it could not be registered
                        nodes.pop();
                        continue;
                    }
                    AZStd::string nodeName = nodeNameMap.GetNodeName(node.m_node);
                    SanitizeNodeName(nodeName);

                    AZ_TraceContext("SceneAPI Node Name", nodeName);
                    Containers::SceneGraph::NodeIndex newNode = scene.GetGraph().AddChild(node.m_parent, nodeName.c_str());

                    AZ_Error(Utilities::ErrorWindow, newNode.IsValid(), "Failed to add Asset Importer node to scene graph");
                    if (!newNode.IsValid())
                    {
                        continue;
                    }

                    AssImpNodeEncounteredContext sourceNodeEncountered(scene, newNode, *assImpSceneWrapper, *m_sceneSystem, nodeNameMap, *azrtti_cast<AZ::AssImpSDKWrapper::AssImpNodeWrapper*>(node.m_node.get()));
                    Events::ProcessingResultCombiner nodeResult;
                    nodeResult += Events::Process(sourceNodeEncountered);

                    // If no importer created data, we still create an empty node that may eventually contain a transform
                    if (sourceNodeEncountered.m_createdData.empty())
                    {
                        AZ_Assert(nodeResult.GetResult() != Events::ProcessingResult::Success,
                            "Importers returned success but no data was created");
                        AZStd::shared_ptr<DataTypes::IGraphObject> nullData(nullptr);
                        sourceNodeEncountered.m_createdData.emplace_back(nullData);
                        nodeResult += Events::ProcessingResult::Success;
                    }

                    // Create single node since only one piece of graph data was created
                    if (sourceNodeEncountered.m_createdData.size() == 1)
                    {
                        AZ_Assert(nodeResult.GetResult() != Events::ProcessingResult::Ignored,
                            "An importer created data, but did not return success");
                        if (nodeResult.GetResult() == Events::ProcessingResult::Failure)
                        {
                            AZ_TracePrintf(Utilities::ErrorWindow, "One or more importers failed to create data.");
                        }

                        AssImpSceneDataPopulatedContext dataProcessed(sourceNodeEncountered,
                            sourceNodeEncountered.m_createdData[0], nodeName.c_str());
                        Events::ProcessingResult result = AddDataNodeWithContexts(dataProcessed);
                        if (result != Events::ProcessingResult::Failure)
                        {
                            newNode = dataProcessed.m_currentGraphPosition;
                        }
                    }
                    // Create an empty parent node and place all data under it. The remaining
                    // tree will be built off of this as the logical parent
                    else
                    {
                        AZ_Assert(nodeResult.GetResult() != Events::ProcessingResult::Ignored,
                            "%i importers created data, but did not return success",
                            sourceNodeEncountered.m_createdData.size());
                        if (nodeResult.GetResult() == Events::ProcessingResult::Failure)
                        {
                            AZ_TracePrintf(Utilities::ErrorWindow, "One or more importers failed to create data.");
                        }

                        size_t offset = nodeName.length();
                        for (size_t i = 0; i < sourceNodeEncountered.m_createdData.size(); ++i)
                        {
                            nodeName += '_';
                            nodeName += AZStd::to_string(aznumeric_cast<AZ::u64>(i + 1));

                            Containers::SceneGraph::NodeIndex subNode =
                                scene.GetGraph().AddChild(newNode, nodeName.c_str());
                            AZ_Assert(subNode.IsValid(), "Failed to create new scene sub node");
                            AssImpSceneDataPopulatedContext dataProcessed(sourceNodeEncountered,
                                sourceNodeEncountered.m_createdData[i], nodeName);
                            dataProcessed.m_currentGraphPosition = subNode;
                            AddDataNodeWithContexts(dataProcessed);

                            // Remove the temporary extension again.
                            nodeName.erase(offset, nodeName.length() - offset);
                        }
                    }

                    AZ_Assert(nodeResult.GetResult() == Events::ProcessingResult::Success,
                        "No importers successfully added processed scene data.");
                    AZ_Assert(newNode != node.m_parent,
                        "Failed to update current graph position during data processing.");

                    int childCount = node.m_node->GetChildCount();
                    for (int i = 0; i < childCount; ++i)
                    {
                        const std::shared_ptr<SDKNode::NodeWrapper> nodeWrapper = node.m_node->GetChild(i);
                        auto assImpNodeWrapper = azrtti_cast<AssImpSDKWrapper::AssImpNodeWrapper*>(nodeWrapper.get());

                        AZ_Assert(assImpNodeWrapper, "Child node is not the expected AssImpNodeWrapper type");

                        std::shared_ptr<AssImpSDKWrapper::AssImpNodeWrapper> child = std::make_shared<AssImpSDKWrapper::AssImpNodeWrapper>(assImpNodeWrapper->GetAssImpNode());
                        if (child)
                        {
                            nodes.emplace(AZStd::move(child), newNode);
                        }
                    }

                    nodes.pop();
                };

                Events::ProcessingResult result = Events::Process<AssImpFinalizeSceneContext>(scene, *assImpSceneWrapper, *m_sceneSystem, nodeNameMap);
                if (result == Events::ProcessingResult::Failure)
                {
                    return false;
                }

                return true;
            }

            void SceneImporter::SanitizeNodeName(AZStd::string& nodeName) const
            {
                // Replace % with something else so it is safe for use in printfs.
                AZStd::replace(nodeName.begin(), nodeName.end(), '%', '_');
            }

        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
