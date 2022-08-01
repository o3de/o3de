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
#include <AzCore/Utils/Utils.h>
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

        CompileGraph();
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
        CompileGraph();
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
        CompileGraph();
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
        CompileGraph();
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
            CompileGraph();
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

    const AZStd::vector<AZStd::string>& MaterialCanvasDocument::GetGeneratedFilePaths() const
    {
        return m_generatedFiles;
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
        CompileGraph();
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentObjectInfoInvalidated, m_id);
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
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

    bool MaterialCanvasDocument::CompileGraph() const
    {
        m_generatedFiles.clear();

        // All slots and nodes will be visited to collect all of the unique include and template paths.
        AZStd::set<AZStd::string> includePaths;
        AZStd::set<AZStd::string> templatePaths;

        // Build a unique set of settings found on a node or slot configuration.
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

        // This is a complete list of all the instructions collected from settings found in each node.
        // The set of instructions will need to be rebuilt for every instance of a generated code block based on the context and outputs
        // expected from that block. This will be updated in another pass so that instruction can be built by filtering the set of nodes
        // down to only those that will be used to generate the outputs listed in each block.
        AZStd::vector<AZStd::string> instructions;

        // Build an accumulated list of settings found on a node or slot configuration.
        auto collectSettingsAsVec = [](const AtomToolsFramework::DynamicNodeSettingsMap& settings,
                                       const AZStd::string& settingName,
                                       AZStd::vector<AZStd::string>& container)
        {
            if (auto settingsItr = settings.find(settingName); settingsItr != settings.end())
            {
                container.insert(container.end(), settingsItr->second.begin(), settingsItr->second.end());
            }
        };

        // Perform string substitutions on a container of strings.
        auto replaceStringsInVec =
            [](const AZStd::string& findText, const AZStd::string& replaceText, AZStd::vector<AZStd::string>& container)
        {
            for (auto& sourceText : container)
            {
                AZ::StringFunc::Replace(sourceText, findText.c_str(), replaceText.c_str());
            }
        };

        // Slots with special types, like color, need to be converted into a type compatible with shader code.
        auto convertSlotTypeName = [](const AZStd::string& slotTypeName) -> AZStd::string
        {
            if (AZ::StringFunc::Equal(slotTypeName, "color"))
            {
                return "float4";
            }

            return slotTypeName;
        };

        // Disconnected input and property slots need to have their values converted into a format that can be injected into the shader.
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
                return AZStd::string::format("float2(%g, %g)", v->GetX(), v->GetY());
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

        // Gather all of the common settings usually associated with a material canvas note to hand slot.
        auto collectCommonSettings = [&](const AtomToolsFramework::DynamicNodeSettingsMap& settings)
        {
            collectSettingsAsSet(settings, "includePaths", includePaths);
            collectSettingsAsSet(settings, "templatePaths", templatePaths);
            collectSettingsAsVec(settings, "classDefinitions", classDefinitions);
            collectSettingsAsVec(settings, "functionDefinitions", functionDefinitions);
        };

        // Each instruction block needs to have name substitutions performed on it. Variable names need to be unique per
        // node and will be prepended with the node ID. Slot types and values will be determined based on connections and
        // then substituted.
        auto updateAndAddSlotInstructions =
            [&](GraphModel::ConstNodePtr node, GraphModel::ConstSlotPtr slot, AZStd::vector<AZStd::string>& instructionsForSlot)
        {
            replaceStringsInVec("NODEID", AZStd::string::format("node%u", node->GetId()), instructionsForSlot);
            replaceStringsInVec("SLOTNAME", slot->GetName().c_str(), instructionsForSlot);
            replaceStringsInVec("SLOTTYPE", convertSlotTypeName(slot->GetDataType()->GetDisplayName()), instructionsForSlot);
            replaceStringsInVec("SLOTVALUE", convertSlotValue(slot->GetValue()), instructionsForSlot);
            instructions.insert(instructions.end(), instructionsForSlot.begin(), instructionsForSlot.end());
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

                collectCommonSettings(nodeConfig.m_settings);

                for (const auto& slotConfig : nodeConfig.m_propertySlots)
                {
                    collectCommonSettings(slotConfig.m_settings);

                    // Write out any property slot data if there were any connections
                    if (hasOutputConnections)
                    {
                        auto slot = dynamicNode->GetSlot(slotConfig.m_name);
                        AZStd::vector<AZStd::string> instructionsForSlot;
                        collectSettingsAsVec(slotConfig.m_settings, "instructions", instructionsForSlot);
                        updateAndAddSlotInstructions(node, slot, instructionsForSlot);
                    }
                }

                for (const auto& slotConfig : nodeConfig.m_inputSlots)
                {
                    collectCommonSettings(slotConfig.m_settings);

                    // For now, this will always generate all input slot instructions but can potentially be optimized to eliminate
                    // instructions that result in temporary variables that just pass through to other variables.
                    auto slot = dynamicNode->GetSlot(slotConfig.m_name);
                    AZStd::vector<AZStd::string> instructionsForSlot;
                    collectSettingsAsVec(slotConfig.m_settings, "instructions", instructionsForSlot);

                    // Input slots will replace the value with the name of the variable from the connected slot if one is set.
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

                    updateAndAddSlotInstructions(node, slot, instructionsForSlot);
                }

                for (const auto& slotConfig : nodeConfig.m_outputSlots)
                {
                    collectCommonSettings(slotConfig.m_settings);

                    // If an output slot has any connections assume that we're going to use any configured instructions
                    auto slot = dynamicNode->GetSlot(slotConfig.m_name);
                    if (slot && !slot->GetConnections().empty())
                    {
                        AZStd::vector<AZStd::string> instructionsForSlot;
                        collectSettingsAsVec(slotConfig.m_settings, "instructions", instructionsForSlot);
                        updateAndAddSlotInstructions(node, slot, instructionsForSlot);
                    }
                }

                // Gather and insert instructions provided by the node.
                // We might need separate blocks of instructions that can be processed before or after the slots are processed.
                AZStd::vector<AZStd::string> instructionsForNode;
                collectSettingsAsVec(nodeConfig.m_settings, "instructions", instructionsForNode);
                replaceStringsInVec("NODEID", AZStd::string::format("node%u", node->GetId()), instructionsForNode);
                instructions.insert(instructions.end(), instructionsForNode.begin(), instructionsForNode.end());
            }
        }

        AZ_TracePrintf("MaterialCanvasDocument", "Dumping data scraped from traversing material graph.\n");

        for (const auto& node : nodes)
        {
            AZ_UNUSED(node);
            AZ_TracePrintf("MaterialCanvasDocument", "Sorted node: %s\n", node->GetTitle());
        }

        // The document name will be used to replace file and symbol names in template files
        AZStd::string documentName;
        AZ::StringFunc::Path::GetFullFileName(m_absolutePath.c_str(), documentName);
        AZ::StringFunc::Replace(documentName, ".materialcanvas.azasset", "");

        // Sanitize the document name to remove any illegal characters that could not be used as symbols in generated code
        AZ::StringFunc::Replace(documentName, "-", "_");
        documentName = AZ::RPI::AssetUtils::SanitizeFileName(documentName);

        for (const auto& templatePath : templatePaths)
        {
            AZ_TracePrintf("MaterialCanvasDocument", "templatePath: %s\n", templatePath.c_str());

            // Remove any aliases to resolve the absolute path to the template file
            const AZStd::string resolvedPath = AtomToolsFramework::GetPathWithoutAlias(templatePath);

            // Load the template file so that we can do symbol substitution and inject any code or data
            if(auto result = AZ::Utils::ReadFile(resolvedPath))
            {
                // Create the path and file name for the file generated from the template using the document path and name
                AZStd::string outputFullName;
                AZ::StringFunc::Path::GetFullFileName(resolvedPath.c_str(), outputFullName);
                AZStd::string outputPath = m_absolutePath;
                AZ::StringFunc::Path::ReplaceFullName(outputPath, outputFullName.c_str());
                AZ::StringFunc::Replace(outputPath, "MaterialGraphName", documentName.c_str());
                AZ::StringFunc::Replace(outputPath, ".template", "");

                // Tokenize lines from the template file so that we can search for injection points by line
                AZStd::vector<AZStd::string> lines;
                AZ::StringFunc::Tokenize(result.GetValue(), lines, '\n', true, true);

                // Substitute all references to the generic graph name with the document name
                replaceStringsInVec("MaterialGraphName", documentName, lines);

                // Locate the first injection point for instructions so that we can remove pre-existing instructions and insert the
                // generated instructions.
                // The begin block will eventually list the variable names that it expects code to be generated for.
                // Then code will be generated for each block, only using the nodes connected to the expected variables.
                auto codeGenBeginItr = AZStd::find_if(lines.begin(), lines.end(), [](const AZStd::string& line) {
                    return AZ::StringFunc::Contains(line, "GENERATED_INSTRUCTIONS_BEGIN");
                });

                while (codeGenBeginItr != lines.end())
                {
                    // We have to insert one instruction at a time because the implementation of vector does not include a standard range
                    // insert that returns an iterator
                    for (const auto& instruction : instructions)
                    {
                        ++codeGenBeginItr;
                        codeGenBeginItr = lines.insert(codeGenBeginItr, instruction);
                    }
                    ++codeGenBeginItr;

                    // From the last line that was inserted, locate the end of the instruction insertion block 
                    auto codeGenEndItr = AZStd::find_if(codeGenBeginItr, lines.end(), [](const AZStd::string& line) {
                        return AZ::StringFunc::Contains(line, "GENERATED_INSTRUCTIONS_END");
                    });

                    // Erase any pre-existing instructions the template might have had between the begin and end blocks
                    codeGenEndItr = lines.erase(codeGenBeginItr, codeGenEndItr);

                    // Search for another instruction insertion point
                    codeGenBeginItr = AZStd::find_if(codeGenEndItr, lines.end(), [](const AZStd::string& line) {
                        return AZ::StringFunc::Contains(line, "GENERATED_INSTRUCTIONS_BEGIN");
                    });
                }

                // After everything has been substituted and inserted, save the file.
                AZStd::string output;
                AZ::StringFunc::Join(output, lines, '\n');
                AZ::Utils::WriteFile(output, outputPath);
                m_generatedFiles.push_back(outputPath);
            }
        }

        for (const auto& includePath : includePaths)
        {
            AZ_UNUSED(includePath);
            AZ_TracePrintf("MaterialCanvasDocument", "includePath: %s\n", includePath.c_str());
        }

        for (const auto& classDefinition : classDefinitions)
        {
            AZ_UNUSED(classDefinition);
            AZ_TracePrintf("MaterialCanvasDocument", "classDefinition: %s\n", classDefinition.c_str());
        }

        for (const auto& functionDefinition : functionDefinitions)
        {
            AZ_UNUSED(functionDefinition);
            AZ_TracePrintf("MaterialCanvasDocument", "functionDefinition: %s\n", functionDefinition.c_str());
        }

        for (const auto& instruction : instructions)
        {
            AZ_UNUSED(instruction);
            AZ_TracePrintf("MaterialCanvasDocument", "instruction: %s\n", instruction.c_str());
        }

        return true;
    }
} // namespace MaterialCanvas

