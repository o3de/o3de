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
#include <AtomToolsFramework/DynamicNode/DynamicNodeUtil.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Document/MaterialCanvasDocument.h>
#include <Document/MaterialCanvasDocumentNotificationBus.h>
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
                ->Event("GetGeneratedFilePaths", &MaterialCanvasDocumentRequests::GetGeneratedFilePaths)
                ->Event("GetGraphName", &MaterialCanvasDocumentRequests::GetGraphName)
                ->Event("CompileGraph", &MaterialCanvasDocumentRequests::CompileGraph)
                ->Event("QueueCompileGraph", &MaterialCanvasDocumentRequests::QueueCompileGraph)
                ->Event("IsCompileGraphQueued", &MaterialCanvasDocumentRequests::IsCompileGraphQueued);
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
            auto graphContext = AZStd::make_shared<GraphModel::GraphContext>("Material Canvas", ".materialcanvas.azasset", registeredDataTypes);
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

        QueueCompileGraph();
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
        QueueCompileGraph();
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
        QueueCompileGraph();
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
        QueueCompileGraph();
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
            QueueCompileGraph();
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
        m_compileGraphQueued = false;
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
        QueueCompileGraph();
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

    AZStd::string MaterialCanvasDocument::GetGraphName() const
    {
        AZStd::string documentName;
        AZ::StringFunc::Path::GetFullFileName(m_absolutePath.c_str(), documentName);
        AZ::StringFunc::Replace(documentName, ".materialcanvas.azasset", "");

        // Sanitize the document name to remove any illegal characters that could not be used as symbols in generated code
        AZ::StringFunc::Replace(documentName, "-", "_");

        return AZ::RPI::AssetUtils::SanitizeFileName(documentName);
    }

    AZStd::string MaterialCanvasDocument::GetOutputPathFromTemplatePath(const AZStd::string& templateInputPath) const
    {
        AZStd::string templateInputFileName;
        AZ::StringFunc::Path::GetFullFileName(templateInputPath.c_str(), templateInputFileName);
        AZ::StringFunc::Replace(templateInputFileName, ".template", "");

        AZStd::string templateOuputPath = m_absolutePath;
        AZ::StringFunc::Path::ReplaceFullName(templateOuputPath, templateInputFileName.c_str());

        AZ::StringFunc::Replace(templateOuputPath, "MaterialGraphName", GetGraphName().c_str());

        return templateOuputPath;
    }

    void MaterialCanvasDocument::ReplaceStringsInContainer(
        const AZStd::string& findText, const AZStd::string& replaceText, AZStd::vector<AZStd::string>& container) const
    {
        for (auto& sourceText : container)
        {
            AZ::StringFunc::Replace(sourceText, findText.c_str(), replaceText.c_str());
        }
    }

    AZStd::string MaterialCanvasDocument::ConvertSlotTypeToAZSL(const AZStd::string& slotTypeName) const
    {
        if (AZ::StringFunc::Equal(slotTypeName, "color"))
        {
            return "float4";
        }

        return slotTypeName;
    }

    AZStd::string MaterialCanvasDocument::ConvertSlotValueToAZSL(const AZStd::any& slotValue) const
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
    }

    AZStd::vector<AZStd::string> MaterialCanvasDocument::GetInstructionsFromSlot(
        GraphModel::ConstNodePtr node, const AtomToolsFramework::DynamicNodeSlotConfig& slotConfig) const
    {
        AZStd::vector<AZStd::string> instructionsForSlot;

        auto slot = node->GetSlot(slotConfig.m_name);
        if (slot && (slot->GetSlotDirection() != GraphModel::SlotDirection::Output || !slot->GetConnections().empty()))
        {
            AtomToolsFramework::CollectDynamicNodeSettings(slotConfig.m_settings, "instructions", instructionsForSlot);

            // Input slots will replace the value with the name of the variable from the connected slot if one is set.
            for (const auto& connection : slot->GetConnections())
            {
                auto sourceSlot = connection->GetSourceSlot();
                auto targetSlot = connection->GetTargetSlot();
                if (sourceSlot && targetSlot && sourceSlot != targetSlot && sourceSlot != slot)
                {
                    ReplaceStringsInContainer(
                        "SLOTVALUE",
                        AZStd::string::format("node%u_%s", sourceSlot->GetParentNode()->GetId(), sourceSlot->GetName().c_str()),
                        instructionsForSlot);
                    break;
                }
            }

            ReplaceStringsInContainer("NODEID", AZStd::string::format("node%u", node->GetId()), instructionsForSlot);
            ReplaceStringsInContainer("SLOTNAME", slot->GetName().c_str(), instructionsForSlot);
            ReplaceStringsInContainer("SLOTTYPE", ConvertSlotTypeToAZSL(slot->GetDataType()->GetDisplayName()), instructionsForSlot);
            ReplaceStringsInContainer("SLOTVALUE", ConvertSlotValueToAZSL(slot->GetValue()), instructionsForSlot);
        }

        return instructionsForSlot;
    }

    bool MaterialCanvasDocument::ShouldUseInstructionsFromInputNode(
        GraphModel::ConstNodePtr outputNode, GraphModel::ConstNodePtr inputNode, const AZStd::vector<AZStd::string>& inputSlotNames) const
    {
        if (inputNode == outputNode)
        {
            return true;
        }

        for (const auto& inputSlotName : inputSlotNames)
        {
            if (const auto slot = outputNode->GetSlot(inputSlotName))
            {
                if (slot->GetSlotDirection() == GraphModel::SlotDirection::Input)
                {
                    for (const auto& connection : slot->GetConnections())
                    {
                        if (connection->GetSourceNode() == inputNode || connection->GetSourceNode()->HasInputConnectionFromNode(inputNode))
                        {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    AZStd::vector<GraphModel::ConstNodePtr> MaterialCanvasDocument::GetInstructionNodesInExecutionOrder(
        GraphModel::ConstNodePtr outputNode, const AZStd::vector<AZStd::string>& inputSlotNames) const
    {
        AZStd::vector<GraphModel::ConstNodePtr> sortedNodes;
        sortedNodes.reserve(m_graph->GetNodes().size());

        for (const auto& nodePair : m_graph->GetNodes())
        {
            const auto& inputNode = nodePair.second;
            if (ShouldUseInstructionsFromInputNode(outputNode, inputNode, inputSlotNames))
            {
                sortedNodes.push_back(inputNode);
            }
        }

        AZStd::sort(
            sortedNodes.begin(),
            sortedNodes.end(),
            [](GraphModel::ConstNodePtr nodeA, GraphModel::ConstNodePtr nodeB)
            {
                return nodeA != nodeB && nodeA->HasOutputConnectionToNode(nodeB);
            });

        return sortedNodes;
    }

    AZStd::vector<AZStd::string> MaterialCanvasDocument::GetInstructionsFromConnectedNodes(
        GraphModel::ConstNodePtr outputNode, const AZStd::vector<AZStd::string>& inputSlotNames) const
    {
        AZStd::vector<AZStd::string> instructions;

        for (const auto& inputNode : GetInstructionNodesInExecutionOrder(outputNode, inputSlotNames))
        {
            auto dynamicNode = azrtti_cast<const AtomToolsFramework::DynamicNode*>(inputNode.get());
            if (!dynamicNode)
            {
                continue;
            }

            const auto& nodeConfig = dynamicNode->GetConfig();

            for (const auto& slotConfig : nodeConfig.m_propertySlots)
            {
                const auto& instructionsForSlot = GetInstructionsFromSlot(inputNode, slotConfig);
                instructions.insert(instructions.end(), instructionsForSlot.begin(), instructionsForSlot.end());
            }

            for (const auto& slotConfig : nodeConfig.m_inputSlots)
            {
                // If the current node is also the output node, do not generate instructions unless it is for one of the expected input
                // slots.
                if (inputNode == outputNode &&
                    AZStd::find(inputSlotNames.begin(), inputSlotNames.end(), slotConfig.m_name) == inputSlotNames.end())
                {
                    continue;
                }

                const auto& instructionsForSlot = GetInstructionsFromSlot(inputNode, slotConfig);
                instructions.insert(instructions.end(), instructionsForSlot.begin(), instructionsForSlot.end());
            }

            for (const auto& slotConfig : nodeConfig.m_outputSlots)
            {
                const auto& instructionsForSlot = GetInstructionsFromSlot(inputNode, slotConfig);
                instructions.insert(instructions.end(), instructionsForSlot.begin(), instructionsForSlot.end());
            }

            // Gather and insert instructions provided by the node.
            // We might need separate blocks of instructions that can be processed before or after the slots are processed.
            AZStd::vector<AZStd::string> instructionsForNode;
            AtomToolsFramework::CollectDynamicNodeSettings(nodeConfig.m_settings, "instructions", instructionsForNode);
            ReplaceStringsInContainer("NODEID", AZStd::string::format("node%u", inputNode->GetId()), instructionsForNode);
            instructions.insert(instructions.end(), instructionsForNode.begin(), instructionsForNode.end());
        }

        return instructions;
    }

    void MaterialCanvasDocument::ReplaceLinesInTemplateBlock(
        const AZStd::string& blockBeginToken,
        const AZStd::string& blockEndToken,
        const LineGenerationFn& lineGenerationFn,
        AZStd::vector<AZStd::string>& templateLines) const
    {
        auto blockBeginItr = AZStd::find_if(
            templateLines.begin(),
            templateLines.end(),
            [&blockBeginToken](const AZStd::string& line)
            {
                return AZ::StringFunc::Contains(line, blockBeginToken);
            });

        while (blockBeginItr != templateLines.end())
        {
            AZ_TracePrintf("MaterialCanvasDocument", "*blockBegin: %s\n", (*blockBeginItr).c_str());

            // We have to insert one line at a time because AZStd::vector does not include a standard
            // range insert that returns an iterator
            const auto& linesToInsert = lineGenerationFn(*blockBeginItr);
            for (const auto& lineToInsert : linesToInsert)
            {
                ++blockBeginItr;
                blockBeginItr = templateLines.insert(blockBeginItr, lineToInsert);
                AZ_TracePrintf("MaterialCanvasDocument", "lineToInsert: %s\n", lineToInsert.c_str());
            }

            if (linesToInsert.empty())
            {
                AZ_TracePrintf("MaterialCanvasDocument", "Nothing was generated. This block will remain unmodified.\n");
            }

            ++blockBeginItr;

            // From the last line that was inserted, locate the end of the insertion block
            auto blockEndItr = AZStd::find_if(
                blockBeginItr,
                templateLines.end(),
                [&blockEndToken](const AZStd::string& line)
                {
                    return AZ::StringFunc::Contains(line, blockEndToken);
                });

            AZ_TracePrintf("MaterialCanvasDocument", "*blockEnd: %s\n", (*blockEndItr).c_str());

            if (!linesToInsert.empty())
            {
                // If any new lines were inserted, erase pre-existing lines the template might have had between the begin and end blocks
                blockEndItr = templateLines.erase(blockBeginItr, blockEndItr);
            }

            // Search for another insertion point
            blockBeginItr = AZStd::find_if(
                blockEndItr,
                templateLines.end(),
                [&blockBeginToken](const AZStd::string& line)
                {
                    return AZ::StringFunc::Contains(line, blockBeginToken);
                });
        }
    }

    bool MaterialCanvasDocument::CompileGraph() const
    {
        m_compileGraphQueued = false;
        m_generatedFiles.clear();

        if (!IsOpen())
        {
            return false;
        }

        // All slots and nodes will be visited to collect all of the unique include paths.
        AZStd::set<AZStd::string> includePaths;

        // There's probably no reason to distinguish between function and class definitions.
        // This could really be any globally defined function, class, struct, define.
        AZStd::vector<AZStd::string> classDefinitions;
        AZStd::vector<AZStd::string> functionDefinitions;

        AZ_TracePrintf("MaterialCanvasDocument", "Dumping data scraped from traversing material graph.\n");

        // Traverse all graph nodes and slots to collect global settings like include files and class definitions
        for (const auto& nodePair : m_graph->GetNodes())
        {
            const auto& currentNode = nodePair.second;

            if (auto dynamicNode = azrtti_cast<const AtomToolsFramework::DynamicNode*>(currentNode.get()))
            {
                AtomToolsFramework::VisitDynamicNodeSettings(
                    dynamicNode->GetConfig(),
                    [&](const AtomToolsFramework::DynamicNodeSettingsMap& settings)
                    {
                        AtomToolsFramework::CollectDynamicNodeSettings(settings, "includePaths", includePaths);
                        AtomToolsFramework::CollectDynamicNodeSettings(settings, "classDefinitions", classDefinitions);
                        AtomToolsFramework::CollectDynamicNodeSettings(settings, "functionDefinitions", functionDefinitions);
                    });
            }
        }

        // Traverse all graph nodes and slots searching for settings to generate files from templates
        for (const auto& nodePair : m_graph->GetNodes())
        {
            const auto& currentNode = nodePair.second;

            // Visit all of the settings for this node to collect template paths for files that need to be generated
            AZStd::set<AZStd::string> templatePaths;

            if (auto dynamicNode = azrtti_cast<const AtomToolsFramework::DynamicNode*>(currentNode.get()))
            {
                AtomToolsFramework::VisitDynamicNodeSettings(
                    dynamicNode->GetConfig(),
                    [&](const AtomToolsFramework::DynamicNodeSettingsMap& settings)
                    {
                        AtomToolsFramework::CollectDynamicNodeSettings(settings, "templatePaths", templatePaths);
                    });
            }

            // Attempt to resolve every template file, replacing tokens, injecting lines, updating settings, and outputting the final result
            for (const auto& templatePath : templatePaths)
            {
                AZ_TracePrintf("MaterialCanvasDocument", "templatePath: %s\n", templatePath.c_str());

                // Remove any aliases to resolve the absolute path to the template file
                const AZStd::string templateInputPath = AtomToolsFramework::GetPathWithoutAlias(templatePath);

                // Attempt to load the template file to do symbol substitution and inject any code or data
                if (auto result = AZ::Utils::ReadFile(templateInputPath))
                {
                    // Tokenize the entire template file into individual lines that can be evaluated, removed, replaced, and have content
                    // injected between them
                    AZStd::vector<AZStd::string> templateLines;
                    AZ::StringFunc::Tokenize(result.GetValue(), templateLines, '\n', true, true);

                    // Substitute all references to the placeholder graph name with one generated from the document name
                    ReplaceStringsInContainer("MaterialGraphName", GetGraphName(), templateLines);

                    // Inject include files found while traversing the graph into any include file blocks in the template.
                    ReplaceLinesInTemplateBlock(
                        "O3DE_GENERATED_INCLUDES_BEGIN",
                        "O3DE_GENERATED_INCLUDES_END",
                        [&includePaths]([[maybe_unused]] const AZStd::string& blockHeader)
                        {
                            // Include file paths will need to be specified as or converted to include statements.
                            return AZStd::vector<AZStd::string>(includePaths.begin(), includePaths.end());
                        },
                        templateLines);

                    // Inject class definitions found while traversing the graph.
                    ReplaceLinesInTemplateBlock(
                        "O3DE_GENERATED_CLASSES_BEGIN",
                        "O3DE_GENERATED_CLASSES_END",
                        [&classDefinitions]([[maybe_unused]] const AZStd::string& blockHeader)
                        {
                            return classDefinitions;
                        },
                        templateLines);

                    // Inject function definitions found while traversing the graph.
                    ReplaceLinesInTemplateBlock(
                        "O3DE_GENERATED_FUNCTIONS_BEGIN",
                        "O3DE_GENERATED_FUNCTIONS_END",
                        [&functionDefinitions]([[maybe_unused]] const AZStd::string& blockHeader)
                        {
                            return functionDefinitions;
                        },
                        templateLines);

                    // Inject shader code instructions stitched together by traversing the graph from each of the input slots on the current
                    // node. The O3DE_GENERATED_INSTRUCTIONS_BEGIN marker will be followed by a list of input slot names corresponding to
                    // required variables in the shader. Instructions will only be generated for the current node and nodes connected to the
                    // specified inputs. This will allow multiple O3DE_GENERATED_INSTRUCTIONS blocks with different inputs to be specified in
                    // multiple locations across multiple files from a single graph.
                    ReplaceLinesInTemplateBlock(
                        "O3DE_GENERATED_INSTRUCTIONS_BEGIN",
                        "O3DE_GENERATED_INSTRUCTIONS_END",
                        [&]([[maybe_unused]] const AZStd::string& blockHeader)
                        {
                            AZStd::vector<AZStd::string> inputSlotNames;
                            AZ::StringFunc::Tokenize(blockHeader, inputSlotNames, ";:, \t\n\r\\/", false, false);
                            return GetInstructionsFromConnectedNodes(currentNode, inputSlotNames);
                        },
                        templateLines);

                    // After everything has been substituted and inserted, save the file.
                    AZStd::string templateOuputPathText;
                    AZ::StringFunc::Join(templateOuputPathText, templateLines, '\n');

                    const AZStd::string templateOuputPath = GetOutputPathFromTemplatePath(templateInputPath);
                    AZ::Utils::WriteFile(templateOuputPathText, templateOuputPath);
                    m_generatedFiles.push_back(templateOuputPath);
                }
            }
        }

        MaterialCanvasDocumentNotificationBus::Event(
            m_toolId, &MaterialCanvasDocumentNotificationBus::Events::OnCompileGraphCompleted, m_id);
        return true;
    }

    void MaterialCanvasDocument::QueueCompileGraph() const
    {
        if (IsOpen() && !m_compileGraphQueued)
        {
            m_compileGraphQueued = true;
            AZ::SystemTickBus::QueueFunction([id = m_id](){
                MaterialCanvasDocumentRequestBus::Event(id, &MaterialCanvasDocumentRequestBus::Events::CompileGraph);
            });
        }
    }

    bool MaterialCanvasDocument::IsCompileGraphQueued() const
    {
        return m_compileGraphQueued;
    }
} // namespace MaterialCanvas
