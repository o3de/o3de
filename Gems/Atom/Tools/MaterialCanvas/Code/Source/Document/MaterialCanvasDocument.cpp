/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/DynamicNode/DynamicNode.h>
#include <AtomToolsFramework/DynamicNode/DynamicNodeManagerRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Document/MaterialCanvasDocument.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphModel/Model/Connection.h>

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/sort.h>

namespace MaterialCanvas
{
    void MaterialCanvasDocument::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MaterialCanvasDocument, AtomToolsFramework::AtomToolsDocument>()
                ->Version(0);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<MaterialCanvasDocumentRequestBus>("MaterialCanvasDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialcanvas")
                ->Event("GetGraphId", &MaterialCanvasDocumentRequests::GetGraphId)
                ;
        }
    }

    MaterialCanvasDocument::MaterialCanvasDocument(
        const AZ::Crc32& toolId,
        const AtomToolsFramework::DocumentTypeInfo& documentTypeInfo,
        AZStd::shared_ptr<GraphModel::GraphContext> graphContext)
        : AtomToolsFramework::AtomToolsDocument(toolId, documentTypeInfo)
        , m_graphContext(graphContext)
    {
        // Creating the scene entity and graph for this document. This may end up moving to the view if we can have the document only store
        // minimal material graph specific data that can be transformed into a graph canvas graph in the view and back. That abstraction
        // would help maintain a separation between the serialized data and the UI for rendering and interacting with the graph. This would
        // also help establish a mediator pattern for other node based tools that want to visualize their data or documents as a graph. My
        // understanding is that graph model will help with this.
        m_graph = AZStd::make_shared<GraphModel::Graph>(m_graphContext);

        GraphModelIntegration::GraphManagerRequestBus::BroadcastResult(
            m_sceneEntity, &GraphModelIntegration::GraphManagerRequests::CreateScene, m_graph, m_toolId);
        m_graphId = m_sceneEntity->GetId();

        RecordGraphState();

        // Listen for GraphController notifications on the new graph.
        GraphModelIntegration::GraphControllerNotificationBus::Handler::BusConnect(m_graphId);

        MaterialCanvasDocumentRequestBus::Handler::BusConnect(m_id);
    }

    MaterialCanvasDocument::~MaterialCanvasDocument()
    {
        MaterialCanvasDocumentRequestBus::Handler::BusDisconnect();

        // Stop listening for GraphController notifications for this graph.
        GraphModelIntegration::GraphControllerNotificationBus::Handler::BusDisconnect();

        GraphModelIntegration::GraphManagerRequestBus::Broadcast(
            &GraphModelIntegration::GraphManagerRequests::DeleteGraphController, m_graphId);

        m_graph.reset();
        m_graphId = GraphCanvas::GraphId();
        delete m_sceneEntity;
        m_sceneEntity = {};
    }

    AtomToolsFramework::DocumentTypeInfo MaterialCanvasDocument::BuildDocumentTypeInfo()
    {
        // Setting up placeholder document type info and extensions. This is not representative of final data.
        AtomToolsFramework::DocumentTypeInfo documentType;
        documentType.m_documentTypeName = "Material Canvas";
        documentType.m_documentFactoryCallback = [](const AZ::Crc32& toolId, const AtomToolsFramework::DocumentTypeInfo& documentTypeInfo) {
            // A list of all registered data types is needed to create a graph context
            GraphModel::DataTypeList registeredDataTypes;
            AtomToolsFramework::DynamicNodeManagerRequestBus::EventResult(
                registeredDataTypes, toolId, &AtomToolsFramework::DynamicNodeManagerRequestBus::Events::GetRegisteredDataTypes);

            // Creating a graph context per document by default. It can be overridden in the application to provide a shared context.
            auto graphContext = AZStd::make_shared<GraphModel::GraphContext>("Material Canvas", ".materialcanvas", registeredDataTypes);
            graphContext->CreateModuleGraphManager();
            return aznew MaterialCanvasDocument(toolId, documentTypeInfo, graphContext);
        };

        // Need to revisit the distinction between file types for creation versus file types for opening. Creation types are meant to be
        // used as templates or documents from which another is derived, like material types or parents. Currently a combination of the
        // filters is used to determine how the create document dialog is populated. The base document class rejects file types that are not
        // listed in the extension supported for opening. Will change to make the base class support opening anything listed in open
        // or create and the create dialog look at the create list exclusively.
        documentType.m_supportedExtensionsToCreate.push_back({ "Material Canvas Template", "materialcanvastemplate.azasset" });
        documentType.m_supportedExtensionsToCreate.push_back({ "Material Canvas", "materialcanvas.azasset" });
        documentType.m_supportedExtensionsToOpen.push_back({ "Material Canvas Template", "materialcanvastemplate.azasset" });
        documentType.m_supportedExtensionsToOpen.push_back({ "Material Canvas", "materialcanvas.azasset" });
        documentType.m_supportedExtensionsToSave.push_back({ "Material Canvas", "materialcanvas.azasset" });

        // Currently using AnyAsset As a placeholder until proper asset types are created.
        documentType.m_supportedAssetTypesToCreate.insert(azrtti_typeid<AZ::RPI::AnyAsset>());

        // Using a blank template file to create a new document until UX and workflow can be revisited for creating new or empty documents.
        // However, there may be no need as this is an established pattern in other applications that provide multiple options and templates
        // to use as a starting point for a new document.
        documentType.m_defaultAssetIdToCreate = AtomToolsFramework::GetSettingsObject<AZ::Data::AssetId>(
            "/O3DE/Atom/MaterialCanvas/DefaultMaterialCanvasTemplateAsset",
            AZ::RPI::AssetUtils::GetAssetIdForProductPath("materialCanvas/blank.materialcanvastemplate.azasset"));
        return documentType;
    }

    AtomToolsFramework::DocumentObjectInfoVector MaterialCanvasDocument::GetObjectInfo() const
    {
        if (!IsOpen())
        {
            AZ_Error("MaterialCanvasDocument", false, "Document is not open.");
            return {};
        }

        AtomToolsFramework::DocumentObjectInfoVector objects;
        return objects;
    }

    bool MaterialCanvasDocument::Open(const AZStd::string& loadPath)
    {
        if (!AtomToolsDocument::Open(loadPath))
        {
            return false;
        }

        auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(m_absolutePath);
        if (!loadResult || !loadResult.GetValue().is<GraphModel::Graph>())
        {
            return OpenFailed();
        }

        // Cloning loaded data using the serialize context because the graph does not have a copy or move constructor
        AZ::SerializeContext* serializeContext = {};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(serializeContext, "Failed to acquire application serialize context.");

        GraphModel::GraphPtr graph;
        graph.reset(serializeContext->CloneObject(AZStd::any_cast<const GraphModel::Graph>(&loadResult.GetValue())));

        CreateGraph(graph);
        m_modified = false;

        GenerateDataFromGraph();
        return OpenSucceeded();
    }

    bool MaterialCanvasDocument::Save()
    {
        if (!AtomToolsDocument::Save())
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        if (!AZ::JsonSerializationUtils::SaveObjectToFile(m_graph.get(), m_savePathNormalized))
        {
            return SaveFailed();
        }

        m_modified = false;
        m_absolutePath = m_savePathNormalized;
        GenerateDataFromGraph();
        return SaveSucceeded();
    }

    bool MaterialCanvasDocument::SaveAsCopy(const AZStd::string& savePath)
    {
        if (!AtomToolsDocument::SaveAsCopy(savePath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        if (!AZ::JsonSerializationUtils::SaveObjectToFile(m_graph.get(), m_savePathNormalized))
        {
            return SaveFailed();
        }

        m_modified = false;
        m_absolutePath = m_savePathNormalized;
        GenerateDataFromGraph();
        return SaveSucceeded();
    }

    bool MaterialCanvasDocument::SaveAsChild(const AZStd::string& savePath)
    {
        if (!AtomToolsDocument::SaveAsChild(savePath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        if (!AZ::JsonSerializationUtils::SaveObjectToFile(m_graph.get(), m_savePathNormalized))
        {
            return SaveFailed();
        }

        m_modified = false;
        m_absolutePath = m_savePathNormalized;
        GenerateDataFromGraph();
        return SaveSucceeded();
    }

    bool MaterialCanvasDocument::IsOpen() const
    {
        return AtomToolsDocument::IsOpen() && m_graph && m_graphId.IsValid();
    }

    bool MaterialCanvasDocument::IsModified() const
    {
        return m_modified;
    }

    bool MaterialCanvasDocument::BeginEdit()
    {
        return true;
    }

    bool MaterialCanvasDocument::EndEdit()
    {
        auto undoState = m_graphStateForUndoRedo;

        RecordGraphState();
        auto redoState = m_graphStateForUndoRedo;

        if (undoState != redoState)
        {
            AddUndoRedoHistory(
                [this, undoState]() { RestoreGraphState(undoState); },
                [this, redoState]() { RestoreGraphState(redoState); });

            m_modified = true;
            AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
                m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentObjectInfoInvalidated, m_id);
            AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
                m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
        }
        return true;
    }

    GraphCanvas::GraphId MaterialCanvasDocument::GetGraphId() const
    {
        return m_graphId;
    }

    void MaterialCanvasDocument::Clear()
    {
        DestroyGraph();
        m_graphStateForUndoRedo.clear();
        m_modified = false;

        AtomToolsFramework::AtomToolsDocument::Clear();
    }

    bool MaterialCanvasDocument::ReopenRecordState()
    {
        return AtomToolsDocument::ReopenRecordState();
    }

    bool MaterialCanvasDocument::ReopenRestoreState()
    {
        return AtomToolsDocument::ReopenRestoreState();
    }

    void MaterialCanvasDocument::OnGraphModelRequestUndoPoint()
    {
        BeginEdit();
        EndEdit();
    }

    void MaterialCanvasDocument::OnGraphModelTriggerUndo()
    {
        Undo();
    }

    void MaterialCanvasDocument::OnGraphModelTriggerRedo()
    {
        Redo();
    }

    void MaterialCanvasDocument::RecordGraphState()
    {
        // Serialize the current graph to a byte stream so that it can be restored with undo redo operations.
        m_graphStateForUndoRedo.clear();
        AZ::IO::ByteContainerStream<decltype(m_graphStateForUndoRedo)> undoGraphStateStream(&m_graphStateForUndoRedo);
        AZ::Utils::SaveObjectToStream(undoGraphStateStream, AZ::ObjectStream::ST_BINARY, m_graph.get());
    }

    void MaterialCanvasDocument::RestoreGraphState(const AZStd::vector<AZ::u8>& graphState)
    {
        // Restore a version of the graph that was previously serialized to a byte stream
        m_graphStateForUndoRedo = graphState;
        AZ::IO::ByteContainerStream<decltype(m_graphStateForUndoRedo)> undoGraphStateStream(&m_graphStateForUndoRedo);

        GraphModel::GraphPtr graph = AZStd::make_shared<GraphModel::Graph>(m_graphContext);
        AZ::Utils::LoadObjectFromStreamInPlace(undoGraphStateStream, *graph.get());

        CreateGraph(graph);

        m_modified = true;
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentObjectInfoInvalidated, m_id);
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);

        GenerateDataFromGraph();
    }

    void MaterialCanvasDocument::CreateGraph(GraphModel::GraphPtr graph)
    {
        DestroyGraph();

        m_graph = graph;
        m_graph->PostLoadSetup(m_graphContext);
        RecordGraphState();

        // The graph controller will create all of the scene items on construction.
        GraphModelIntegration::GraphManagerRequestBus::Broadcast(
            &GraphModelIntegration::GraphManagerRequests::CreateGraphController, m_graphId, m_graph);
    }

    void MaterialCanvasDocument::DestroyGraph()
    {
        // The graph controller does not currently delete all of the scene items when it's destroyed.
        GraphModelIntegration::GraphManagerRequestBus::Broadcast(
            &GraphModelIntegration::GraphManagerRequests::DeleteGraphController, m_graphId);
        m_graph.reset();

        // This needs to be done whenever the graph is destroyed during undo and redo so that the previous version of the data is deleted. 
        GraphCanvas::GraphModelRequestBus::Event(m_graphId, &GraphCanvas::GraphModelRequests::RequestPushPreventUndoStateUpdate);
        GraphCanvas::SceneRequestBus::Event(m_graphId, &GraphCanvas::SceneRequests::ClearScene);
        GraphCanvas::GraphModelRequestBus::Event(m_graphId, &GraphCanvas::GraphModelRequests::RequestPopPreventUndoStateUpdate);
    }

    bool MaterialCanvasDocument::CompareNodeExecutionOrder(GraphModel::ConstNodePtr nodeA, GraphModel::ConstNodePtr nodeB) const
    {
        if (!nodeA || !nodeB)
        {
            return false;
        }

        if (nodeA == nodeB)
        {
            return true;
        }

        for (const auto& slotPair : nodeA->GetSlots())
        {
            if (slotPair.second->GetSlotDirection() == GraphModel::SlotDirection::Input)
            {
                for (const auto& connection : slotPair.second->GetConnections())
                {
                    if (CompareNodeExecutionOrder(connection->GetSourceNode(), nodeB))
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    AZStd::vector<GraphModel::ConstNodePtr> MaterialCanvasDocument::GetNodesInExecutionOrder() const
    {
        AZStd::vector<GraphModel::ConstNodePtr> nodes;
        nodes.reserve(m_graph->GetNodes().size());

        for (const auto& nodePair : m_graph->GetNodes())
        {
            nodes.push_back(nodePair.second);
        }

        AZStd::sort(
            nodes.begin(),
            nodes.end(),
            [this](GraphModel::ConstNodePtr nodeA, GraphModel::ConstNodePtr nodeB)
            {
                return !CompareNodeExecutionOrder(nodeA, nodeB);
            });

        return nodes;
    }

    bool MaterialCanvasDocument::GenerateDataFromGraph() const
    {
        // All of these slots and nodes will be visited to collect all of the uniuue include and template paths.
        AZStd::set<AZStd::string> includePaths;
        AZStd::set<AZStd::string> templatePaths;
        auto collectSettingsAsSet = [](const AtomToolsFramework::DynamicNodeSettingsMap& settings,
                                       const AZStd::string& settingName,
                                       AZStd::set<AZStd::string>& container)
        {
            if (auto settingsItr = settings.find(settingName); settingsItr != settings.end())
            {
                container.insert(settingsItr->second.begin(), settingsItr->second.end());
            }
        };

        // There's probably no reason to distinguish between function and class definitions. This could really be any globally defined
        // function, class, struct, define.
        AZStd::vector<AZStd::string> classDefinitions;
        AZStd::vector<AZStd::string> functionDefinitions;
        AZStd::vector<AZStd::string> instructions;
        auto collectSettingsAsVec = [](const AtomToolsFramework::DynamicNodeSettingsMap& settings,
                                       const AZStd::string& settingName,
                                       AZStd::vector<AZStd::string>& container)
        {
            if (auto settingsItr = settings.find(settingName); settingsItr != settings.end())
            {
                container.insert(container.end(), settingsItr->second.begin(), settingsItr->second.end());
            }
        };
        auto replaceStringsInVec =
            [](const AZStd::string& findText, const AZStd::string& replaceText, AZStd::vector<AZStd::string>& container)
        {
            for (auto& sourceText : container)
            {
                AZ::StringFunc::Replace(sourceText, findText.c_str(), replaceText.c_str());
            }
        };
        auto convertSlotTypeName = [](const AZStd::string& slotTypeName) -> AZStd::string
        {
            if (AZ::StringFunc::Equal(slotTypeName, "color"))
            {
                return "float4";
            }

            return slotTypeName;
        };
        auto convertSlotValue = [](const AZStd::any& slotValue) -> AZStd::string
        {
            if (auto v = AZStd::any_cast<const AZ::Color>(&slotValue))
            {
                return AZStd::string::format("float4(%g, %g, %g, %g)", v->GetR(), v->GetG(), v->GetB(), v->GetA());
            }
            if (auto v = AZStd::any_cast<const AZ::Vector4>(&slotValue))
            {
                return AZStd::string::format("float4(%g, %g, %g, %g)", v->GetX(), v->GetY(), v->GetZ(), v->GetW());
            }
            if (auto v = AZStd::any_cast<const AZ::Vector3>(&slotValue))
            {
                return AZStd::string::format("float3(%g, %g, %g)", v->GetX(), v->GetY(), v->GetZ());
            }
            if (auto v = AZStd::any_cast<const AZ::Vector2>(&slotValue))
            {
                return AZStd::string::format("float2(%g, %g, %g)", v->GetX(), v->GetY());
            }
            if (auto v = AZStd::any_cast<const float>(&slotValue))
            {
                return AZStd::string::format("%g", *v);
            }
            if (auto v = AZStd::any_cast<const int>(&slotValue))
            {
                return AZStd::string::format("%i", *v);
            }
            if (auto v = AZStd::any_cast<const unsigned int>(&slotValue))
            {
                return AZStd::string::format("%u", *v);
            }
            return AZStd::string();
        };

        AZStd::vector<GraphModel::ConstNodePtr> nodes = GetNodesInExecutionOrder();
        for (const auto& node : nodes)
        {
            if (auto dynamicNode = azrtti_cast<const AtomToolsFramework::DynamicNode*>(node.get()))
            {
                const auto& nodeConfig = dynamicNode->GetConfig();

                // Prescan to see if this node has any output connections
                // Generally speaking, we can ignore the node if there are no output connections but we're just trying to gather data now.
                bool hasOutputConnections = false;
                for (const auto& slotConfig : nodeConfig.m_outputSlots)
                {
                    auto slot = dynamicNode->GetSlot(slotConfig.m_name);
                    if (slot && !slot->GetConnections().empty())
                    {
                        hasOutputConnections = true;
                        break;
                    }
                }

                collectSettingsAsSet(nodeConfig.m_settings, "includePaths", includePaths);
                collectSettingsAsSet(nodeConfig.m_settings, "templatePaths", templatePaths);
                collectSettingsAsVec(nodeConfig.m_settings, "classDefinitions", classDefinitions);
                collectSettingsAsVec(nodeConfig.m_settings, "functionDefinitions", functionDefinitions);

                for (const auto& slotConfig : nodeConfig.m_propertySlots)
                {
                    collectSettingsAsSet(slotConfig.m_settings, "includePaths", includePaths);
                    collectSettingsAsSet(slotConfig.m_settings, "templatePaths", templatePaths);
                    collectSettingsAsVec(slotConfig.m_settings, "classDefinitions", classDefinitions);
                    collectSettingsAsVec(slotConfig.m_settings, "functionDefinitions", functionDefinitions);

                    // Write out any property slot data if there were any helpful connections
                    if (hasOutputConnections)
                    {
                        // Each instruction block needs to have name substitutions performed on it.
                        AZStd::vector<AZStd::string> instructionsForSlot;
                        collectSettingsAsVec(slotConfig.m_settings, "instructions", instructionsForSlot);

                        auto slot = dynamicNode->GetSlot(slotConfig.m_name);
                        replaceStringsInVec("NODEID", AZStd::string::format("node%u", node->GetId()), instructionsForSlot);
                        replaceStringsInVec("SLOTNAME", slot->GetName().c_str(), instructionsForSlot);
                        replaceStringsInVec("SLOTTYPE", convertSlotTypeName(slot->GetDataType()->GetDisplayName()), instructionsForSlot);
                        replaceStringsInVec("SLOTVALUE", convertSlotValue(slot->GetValue()), instructionsForSlot);
                        instructions.insert(instructions.end(), instructionsForSlot.begin(), instructionsForSlot.end());
                    }
                }

                for (const auto& slotConfig : nodeConfig.m_inputSlots)
                {
                    collectSettingsAsSet(slotConfig.m_settings, "includePaths", includePaths);
                    collectSettingsAsSet(slotConfig.m_settings, "templatePaths", templatePaths);
                    collectSettingsAsVec(slotConfig.m_settings, "classDefinitions", classDefinitions);
                    collectSettingsAsVec(slotConfig.m_settings, "functionDefinitions", functionDefinitions);

                    // There is currently no data to determine if an input property is utilized or not. We should be able to add metadata
                    // for that to the output slots.
                    // For now, we will always generate all input slot instructions.
                    // Each instruction block needs to have name substitutions performed on it.
                    AZStd::vector<AZStd::string> instructionsForSlot;
                    collectSettingsAsVec(slotConfig.m_settings, "instructions", instructionsForSlot);

                    auto slot = dynamicNode->GetSlot(slotConfig.m_name);
                    if (slot && !slot->GetConnections().empty())
                    {
                        for (const auto& connection : slot->GetConnections())
                        {
                            auto sourceSlot = connection->GetSourceSlot();
                            if (sourceSlot && sourceSlot->GetSlotDirection() == GraphModel::SlotDirection::Output)
                            {
                                replaceStringsInVec(
                                    "SLOTVALUE",
                                    AZStd::string::format("node%u_%s", sourceSlot->GetParentNode()->GetId(), sourceSlot->GetName().c_str()),
                                    instructionsForSlot);
                                break;
                            }
                        }
                    }

                    replaceStringsInVec("NODEID", AZStd::string::format("node%u", node->GetId()), instructionsForSlot);
                    replaceStringsInVec("SLOTNAME", slot->GetName().c_str(), instructionsForSlot);
                    replaceStringsInVec("SLOTTYPE", convertSlotTypeName(slot->GetDataType()->GetDisplayName()), instructionsForSlot);
                    replaceStringsInVec("SLOTVALUE", convertSlotValue(slot->GetValue()), instructionsForSlot);
                    instructions.insert(instructions.end(), instructionsForSlot.begin(), instructionsForSlot.end());
                }

                for (const auto& slotConfig : nodeConfig.m_outputSlots)
                {
                    collectSettingsAsSet(slotConfig.m_settings, "includePaths", includePaths);
                    collectSettingsAsSet(slotConfig.m_settings, "templatePaths", templatePaths);
                    collectSettingsAsVec(slotConfig.m_settings, "classDefinitions", classDefinitions);
                    collectSettingsAsVec(slotConfig.m_settings, "functionDefinitions", functionDefinitions);

                    // If an output slot has any connections assume that we're going to use any configured instructions
                    auto slot = dynamicNode->GetSlot(slotConfig.m_name);
                    if (slot && !slot->GetConnections().empty())
                    {
                        // Each instruction block needs to have name substitutions performed on it.
                        AZStd::vector<AZStd::string> instructionsForSlot;
                        collectSettingsAsVec(slotConfig.m_settings, "instructions", instructionsForSlot);

                        replaceStringsInVec("NODEID", AZStd::string::format("node%u", node->GetId()), instructionsForSlot);
                        replaceStringsInVec("SLOTNAME", slot->GetName().c_str(), instructionsForSlot);
                        replaceStringsInVec("SLOTTYPE", convertSlotTypeName(slot->GetDataType()->GetDisplayName()), instructionsForSlot);
                        replaceStringsInVec("SLOTVALUE", convertSlotValue(slot->GetValue()), instructionsForSlot);
                        instructions.insert(instructions.end(), instructionsForSlot.begin(), instructionsForSlot.end());
                    }
                }
            }
        }

        AZ_TracePrintf("MaterialCanvasDocument", "Dumping data scraped from traversing material graph.\n");

        for (const auto& node : nodes)
        {
            AZ_TracePrintf("MaterialCanvasDocument", "Sorted node: %s\n", node->GetTitle());
        }

        for (const auto& templatePath : templatePaths)
        {
            AZ_TracePrintf("MaterialCanvasDocument", "templatePath: %s\n", templatePath.c_str());
        }

        for (const auto& includePath : includePaths)
        {
            AZ_TracePrintf("MaterialCanvasDocument", "includePath: %s\n", includePath.c_str());
        }

        for (const auto& classDefinition : classDefinitions)
        {
            AZ_TracePrintf("MaterialCanvasDocument", "classDefinition: %s\n", classDefinition.c_str());
        }

        for (const auto& functionDefinition : functionDefinitions)
        {
            AZ_TracePrintf("MaterialCanvasDocument", "functionDefinition: %s\n", functionDefinition.c_str());
        }

        for (const auto& instruction : instructions)
        {
            AZ_TracePrintf("MaterialCanvasDocument", "instruction: %s\n", instruction.c_str());
        }

        return false;
    }
} // namespace MaterialCanvas
