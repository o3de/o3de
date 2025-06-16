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
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneBuilder/ImportContextRegistry.h>
#include <SceneAPI/SceneBuilder/ImportContexts/ImportContextProvider.h>
#include <SceneAPI/SceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/SceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/SceneBuilder/SceneImporter.h>
#include <SceneAPI/SceneBuilder/SceneSystem.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IImportGroup.h>
#include <SceneAPI/SceneCore/Import/ManifestImportRequestHandler.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/GraphData/TransformData.h>

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
                , m_contextProvider(nullptr)
                , m_sceneWrapper(nullptr)
            {
                m_sceneWrapper = AZStd::make_unique<SDKScene::SceneWrapperBase>();
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

                AZStd::string filePath = context.GetInputDirectory();
                AZStd::string extension = AZ::IO::Path(filePath).Extension().String();
                AZStd::to_lower(extension);

                auto* registry = ImportContextRegistryInterface::Get();
                if (registry)
                {
                    m_contextProvider.reset(registry->SelectImportProvider(extension));
                }
                else
                {
                    AZ_Error("SceneBuilder", false, "ImportContextRegistry interface is not available.");
                    return Events::ProcessingResult::Failure;
                }

                if (!m_contextProvider)
                {
                    AZ_Error("SceneBuilder", false, "Cannot pick Import Context for file: %s", filePath.c_str());
                    return Events::ProcessingResult::Failure;
                }

                AZ_TracePrintf(
                    "SceneBuilder",
                    "Using '%s' Import Context Provider for file: %s",
                    m_contextProvider->GetImporterName().data(),
                    filePath.c_str());
                m_sceneWrapper = m_contextProvider->CreateSceneWrapper();
                if (!m_sceneWrapper->LoadSceneFromFile(context.GetInputDirectory().c_str(), importSettings))
                {
                    return Events::ProcessingResult::Failure;
                }

                m_sceneSystem->Set(m_sceneWrapper.get());

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

                AZStd::pair<SDKScene::SceneWrapperBase::AxisVector, int32_t> upAxisAndSign = m_sceneWrapper->GetUpVectorAndSign();

                const AZ::Aabb& aabb = m_sceneWrapper->GetAABB();
                scene.SetSceneDimension(aabb.GetExtents());
                scene.SetSceneVertices(m_sceneWrapper->GetVerticesCount());

                if (upAxisAndSign.second <= 0)
                {
                    AZ_TracePrintf(
                        SceneAPI::Utilities::ErrorWindow, "Negative scene orientation is not a currently supported orientation.");
                    return false;
                }
                switch (upAxisAndSign.first)
                {
                case SDKScene::SceneWrapperBase::AxisVector::X:
                    scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::XUp);
                    break;
                case SDKScene::SceneWrapperBase::AxisVector::Y:
                    scene.SetOriginalSceneOrientation(Containers::Scene::SceneOrientation::YUp);
                    break;
                case SDKScene::SceneWrapperBase::AxisVector::Z:
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

                    auto sourceNodeEncountered = m_contextProvider->CreateNodeEncounteredContext(
                        scene, newNode, *m_sceneSystem, nodeNameMap, *m_sceneWrapper, *(node.m_node));
                    Events::ProcessingResultCombiner nodeResult;
                    nodeResult += Events::Process(*sourceNodeEncountered);

                    // If no importer created data, we still create an empty node that may eventually contain a transform
                    if (sourceNodeEncountered->m_createdData.empty())
                    {
                        AZ_Assert(
                            nodeResult.GetResult() != Events::ProcessingResult::Success,
                            "Importers returned success but no data was created");
                        AZStd::shared_ptr<DataTypes::IGraphObject> nullData(nullptr);
                        sourceNodeEncountered->m_createdData.emplace_back(nullData);
                        nodeResult += Events::ProcessingResult::Success;
                    }

                    AZ_Assert(nodeResult.GetResult() != Events::ProcessingResult::Ignored,
                        "%i importer(s) created data, but did not return success",
                        sourceNodeEncountered->m_createdData.size());
                    if (nodeResult.GetResult() == Events::ProcessingResult::Failure)
                    {
                        AZ_TracePrintf(Utilities::ErrorWindow, "One or more importers failed to create data.");
                    }

                    size_t offset = nodeName.length();
                    for (size_t i = 0; i < sourceNodeEncountered->m_createdData.size(); ++i)
                    {
                        bool saveCreatedDataToNewNode = (sourceNodeEncountered->m_createdData.size() == 1 ||
                            sourceNodeEncountered->m_createdData[i]->RTTI_IsTypeOf(DataTypes::IBoneData::TYPEINFO_Uuid()));
                        if (!saveCreatedDataToNewNode)
                        {
                            nodeName += '_';
                            nodeName += AZStd::to_string(aznumeric_cast<AZ::u64>(i + 1));
                        }
                        auto dataProcessed = m_contextProvider->CreateSceneDataPopulatedContext(*sourceNodeEncountered,
                            sourceNodeEncountered->m_createdData[i], nodeName.c_str());
                        if (saveCreatedDataToNewNode)
                        {
                            // Create single node since only one piece of graph data was created
                            Events::ProcessingResult result = AddDataNodeWithContexts(*dataProcessed);
                            if (result != Events::ProcessingResult::Failure)
                            {
                                newNode = dataProcessed->m_currentGraphPosition;
                            }
                        }
                        else
                        {
                            // Create an empty parent node and place all data under it. The remaining
                            // tree will be built off of this as the logical parent
                            Containers::SceneGraph::NodeIndex subNode =
                                scene.GetGraph().AddChild(newNode, nodeName.c_str());
                            AZ_Assert(subNode.IsValid(), "Failed to create new scene sub node");
                            dataProcessed->m_currentGraphPosition = subNode;
                            AddDataNodeWithContexts(*dataProcessed);

                            // Remove the temporary extension again.
                            nodeName.erase(offset, nodeName.length() - offset);
                        }
                    }

                    AZ_Assert(nodeResult.GetResult() == Events::ProcessingResult::Success,
                        "No importers successfully added processed scene data.");
                    AZ_Assert(newNode != node.m_parent, "Failed to update current graph position during data processing.");

                    int childCount = node.m_node->GetChildCount();
                    for (int i = 0; i < childCount; ++i)
                    {
                        const std::shared_ptr<SDKNode::NodeWrapper> nodeWrapper = node.m_node->GetChild(i);
                        if (auto child = nodeWrapper)
                        {
                            nodes.emplace(AZStd::move(child), newNode);
                        }
                    }

                    nodes.pop();
                };

                auto finalizeSceneContext =
                    m_contextProvider->CreateFinalizeSceneContext(scene, *m_sceneSystem, *m_sceneWrapper, nodeNameMap);
                Events::ProcessingResult finalizeResult = Events::Process(*finalizeSceneContext);
                return finalizeResult != Events::ProcessingResult::Failure;
            }

            void SceneImporter::SanitizeNodeName(AZStd::string& nodeName) const
            {
                // Replace % with something else so it is safe for use in printfs.
                AZStd::replace(nodeName.begin(), nodeName.end(), '%', '_');
            }

        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
