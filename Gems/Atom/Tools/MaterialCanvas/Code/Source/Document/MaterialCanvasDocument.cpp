/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/SamplerState.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/DynamicNode/DynamicNode.h>
#include <AtomToolsFramework/DynamicNode/DynamicNodeManagerRequestBus.h>
#include <AtomToolsFramework/DynamicNode/DynamicNodeUtil.h>
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/string/regex.h>
#include <Document/MaterialCanvasDocument.h>
#include <Document/MaterialCanvasDocumentNotificationBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphModel/Model/Connection.h>

namespace MaterialCanvas
{
    namespace
    {
        bool IsCompileLoggingEnabled()
        {
            return AtomToolsFramework::GetSettingsValue("/O3DE/Atom/MaterialCanvasDocument/CompileLoggingEnabled", false);
        }
    } // namespace

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
                ->Event("GetGraph", &MaterialCanvasDocumentRequests::GetGraph)
                ->Event("GetGraphId", &MaterialCanvasDocumentRequests::GetGraphId)
                ->Event("GetGraphName", &MaterialCanvasDocumentRequests::GetGraphName)
                ->Event("GetGeneratedFilePaths", &MaterialCanvasDocumentRequests::GetGeneratedFilePaths)
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
        AZ_Assert(m_graphContext, "Graph context must be valid in order to create a graph document.");

        // Creating the scene entity and graph for this document. This may end up moving to the view if we can have the document only store
        // minimal material graph specific data that can be transformed into a graph canvas graph in the view and back. That abstraction
        // would help maintain a separation between the serialized data and the UI for rendering and interacting with the graph. This would
        // also help establish a mediator pattern for other node based tools that want to visualize their data or documents as a graph. My
        // understanding is that graph model will help with this.
        m_graph = AZStd::make_shared<GraphModel::Graph>(m_graphContext);
        AZ_Assert(m_graph, "Failed to create graph object.");

        GraphModelIntegration::GraphManagerRequestBus::BroadcastResult(
            m_sceneEntity, &GraphModelIntegration::GraphManagerRequests::CreateScene, m_graph, m_toolId);
        AZ_Assert(m_sceneEntity, "Failed to create graph scene entity.");

        m_graphId = m_sceneEntity->GetId();
        AZ_Assert(m_graphId.IsValid(), "Graph scene entity ID is not valid.");

        RecordGraphState();

        // Listen for GraphController notifications on the new graph.
        GraphModelIntegration::GraphControllerNotificationBus::Handler::BusConnect(m_graphId);

        GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_graphId);

        MaterialCanvasDocumentRequestBus::Handler::BusConnect(m_id);
    }

    MaterialCanvasDocument::~MaterialCanvasDocument()
    {
        MaterialCanvasDocumentRequestBus::Handler::BusDisconnect();
        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
        GraphModelIntegration::GraphControllerNotificationBus::Handler::BusDisconnect();

        DestroyGraph();

        m_graphId = GraphCanvas::GraphId();
        delete m_sceneEntity;
        m_sceneEntity = {};
    }

    AtomToolsFramework::DocumentTypeInfo MaterialCanvasDocument::BuildDocumentTypeInfo()
    {
        // Setting up placeholder document type info and extensions.
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
        AtomToolsFramework::DocumentObjectInfoVector objects = AtomToolsDocument::GetObjectInfo();
        objects.reserve(objects.size() + m_groups.size());

        for (const auto& group : m_groups)
        {
            if (!group->m_properties.empty())
            {
                AtomToolsFramework::DocumentObjectInfo objectInfo;
                objectInfo.m_visible = group->m_visible;
                objectInfo.m_name = group->m_name;
                objectInfo.m_displayName = group->m_displayName;
                objectInfo.m_description = group->m_description;
                objectInfo.m_objectType = azrtti_typeid<AtomToolsFramework::DynamicPropertyGroup>();
                objectInfo.m_objectPtr = const_cast<AtomToolsFramework::DynamicPropertyGroup*>(group.get());
                objectInfo.m_nodeIndicatorFunction = [](const AzToolsFramework::InstanceDataNode* /*node*/)
                {
                    // There are currently no indicators for material canvas nodes.
                    return nullptr;
                };
                objects.emplace_back(AZStd::move(objectInfo));
            }
        }

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

        m_modified = false;
        CreateGraph(graph);
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

        AZ_Error("MaterialCanvasDocument", m_graph, "Attempting to save invalid graph object.");
        if (!m_graph || !AZ::JsonSerializationUtils::SaveObjectToFile(m_graph.get(), m_savePathNormalized))
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

        AZ_Error("MaterialCanvasDocument", m_graph, "Attempting to save invalid graph object.");
        if (!m_graph || !AZ::JsonSerializationUtils::SaveObjectToFile(m_graph.get(), m_savePathNormalized))
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

        AZ_Error("MaterialCanvasDocument", m_graph, "Attempting to save invalid graph object. ");
        if (!m_graph || !AZ::JsonSerializationUtils::SaveObjectToFile(m_graph.get(), m_savePathNormalized))
        {
            return SaveFailed();
        }

        m_modified = false;
        m_absolutePath = m_savePathNormalized;
        QueueCompileGraph();
        return SaveSucceeded();
    }

    bool MaterialCanvasDocument::IsModified() const
    {
        return m_modified;
    }

    bool MaterialCanvasDocument::BeginEdit()
    {
        RecordGraphState();
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
                m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
            GraphCanvas::ViewRequestBus::Event(m_graphId, &GraphCanvas::ViewRequests::RefreshView);
            QueueCompileGraph();
        }
        return true;
    }

    GraphModel::GraphPtr MaterialCanvasDocument::GetGraph() const
    {
        return m_graph;
    }

    GraphCanvas::GraphId MaterialCanvasDocument::GetGraphId() const
    {
        return m_graphId;
    }

    AZStd::string MaterialCanvasDocument::GetGraphName() const
    {
        // Sanitize the document name to remove any illegal characters that could not be used as symbols in generated code
        AZStd::string documentName;
        AZ::StringFunc::Path::GetFullFileName(m_absolutePath.c_str(), documentName);
        AZ::StringFunc::Replace(documentName, ".materialcanvas.azasset", "");
        return AtomToolsFramework::GetSymbolNameFromText(documentName);
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
        m_groups.clear();
        m_modified = false;

        AtomToolsFramework::AtomToolsDocument::Clear();
    }

    void MaterialCanvasDocument::OnGraphModelSlotModified([[maybe_unused]] GraphModel::SlotPtr slot)
    {
        m_modified = true;
        BuildEditablePropertyGroups();
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
        QueueCompileGraph();
    }

    void MaterialCanvasDocument::OnGraphModelRequestUndoPoint()
    {
        // Undo and redo is being handled differently for edits received directly from graph model and graph canvas. By the time this is
        // reached, changes have already been applied to the graph. Other operations performed in the document class ensure that a last
        // known good graph state was recorded after every change to be able  to undo this operation. .
        auto undoState = m_graphStateForUndoRedo;
        RecordGraphState();
        auto redoState = m_graphStateForUndoRedo;

        if (undoState != redoState)
        {
            AddUndoRedoHistory(
                [this, undoState]() { RestoreGraphState(undoState); },
                [this, redoState]() { RestoreGraphState(redoState); });

            m_modified = true;
            BuildEditablePropertyGroups();
            AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
                m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
            QueueCompileGraph();
        }
    }

    void MaterialCanvasDocument::OnGraphModelTriggerUndo()
    {
        Undo();
    }

    void MaterialCanvasDocument::OnGraphModelTriggerRedo()
    {
        Redo();
    }

    void MaterialCanvasDocument::OnSelectionChanged()
    {
        BuildEditablePropertyGroups();
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

        m_modified = true;
        CreateGraph(graph);
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
        QueueCompileGraph();
    }

    void MaterialCanvasDocument::CreateGraph(GraphModel::GraphPtr graph)
    {
        DestroyGraph();

        if (graph)
        {
            m_graph = graph;
            m_graph->PostLoadSetup(m_graphContext);

            // The graph controller will create all of the scene items on construction.
            GraphModelIntegration::GraphManagerRequestBus::Broadcast(
                &GraphModelIntegration::GraphManagerRequests::CreateGraphController, m_graphId, m_graph);

            RecordGraphState();
            BuildEditablePropertyGroups();
        }
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

    void MaterialCanvasDocument::BuildEditablePropertyGroups()
    {
        // Sort nodes according to their connection so they appear in a consistent order in the inspector
        GraphModel::NodePtrList selectedNodes;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(
            selectedNodes, m_graphId, &GraphModelIntegration::GraphControllerRequests::GetSelectedNodes);

        SortNodesInExecutionOrder(selectedNodes);

        m_groups.clear();
        m_groups.reserve(selectedNodes.size());

        for (const auto& currentNode : selectedNodes)
        {
            auto dynamicNode = azrtti_cast<const AtomToolsFramework::DynamicNode*>(currentNode.get());
            if (!dynamicNode)
            {
                continue;
            }

            const auto& nodeConfig = dynamicNode->GetConfig();

            // Create a new property group and set up the header to match the node
            AZStd::shared_ptr<AtomToolsFramework::DynamicPropertyGroup> group;
            group.reset(aznew AtomToolsFramework::DynamicPropertyGroup);
            group->m_name = GetSymbolNameFromNode(currentNode);
            group->m_displayName = AtomToolsFramework::GetDisplayNameFromText(
                AZStd::string::format("Node%u %s", currentNode->GetId(), currentNode->GetTitle()));
            group->m_description = currentNode->GetSubTitle();

            group->m_properties.reserve(
                dynamicNode->GetConfig().m_propertySlots.size() +
                dynamicNode->GetConfig().m_inputSlots.size() +
                dynamicNode->GetConfig().m_outputSlots.size());

            // Visit all of the slots in the order to find in the configuration to add properties to the container for the inspector.
            AtomToolsFramework::VisitDynamicNodeSlotConfigs(
                nodeConfig,
                [&](const AtomToolsFramework::DynamicNodeSlotConfig& slotConfig)
                {
                    if (auto currentSlot = currentNode->GetSlot(slotConfig.m_name))
                    {
                        if (currentSlot->GetSlotDirection() == GraphModel::SlotDirection::Input)
                        {
                            // Create and add a dynamic property for each input slot on the node
                            AtomToolsFramework::DynamicPropertyConfig propertyConfig;
                            propertyConfig.m_id = currentSlot->GetName();
                            propertyConfig.m_name = currentSlot->GetName();
                            propertyConfig.m_displayName = currentSlot->GetDisplayName();
                            propertyConfig.m_groupName = group->m_name;
                            propertyConfig.m_groupDisplayName = group->m_displayName;
                            propertyConfig.m_description = currentSlot->GetDescription();
                            propertyConfig.m_defaultValue = currentSlot->GetDefaultValue();
                            propertyConfig.m_originalValue = currentSlot->GetValue();
                            propertyConfig.m_parentValue = currentSlot->GetDefaultValue();
                            propertyConfig.m_readOnly = !currentSlot->GetConnections().empty();
                            propertyConfig.m_showThumbnail = true;

                            // Set up the change call back to apply the value of the property from the inspector to the slot. This could
                            // also send a document modified notifications and queue regeneration of shader and material assets but the
                            // compilation process and going through the ap is not responsive enough for this to matter.
                            propertyConfig.m_dataChangeCallback = [currentSlot](const AZStd::any& value)
                            {
                                currentSlot->SetValue(value);
                                return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
                            };

                            group->m_properties.emplace_back(AZStd::move(propertyConfig));
                        }
                    }
                });

            m_groups.emplace_back(group);
        }

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentObjectInfoInvalidated, m_id);
    }

    AZStd::string MaterialCanvasDocument::GetOutputPathFromTemplatePath(const AZStd::string& templateInputPath) const
    {
        AZStd::string templateInputFileName;
        AZ::StringFunc::Path::GetFullFileName(templateInputPath.c_str(), templateInputFileName);
        AZ::StringFunc::Replace(templateInputFileName, ".template", "");

        AZStd::string templateOutputPath = m_absolutePath;
        AZ::StringFunc::Path::ReplaceFullName(templateOutputPath, templateInputFileName.c_str());

        AZ::StringFunc::Replace(templateOutputPath, "MaterialGraphName", GetGraphName().c_str());

        return templateOutputPath;
    }

    void MaterialCanvasDocument::ReplaceSymbolsInContainer(
        const AZStd::string& findText, const AZStd::string& replaceText, AZStd::vector<AZStd::string>& container) const
    {
        const AZStd::regex findRegex(findText);
        for (auto& sourceText : container)
        {
            sourceText = AZStd::regex_replace(sourceText, findRegex, replaceText);
        }
    }

    void MaterialCanvasDocument::ReplaceSymbolsInContainer(
        const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& substitutionSymbols, AZStd::vector<AZStd::string>& container) const
    {
        for (const auto& substitutionSymbolPair : substitutionSymbols)
        {
            ReplaceSymbolsInContainer(substitutionSymbolPair.first, substitutionSymbolPair.second, container);
        }
    }

    unsigned int MaterialCanvasDocument::GetVectorSize(const AZStd::any& slotValue) const
    {
        if (slotValue.is<AZ::Color>())
        {
            return 4;
        }
        if (slotValue.is<AZ::Vector4>())
        {
            return 4;
        }
        if (slotValue.is<AZ::Vector3>())
        {
            return 3;
        }
        if (slotValue.is<AZ::Vector2>())
        {
            return 2;
        }
        if (slotValue.is<bool>() || slotValue.is<int>() || slotValue.is<unsigned int>() || slotValue.is<float>())
        {
            return 1;
        }
        return 0;
    }

    AZStd::any MaterialCanvasDocument::ConvertToScalar(const AZStd::any& slotValue) const
    {
        if (auto v = AZStd::any_cast<const AZ::Color>(&slotValue))
        {
            return AZStd::any(v->GetR());
        }
        if (auto v = AZStd::any_cast<const AZ::Vector4>(&slotValue))
        {
            return AZStd::any(v->GetX());
        }
        if (auto v = AZStd::any_cast<const AZ::Vector3>(&slotValue))
        {
            return AZStd::any(v->GetX());
        }
        if (auto v = AZStd::any_cast<const AZ::Vector2>(&slotValue))
        {
            return AZStd::any(v->GetX());
        }
        return slotValue;
    }

    template<typename T>
    AZStd::any MaterialCanvasDocument::ConvertToVector(const AZStd::any& slotValue) const
    {
        if (auto v = AZStd::any_cast<const AZ::Color>(&slotValue))
        {
            return AZStd::any(T(v->GetAsVector4()));
        }
        if (auto v = AZStd::any_cast<const AZ::Vector4>(&slotValue))
        {
            return AZStd::any(T(*v));
        }
        if (auto v = AZStd::any_cast<const AZ::Vector3>(&slotValue))
        {
            return AZStd::any(T(*v));
        }
        if (auto v = AZStd::any_cast<const AZ::Vector2>(&slotValue))
        {
            return AZStd::any(T(*v));
        }
        return slotValue;
    }

    AZStd::any MaterialCanvasDocument::ConvertToVector(const AZStd::any& slotValue, unsigned int score) const
    {
        switch (score)
        {
        case 4:
            return ConvertToVector<AZ::Vector4>(slotValue);
        case 3:
            return ConvertToVector<AZ::Vector3>(slotValue);
        case 2:
            return ConvertToVector<AZ::Vector2>(slotValue);
        case 1:
            return ConvertToScalar(slotValue);
        default:
            return slotValue;
        }
    }

    AZStd::string MaterialCanvasDocument::GetAzslTypeFromSlot(GraphModel::ConstSlotPtr slot) const
    {
        const auto& slotItr = m_slotValueTable.find(slot);
        const auto& slotValue = slotItr != m_slotValueTable.end() ? slotItr->second : slot->GetValue();
        const auto& slotDataType = m_graphContext->GetDataTypeForValue(slotValue);
        const auto& slotDataTypeName = slotDataType ? slotDataType->GetDisplayName() : AZStd::string{};

        if (AZ::StringFunc::Equal(slotDataTypeName, "color"))
        {
            return "float4";
        }

        return slotDataTypeName;
    }

    AZStd::string MaterialCanvasDocument::GetAzslValueFromSlot(GraphModel::ConstSlotPtr slot) const
    {
        const auto& slotItr = m_slotValueTable.find(slot);
        const auto& slotValue = slotItr != m_slotValueTable.end() ? slotItr->second : slot->GetValue();

        // This code and some of these rules will be refactored and generalized after splitting this class into a document and builder or
        // compiler class. Once that is done, it will be easier to register types, conversions, substitutions with the system.
        for (const auto& connection : slot->GetConnections())
        {
            auto sourceSlot = connection->GetSourceSlot();
            auto targetSlot = connection->GetTargetSlot();
            if (targetSlot && sourceSlot && targetSlot != sourceSlot && targetSlot == slot)
            {
                // If there is an incoming connection to this slot, the name of the source slot from the incoming connection will be used as
                // part of the value for the slot. It must be cast to the correct vector type for generated code. These conversions will be
                // extended once the code is extracted and made part of a separate system.
                const auto& sourceSlotItr = m_slotValueTable.find(sourceSlot);
                const auto& sourceSlotValue = sourceSlotItr != m_slotValueTable.end() ? sourceSlotItr->second : sourceSlot->GetValue();
                const auto& sourceSlotSymbolName = GetSymbolNameFromSlot(sourceSlot);
                if (sourceSlotValue.is<AZ::Vector4>())
                {
                    if (slotValue.is<AZ::Vector3>())
                    {
                        return AZStd::string::format("(float3)%s", sourceSlotSymbolName.c_str());
                    }
                    if (slotValue.is<AZ::Vector2>())
                    {
                        return AZStd::string::format("(float2)%s", sourceSlotSymbolName.c_str());
                    }
                }
                if (sourceSlotValue.is<AZ::Vector3>())
                {
                    if (slotValue.is<AZ::Vector4>())
                    {
                        return AZStd::string::format("float4(%s, 1)", sourceSlotSymbolName.c_str());
                    }
                    if (slotValue.is<AZ::Vector2>())
                    {
                        return AZStd::string::format("(float2)%s", sourceSlotSymbolName.c_str());
                    }
                }
                if (sourceSlotValue.is<AZ::Vector2>())
                {
                    if (slotValue.is<AZ::Vector4>())
                    {
                        return AZStd::string::format("float4(%s, 0, 1)", sourceSlotSymbolName.c_str());
                    }
                    if (slotValue.is<AZ::Vector3>())
                    {
                        return AZStd::string::format("float3(%s, 0)", sourceSlotSymbolName.c_str());
                    }
                }
                return sourceSlotSymbolName;
            }
        }

        // If the slot's embedded value is being used then generate shader code to represent it. More generic options will be explored to
        // clean this code up, possibly storing numeric values in a two-dimensional floating point array with the layout corresponding to
        // most vector and matrix types.
        if (auto v = AZStd::any_cast<const AZ::Color>(&slotValue))
        {
            return AZStd::string::format("{%g, %g, %g, %g}", v->GetR(), v->GetG(), v->GetB(), v->GetA());
        }
        if (auto v = AZStd::any_cast<const AZ::Vector4>(&slotValue))
        {
            return AZStd::string::format("{%g, %g, %g, %g}", v->GetX(), v->GetY(), v->GetZ(), v->GetW());
        }
        if (auto v = AZStd::any_cast<const AZ::Vector3>(&slotValue))
        {
            return AZStd::string::format("{%g, %g, %g}", v->GetX(), v->GetY(), v->GetZ());
        }
        if (auto v = AZStd::any_cast<const AZ::Vector2>(&slotValue))
        {
            return AZStd::string::format("{%g, %g}", v->GetX(), v->GetY());
        }
        if (auto v = AZStd::any_cast<const AZStd::array<AZ::Vector2, 2>>(&slotValue))
        {
            const auto& value = *v;
            return AZStd::string::format(
                "{%g, %g, %g, %g}",
                value[0].GetX(), value[0].GetY(),
                value[1].GetX(), value[1].GetY());
        }
        if (auto v = AZStd::any_cast<const AZStd::array<AZ::Vector3, 3>>(&slotValue))
        {
            const auto& value = *v;
            return AZStd::string::format(
                "{%g, %g, %g, %g, %g, %g, %g, %g, %g}",
                value[0].GetX(), value[0].GetY(), value[0].GetZ(),
                value[1].GetX(), value[1].GetY(), value[1].GetZ(),
                value[2].GetX(), value[2].GetY(), value[2].GetZ());
        }
        if (auto v = AZStd::any_cast<const AZStd::array<AZ::Vector4, 3>>(&slotValue))
        {
            const auto& value = *v;
            return AZStd::string::format(
                "{%g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g}",
                value[0].GetX(), value[0].GetY(), value[0].GetZ(), value[0].GetW(),
                value[1].GetX(), value[1].GetY(), value[1].GetZ(), value[1].GetW(),
                value[2].GetX(), value[2].GetY(), value[2].GetZ(), value[2].GetW());
        }
        if (auto v = AZStd::any_cast<const AZStd::array<AZ::Vector4, 4>>(&slotValue))
        {
            const auto& value = *v;
            return AZStd::string::format(
                "{%g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g, %g}",
                value[0].GetX(), value[0].GetY(), value[0].GetZ(), value[0].GetW(),
                value[1].GetX(), value[1].GetY(), value[1].GetZ(), value[1].GetW(),
                value[2].GetX(), value[2].GetY(), value[2].GetZ(), value[2].GetW(),
                value[3].GetX(), value[3].GetY(), value[3].GetZ(), value[3].GetW());
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
        if (auto v = AZStd::any_cast<const bool>(&slotValue))
        {
            return AZStd::string::format("%u", *v ? 1 : 0);
        }
        return AZStd::string();
    }

    AZStd::string MaterialCanvasDocument::GetAzslSrgMemberFromSlot(
        GraphModel::ConstNodePtr node, const AtomToolsFramework::DynamicNodeSlotConfig& slotConfig) const
    {
        if (const auto& slot = node->GetSlot(slotConfig.m_name))
        {
            const auto& slotValue = slot->GetValue();
            if (auto v = AZStd::any_cast<const AZ::RHI::SamplerState>(&slotValue))
            {
                // The fields commented out below either cause errors or are not recognized by the shader compiler.
                AZStd::string srgMember;
                srgMember += AZStd::string::format("Sampler SLOTNAME\n");
                srgMember += AZStd::string::format("{\n");
                srgMember += AZStd::string::format("MaxAnisotropy = %u;\n", v->m_anisotropyMax);
                //srgMember += AZStd::string::format("AnisotropyEnable = %u;\n", v->m_anisotropyEnable);
                srgMember += AZStd::string::format("MinFilter = %s;\n", AZ::RHI::FilterModeNamespace::ToString(v->m_filterMin).data());
                srgMember += AZStd::string::format("MagFilter = %s;\n", AZ::RHI::FilterModeNamespace::ToString(v->m_filterMag).data());
                srgMember += AZStd::string::format("MipFilter = %s;\n", AZ::RHI::FilterModeNamespace::ToString(v->m_filterMip).data());
                srgMember += AZStd::string::format("ReductionType = %s;\n", AZ::RHI::ReductionTypeNamespace::ToString(v->m_reductionType).data());
                //srgMember += AZStd::string::format("ComparisonFunc = %s;\n", AZ::RHI::ComparisonFuncNamespace::ToString(v->m_comparisonFunc).data());
                srgMember += AZStd::string::format("AddressU = %s;\n", AZ::RHI::AddressModeNamespace::ToString(v->m_addressU).data());
                srgMember += AZStd::string::format("AddressV = %s;\n", AZ::RHI::AddressModeNamespace::ToString(v->m_addressV).data());
                srgMember += AZStd::string::format("AddressW = %s;\n", AZ::RHI::AddressModeNamespace::ToString(v->m_addressW).data());
                srgMember += AZStd::string::format("MinLOD = %f;\n", v->m_mipLodMin);
                srgMember += AZStd::string::format("MaxLOD = %f;\n", v->m_mipLodMax);
                srgMember += AZStd::string::format("MipLODBias = %f;\n", v->m_mipLodBias);
                srgMember += AZStd::string::format("BorderColor = %s;\n", AZ::RHI::BorderColorNamespace::ToString(v->m_borderColor).data());
                srgMember += "};\n";
                return srgMember;
            }

            if (auto v = AZStd::any_cast<const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>>(&slotValue))
            {
                return AZStd::string::format("Texture2D SLOTNAME;\n");
            }

            return AZStd::string::format("SLOTTYPE SLOTNAME;\n");
        }
        return AZStd::string();
    }

    AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> MaterialCanvasDocument::GetSubstitutionSymbolsFromNode(
        GraphModel::ConstNodePtr node) const
    {
        AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> substitutionSymbols;

        // Reserving space for the number of elements added in this function. 
        substitutionSymbols.reserve(node->GetSlots().size() * 4 + 1);
        substitutionSymbols.emplace_back("NODEID", GetSymbolNameFromNode(node));

        for (const auto& slotPair : node->GetSlots())
        {
            const auto& slot = slotPair.second;

            // These substitutions will allow accessing the slot ID, type, value from anywhere in the node's shader code.
            substitutionSymbols.emplace_back(AZStd::string::format("SLOTTYPE\\(%s\\)", slot->GetName().c_str()), GetAzslTypeFromSlot(slot));
            substitutionSymbols.emplace_back(AZStd::string::format("SLOTVALUE\\(%s\\)", slot->GetName().c_str()), GetAzslValueFromSlot(slot));
            substitutionSymbols.emplace_back(AZStd::string::format("SLOTNAME\\(%s\\)", slot->GetName().c_str()), GetSymbolNameFromSlot(slot));

            // This expression will allow direct substitution of node variable names in node configurations with the decorated symbol name.
            // It will match whole words only. No additional decoration should be required on the node configuration side. However, support
            // for the older slot type, name, value substitutions are still supported as a convenience.
            substitutionSymbols.emplace_back(AZStd::string::format("\\b%s\\b", slot->GetName().c_str()), GetSymbolNameFromSlot(slot));
        }
        return substitutionSymbols;
    }

    AZStd::vector<AZStd::string> MaterialCanvasDocument::GetInstructionsFromSlot(
        GraphModel::ConstNodePtr node,
        const AtomToolsFramework::DynamicNodeSlotConfig& slotConfig,
        const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& substitutionSymbols) const
    {
        AZStd::vector<AZStd::string> instructionsForSlot;

        auto slot = node->GetSlot(slotConfig.m_name);
        if (slot && (slot->GetSlotDirection() != GraphModel::SlotDirection::Output || !slot->GetConnections().empty()))
        {
            AtomToolsFramework::CollectDynamicNodeSettings(slotConfig.m_settings, "instructions", instructionsForSlot);

            ReplaceSymbolsInContainer(substitutionSymbols, instructionsForSlot);
            ReplaceSymbolsInContainer("SLOTNAME", GetSymbolNameFromSlot(slot), instructionsForSlot);
            ReplaceSymbolsInContainer("SLOTTYPE", GetAzslTypeFromSlot(slot), instructionsForSlot);
            ReplaceSymbolsInContainer("SLOTVALUE", GetAzslValueFromSlot(slot), instructionsForSlot);
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
                        AZ_Assert(connection->GetSourceNode() != outputNode, "This should never be the source node on an input connection.");
                        AZ_Assert(connection->GetTargetNode() == outputNode, "This should always be the target node on an input connection.");
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

    template<typename NodeContainer>
    void MaterialCanvasDocument::SortNodesInExecutionOrder(NodeContainer& nodes) const
    {
        using NodeValueType = typename NodeContainer::value_type;
        using NodeValueTypeRef = typename NodeContainer::const_reference;

        // Pre-calculate and cache sorting scores for all nodes to avoid reprocessing during the sort
        AZStd::map<NodeValueType, AZStd::tuple<bool, bool, uint32_t>> nodeScoreTable;
        for (NodeValueTypeRef node : nodes)
        {
            nodeScoreTable[node] = AZStd::make_tuple(node->HasInputSlots(), !node->HasOutputSlots(), node->GetMaxInputDepth());
        }

        AZStd::stable_sort(nodes.begin(), nodes.end(), [&](NodeValueTypeRef nodeA, NodeValueTypeRef nodeB) {
            return nodeScoreTable[nodeA] < nodeScoreTable[nodeB];
        });
    }

    AZStd::vector<GraphModel::ConstNodePtr> MaterialCanvasDocument::GetAllNodesInExecutionOrder() const
    {
        AZStd::vector<GraphModel::ConstNodePtr> nodes;

        if (m_graph)
        {
            nodes.reserve(m_graph->GetNodes().size());
            for (const auto& nodePair : m_graph->GetNodes())
            {
                nodes.push_back(nodePair.second);
            }

            SortNodesInExecutionOrder(nodes);
        }

        return nodes;
    }

    AZStd::vector<GraphModel::ConstNodePtr> MaterialCanvasDocument::GetInstructionNodesInExecutionOrder(
        GraphModel::ConstNodePtr outputNode, const AZStd::vector<AZStd::string>& inputSlotNames) const
    {
        AZStd::vector<GraphModel::ConstNodePtr> nodes = GetAllNodesInExecutionOrder();
        AZStd::erase_if(nodes, [this, &outputNode, &inputSlotNames](const auto& node) {
            return !ShouldUseInstructionsFromInputNode(outputNode, node, inputSlotNames);
        });
        return nodes;
    }

    AZStd::vector<AZStd::string> MaterialCanvasDocument::GetInstructionsFromConnectedNodes(
        GraphModel::ConstNodePtr outputNode,
        const AZStd::vector<AZStd::string>& inputSlotNames,
        AZStd::vector<GraphModel::ConstNodePtr>& instructionNodes) const
    {
        AZStd::vector<AZStd::string> instructions;

        for (const auto& inputNode : GetInstructionNodesInExecutionOrder(outputNode, inputSlotNames))
        {
            // Build a list of all nodes that will contribute instructions for the output node
            if (AZStd::find(instructionNodes.begin(), instructionNodes.end(), inputNode) == instructionNodes.end())
            {
                instructionNodes.push_back(inputNode);
            }

            auto dynamicNode = azrtti_cast<const AtomToolsFramework::DynamicNode*>(inputNode.get());
            if (dynamicNode)
            {
                const auto& nodeConfig = dynamicNode->GetConfig();
                const auto& substitutionSymbols = GetSubstitutionSymbolsFromNode(inputNode);

                // Instructions are gathered separately for all of the slot categories because they need to be added in a specific order.

                // Gather and perform substitutions on instructions embedded directly in the node.
                AZStd::vector<AZStd::string> instructionsForNode;
                AtomToolsFramework::CollectDynamicNodeSettings(nodeConfig.m_settings, "instructions", instructionsForNode);
                ReplaceSymbolsInContainer(substitutionSymbols, instructionsForNode);

                // Gather and perform substitutions on instructions contained in property slots.
                AZStd::vector<AZStd::string> instructionsForPropertySlots;
                for (const auto& slotConfig : nodeConfig.m_propertySlots)
                {
                    const auto& instructionsForSlot = GetInstructionsFromSlot(inputNode, slotConfig, substitutionSymbols);
                    instructionsForPropertySlots.insert(instructionsForPropertySlots.end(), instructionsForSlot.begin(), instructionsForSlot.end());
                }

                // Gather and perform substitutions on instructions contained in input slots.
                AZStd::vector<AZStd::string> instructionsForInputSlots;
                for (const auto& slotConfig : nodeConfig.m_inputSlots)
                {
                    // If this is the output node, only gather instructions for requested input slots.
                    if (inputNode == outputNode &&
                        AZStd::find(inputSlotNames.begin(), inputSlotNames.end(), slotConfig.m_name) == inputSlotNames.end())
                    {
                        continue;
                    }

                    const auto& instructionsForSlot = GetInstructionsFromSlot(inputNode, slotConfig, substitutionSymbols);
                    instructionsForInputSlots.insert(instructionsForInputSlots.end(), instructionsForSlot.begin(), instructionsForSlot.end());
                }

                // Gather and perform substitutions on instructions contained in output slots.
                AZStd::vector<AZStd::string> instructionsForOutputSlots;
                for (const auto& slotConfig : nodeConfig.m_outputSlots)
                {
                    const auto& instructionsForSlot = GetInstructionsFromSlot(inputNode, slotConfig, substitutionSymbols);
                    instructionsForOutputSlots.insert(instructionsForOutputSlots.end(), instructionsForSlot.begin(), instructionsForSlot.end());
                }

                instructions.insert(instructions.end(), instructionsForPropertySlots.begin(), instructionsForPropertySlots.end());
                instructions.insert(instructions.end(), instructionsForInputSlots.begin(), instructionsForInputSlots.end());
                instructions.insert(instructions.end(), instructionsForNode.begin(), instructionsForNode.end());
                instructions.insert(instructions.end(), instructionsForOutputSlots.begin(), instructionsForOutputSlots.end());
            }
        }

        return instructions;
    }

    AZStd::string MaterialCanvasDocument::GetSymbolNameFromNode(GraphModel::ConstNodePtr node) const
    {
        return AtomToolsFramework::GetSymbolNameFromText(AZStd::string::format("node%u_%s", node->GetId(), node->GetTitle()));
    }

    AZStd::string MaterialCanvasDocument::GetSymbolNameFromSlot(GraphModel::ConstSlotPtr slot) const
    {
        bool allowNameSubstitution = true;
        if (auto dynamicNode = azrtti_cast<const AtomToolsFramework::DynamicNode*>(slot->GetParentNode().get()))
        {
            const auto& nodeConfig = dynamicNode->GetConfig();
            AtomToolsFramework::VisitDynamicNodeSlotConfigs(
                nodeConfig,
                [&](const AtomToolsFramework::DynamicNodeSlotConfig& slotConfig)
                {
                    if (slot->GetName() == slotConfig.m_name)
                    {
                        allowNameSubstitution = slotConfig.m_allowNameSubstitution;
                    }
                });
        }

        if (!allowNameSubstitution)
        {
            return slot->GetName();
        }

        if (slot->SupportsExtendability())
        {
            return AZStd::string::format(
                "%s_%s_%d", GetSymbolNameFromNode(slot->GetParentNode()).c_str(), slot->GetName().c_str(), slot->GetSlotSubId());
        }

        return AZStd::string::format("%s_%s", GetSymbolNameFromNode(slot->GetParentNode()).c_str(), slot->GetName().c_str());
    }

    AZStd::vector<AZStd::string> MaterialCanvasDocument::GetMaterialInputsFromSlot(
        GraphModel::ConstNodePtr node,
        const AtomToolsFramework::DynamicNodeSlotConfig& slotConfig,
        const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& substitutionSymbols) const
    {
        AZStd::vector<AZStd::string> materialInputsForSlot;

        if (auto slot = node->GetSlot(slotConfig.m_name))
        {
            AtomToolsFramework::CollectDynamicNodeSettings(slotConfig.m_settings, "materialInputs", materialInputsForSlot);

            ReplaceSymbolsInContainer(substitutionSymbols, materialInputsForSlot);
            ReplaceSymbolsInContainer("SLOTSTANDARDSRGMEMBER", GetAzslSrgMemberFromSlot(node, slotConfig), materialInputsForSlot);
            ReplaceSymbolsInContainer("SLOTNAME", GetSymbolNameFromSlot(slot), materialInputsForSlot);
            ReplaceSymbolsInContainer("SLOTTYPE", GetAzslTypeFromSlot(slot), materialInputsForSlot);
            ReplaceSymbolsInContainer("SLOTVALUE", GetAzslValueFromSlot(slot), materialInputsForSlot);
        }

        return materialInputsForSlot;
    }

    AZStd::vector<AZStd::string> MaterialCanvasDocument::GetMaterialInputsFromNodes(
        const AZStd::vector<GraphModel::ConstNodePtr>& instructionNodes) const
    {
        AZ_Assert(m_graph, "Attempting to generate data from invalid graph object.");

        AZStd::vector<AZStd::string> materialInputs;

        for (const auto& inputNode : instructionNodes)
        {
            auto dynamicNode = azrtti_cast<const AtomToolsFramework::DynamicNode*>(inputNode.get());
            if (dynamicNode)
            {
                const auto& nodeConfig = dynamicNode->GetConfig();
                const auto& substitutionSymbols = GetSubstitutionSymbolsFromNode(inputNode);

                AZStd::vector<AZStd::string> materialInputsForNode;
                AtomToolsFramework::CollectDynamicNodeSettings(nodeConfig.m_settings, "materialInputs", materialInputsForNode);
                ReplaceSymbolsInContainer(substitutionSymbols, materialInputsForNode);

                AtomToolsFramework::VisitDynamicNodeSlotConfigs(
                    nodeConfig,
                    [&](const AtomToolsFramework::DynamicNodeSlotConfig& slotConfig)
                    {
                        const auto& materialInputsForSlot = GetMaterialInputsFromSlot(inputNode, slotConfig, substitutionSymbols);
                        materialInputsForNode.insert(materialInputsForNode.end(), materialInputsForSlot.begin(), materialInputsForSlot.end());
                    });

                materialInputs.insert(materialInputs.end(), materialInputsForNode.begin(), materialInputsForNode.end());
            }
        }

        return materialInputs;
    }

    bool MaterialCanvasDocument::BuildMaterialTypeFromTemplate(
        GraphModel::ConstNodePtr templateNode,
        const AZStd::vector<GraphModel::ConstNodePtr>& instructionNodes,
        const AZStd::string& templateInputPath,
        const AZStd::string& templateOutputPath) const
    {
        AZ_Assert(m_graph, "Attempting to generate data from invalid graph object.");
        AZ_Assert(templateNode, "Attempting to generate data from invalid template node.");

        // Load the material type template file, which is the same format as MaterialTypeSourceData with a different extension
        auto materialTypeOutcome = AZ::RPI::MaterialUtils::LoadMaterialTypeSourceData(templateInputPath);
        if (!materialTypeOutcome.IsSuccess())
        {
            AZ_Error("MaterialCanvasDocument", false, "Material type template could not be loaded: '%s'.", templateInputPath.c_str());
            return false;
        }

        // Copy the material type source data from the template and begin populating it.
        AZ::RPI::MaterialTypeSourceData materialTypeSourceData = materialTypeOutcome.TakeValue();

        // If the node providing all the template information has a description then assign it to the material type source data.
        const auto templateDescriptionSlot = templateNode->GetSlot("inDescription");
        if (templateDescriptionSlot)
        {
            materialTypeSourceData.m_description = templateDescriptionSlot->GetValue<AZStd::string>();
        }

        // Search the graph for nodes defining material input properties that should be added to the material type and material SRG
        for (const auto& inputNode : instructionNodes)
        {
            // Gather a list of all of the slots with data that needs to be added to the material type. 
            AZStd::vector<GraphModel::ConstSlotPtr> materialInputValueSlots;
            if (auto dynamicNode = azrtti_cast<const AtomToolsFramework::DynamicNode*>(inputNode.get()))
            {
                AtomToolsFramework::VisitDynamicNodeSlotConfigs(
                    dynamicNode->GetConfig(),
                    [&](const AtomToolsFramework::DynamicNodeSlotConfig& slotConfig)
                    {
                        if (slotConfig.m_settings.contains("materialInputs"))
                        {
                            const auto& materialInputValueSlot = inputNode->GetSlot(slotConfig.m_name);
                            if (materialInputValueSlot &&
                                !materialInputValueSlot->GetValue().empty() &&
                                !materialInputValueSlot->GetValue().is<AZ::RHI::SamplerState>())
                            {
                                materialInputValueSlots.push_back(materialInputValueSlot);
                            }
                        }
                    });
            }

            if (materialInputValueSlots.empty())
            {
                continue;
            }

            // Each node contains property and input slots corresponding to MaterialTypeSourceData::PropertyDefinition members
            const auto materialInputNameSlot = inputNode->GetSlot("inName");
            const auto materialInputGroupSlot = inputNode->GetSlot("inGroup");
            const auto materialInputDescriptionSlot = inputNode->GetSlot("inDescription");
            if (!materialInputGroupSlot || !materialInputNameSlot || !materialInputDescriptionSlot)
            {
                continue;
            }

            // Because users can specify any value for property and group names, and attempt will be made to convert them into valid,
            // usable names by sanitizing, removing unsupported characters, and changing case
            AZStd::string propertyGroupName = AtomToolsFramework::GetSymbolNameFromText(materialInputGroupSlot->GetValue<AZStd::string>());
            if (propertyGroupName.empty())
            {
                // If no group name was specified, general will be used by default
                propertyGroupName = "general";
            }

            // Find or create a property group with the specified name
            auto propertyGroup = materialTypeSourceData.FindPropertyGroup(propertyGroupName);
            if (!propertyGroup)
            {
                // Add the property group to the material type if it was not already registered
                propertyGroup = materialTypeSourceData.AddPropertyGroup(propertyGroupName);

                // The unmodified text value will be used as the display name and description for now
                propertyGroup->SetDisplayName(AtomToolsFramework::GetDisplayNameFromText(propertyGroupName));
                propertyGroup->SetDescription(AtomToolsFramework::GetDisplayNameFromText(propertyGroupName));
            }

            // Register all the properties that were parsed out of the slots with the material type. 
            for (const auto& materialInputValueSlot : materialInputValueSlots)
            {
                // The variable name is generated from the node ID and the slot name.
                const auto& variableName = GetSymbolNameFromSlot(materialInputValueSlot);

                // The display name is optional but an attempt will be made to read it from the display name slot.
                const auto& displayName = AtomToolsFramework::GetDisplayNameFromText(materialInputNameSlot->GetValue<AZStd::string>());

                // The property name exposed for scripting and assigning material values will be derived from the display name, if
                // specified. Otherwise it will be the same as the variable name.
                const auto& propertyName = !displayName.empty() ? AtomToolsFramework::GetSymbolNameFromText(displayName) : variableName;

                // The property ID is composed of a combination of the group name and the property name. This is the full address of a
                // material property and what will appear in the material type and material files.
                const AZ::Name propertyId(propertyGroupName + "." + propertyName);

                auto property = propertyGroup->AddProperty(propertyName);
                property->m_displayName = displayName;
                property->m_description = materialInputDescriptionSlot->GetValue<AZStd::string>();
                property->m_value = AZ::RPI::MaterialPropertyValue::FromAny(materialInputValueSlot->GetValue());

                // The property definition requires an explicit type enum that's converted from the actual data type.
                property->m_dataType =
                    AtomToolsFramework::GetMaterialPropertyDataTypeFromValue(property->m_value, !property->m_enumValues.empty());

                // Images and enums need additional conversion prior to being saved.
                AtomToolsFramework::ConvertToExportFormat(templateOutputPath, propertyId, *property, property->m_value);

                // This property connects to the material SRG member with the same name. Shader options are not yet supported.
                property->m_outputConnections.emplace_back(AZ::RPI::MaterialPropertyOutputType::ShaderInput, variableName, -1);
            }
        }

        // The file is written to an in memory buffer before saving to facilitate string substitutions.
        AZStd::string templateOutputText;
        if (!AZ::RPI::JsonUtils::SaveObjectToString(templateOutputText, materialTypeSourceData))
        {
            AZ_Error("MaterialCanvasDocument", false, "Material type template could not be saved: '%s'.", templateOutputPath.c_str());
            return false;
        }

        // Substitute the material graph name and any other material canvas specific tokens
        AZ::StringFunc::Replace(templateOutputText, "MaterialGraphName", GetGraphName().c_str());

        AZ_TracePrintf_IfTrue(
            "MaterialCanvasDocument", IsCompileLoggingEnabled(), "Saving generated file: %s\n", templateOutputPath.c_str());

        // The material type is complete and can be saved to disk.
        const auto writeOutcome = AZ::Utils::WriteFile(templateOutputText, templateOutputPath);
        if (!writeOutcome)
        {
            AZ_Error("MaterialCanvasDocument", false, "Material type template could not be saved: '%s'.", templateOutputPath.c_str());
            return false;
        }

        return true;
    }

    bool MaterialCanvasDocument::CompileGraph() const
    {
        m_compileGraphQueued = false;
        m_slotValueTable.clear();
        m_generatedFiles.clear();

        // Skip compilation if there is no graph or this is a template.
        if (!m_graph || AZ::StringFunc::EndsWith(m_absolutePath, "materialcanvastemplate.azasset"))
        {
            return false;
        }

        AZ_TracePrintf_IfTrue("MaterialCanvasDocument", IsCompileLoggingEnabled(), "Compiling graph data.\n");

        // All slots and nodes will be visited to collect all of the unique include paths.
        AZStd::set<AZStd::string> includePaths;

        // There's probably no reason to distinguish between function and class definitions.
        // This could really be any globally defined function, class, struct, define.
        AZStd::vector<AZStd::string> classDefinitions;
        AZStd::vector<AZStd::string> functionDefinitions;

        // Visit all unique node configurations in the graph to collect their include paths, class definitions, and function definitions.
        AZStd::unordered_set<AZ::Uuid> configIdsVisited;
        for (const auto& nodePair : m_graph->GetNodes())
        {
            const auto& currentNode = nodePair.second;

            if (auto dynamicNode = azrtti_cast<const AtomToolsFramework::DynamicNode*>(currentNode.get()))
            {
                if (!configIdsVisited.contains(dynamicNode->GetConfig().m_id))
                {
                    configIdsVisited.insert(dynamicNode->GetConfig().m_id);
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
        }

        const auto& allNodes = GetAllNodesInExecutionOrder();
        BuildSlotValueTable(allNodes);

        // Traverse all graph nodes and slots searching for settings to generate files from templates
        for (const auto& currentNode : allNodes)
        {
            // Search this node for any template path settings that describe files that need to be generated from the graph.
            AZStd::set<AZStd::string> templatePaths;
            if (auto dynamicNode = azrtti_cast<const AtomToolsFramework::DynamicNode*>(currentNode.get()))
            {
                AtomToolsFramework::VisitDynamicNodeSettings(
                    dynamicNode->GetConfig(),
                    [&templatePaths](const AtomToolsFramework::DynamicNodeSettingsMap& settings)
                    {
                        AtomToolsFramework::CollectDynamicNodeSettings(settings, "templatePaths", templatePaths);
                    });
            }

            // If no template files were specified for this node then skip additional processing and continue to the next one.
            if (templatePaths.empty())
            {
                continue;
            }

            // Attempt to load all of the template files referenced by this node. All of the template data will be tokenized into individual
            // lines and stored in a container so then multiple passes can be made on each file, substituting tokens and filling in
            // details provided by the graph. None of the files generated from this node will be saved until they have all been processed.
            // Template files for material types will be processed in their own pass Because they require special handling and need to be
            // saved before material file templates to not trigger asset processor dependency errors.
            AZStd::vector<TemplateFileData> templateFileDataVec;
            for (const auto& templatePath : templatePaths)
            {
                TemplateFileData templateFileData;
                templateFileData.m_inputPath = AtomToolsFramework::GetPathWithoutAlias(templatePath);
                templateFileData.m_outputPath = GetOutputPathFromTemplatePath(templateFileData.m_inputPath);
                if (!AZ::StringFunc::EndsWith(templateFileData.m_outputPath, ".materialtype"))
                {
                    // Attempt to load the template file to do symbol substitution and inject code or data
                    if (!templateFileData.Load())
                    {
                        m_slotValueTable.clear();
                        m_generatedFiles.clear();
                        return false;
                    }
                    templateFileDataVec.emplace_back(AZStd::move(templateFileData));
                }
            }

            // Perform an initial pass over all template files, injecting include files, class definitions, function definitions, simple
            // things that don't require much processing.
            for (auto& templateFileData : templateFileDataVec)
            {
                // Substitute all references to the placeholder graph name with one generated from the document name
                ReplaceSymbolsInContainer("MaterialGraphName", GetGraphName(), templateFileData.m_lines);

                // Inject include files found while traversing the graph into any include file blocks in the template.
                templateFileData.ReplaceLinesInBlock(
                    "O3DE_GENERATED_INCLUDES_BEGIN",
                    "O3DE_GENERATED_INCLUDES_END",
                    [&includePaths, &templateFileData]([[maybe_unused]] const AZStd::string& blockHeader)
                    {
                        // Include file paths will need to be converted to include statements.
                        AZStd::vector<AZStd::string> includeStatements;
                        includeStatements.reserve(includePaths.size());
                        for (const auto& path : includePaths)
                        {
                            // TODO Replace relative path reference function
                            // The relative path reference function will only work for include files in the same gem.
                            includeStatements.push_back(AZStd::string::format(
                                "#include <%s>;",
                                AtomToolsFramework::GetPathToExteralReference(templateFileData.m_outputPath, path).c_str()));
                        }
                        return includeStatements;
                    });

                // Inject class definitions found while traversing the graph.
                templateFileData.ReplaceLinesInBlock(
                    "O3DE_GENERATED_CLASSES_BEGIN",
                    "O3DE_GENERATED_CLASSES_END",
                    [&classDefinitions]([[maybe_unused]] const AZStd::string& blockHeader)
                    {
                        return classDefinitions;
                    });

                // Inject function definitions found while traversing the graph.
                templateFileData.ReplaceLinesInBlock(
                    "O3DE_GENERATED_FUNCTIONS_BEGIN",
                    "O3DE_GENERATED_FUNCTIONS_END",
                    [&functionDefinitions]([[maybe_unused]] const AZStd::string& blockHeader)
                    {
                        return functionDefinitions;
                    });
            }

            // The next phase injects shader code instructions assembled by traversing the graph from each of the input slots on the current
            // node. The O3DE_GENERATED_INSTRUCTIONS_BEGIN marker will be followed by a list of input slot names corresponding to required
            // variables in the shader. Instructions will only be generated for the current node and nodes connected to the specified
            // inputs. This will allow multiple O3DE_GENERATED_INSTRUCTIONS blocks with different inputs to be specified in multiple
            // locations across multiple files from a single graph.

            // This will also keep track of nodes with instructions and data that contribute to the final shader code. The list of
            // contributing nodes will be used to exclude unused material inputs from generated SRGs and material types.
            AZStd::vector<GraphModel::ConstNodePtr> instructionNodesForAllBlocks;
            for (auto& templateFileData : templateFileDataVec)
            {
                templateFileData.ReplaceLinesInBlock(
                    "O3DE_GENERATED_INSTRUCTIONS_BEGIN",
                    "O3DE_GENERATED_INSTRUCTIONS_END",
                    [&]([[maybe_unused]] const AZStd::string& blockHeader)
                    {
                        AZStd::vector<AZStd::string> inputSlotNames;
                        AZ::StringFunc::Tokenize(blockHeader, inputSlotNames, ";:, \t\r\n\\/", false, false);
                        return GetInstructionsFromConnectedNodes(currentNode, inputSlotNames, instructionNodesForAllBlocks);
                    });
            }

            // At this point, all of the instructions have been generated for all of the template files used by this node. We now also have
            // a complete list of all nodes that contributed instructions to the final shader code across all of the files. Now, we can
            // safely generate the material SRG and material type that only contain variables referenced in the shaders. Without tracking
            // this, all variables would be included in the SRG and material type. The shader compiler would eliminate unused variables from
            // the compiled shader code. The material type would fail to build if it referenced any of the eliminated variables.
            for (auto& templateFileData : templateFileDataVec)
            {
                templateFileData.ReplaceLinesInBlock(
                    "O3DE_GENERATED_MATERIAL_SRG_BEGIN",
                    "O3DE_GENERATED_MATERIAL_SRG_END",
                    [&]([[maybe_unused]] const AZStd::string& blockHeader)
                    {
                        return GetMaterialInputsFromNodes(instructionNodesForAllBlocks);
                    });
            }

            // Save all of the generated files except for materials and material types. Generated material type files must be saved after
            // generated shader files to prevent AP errors because of missing dependencies.
            for (const auto& templateFileData : templateFileDataVec)
            {
                if (!AZ::StringFunc::EndsWith(templateFileData.m_outputPath, ".material"))
                {
                    if (!templateFileData.Save())
                    {
                        m_slotValueTable.clear();
                        m_generatedFiles.clear();
                        return false;
                    }
                    m_generatedFiles.push_back(templateFileData.m_outputPath);
                }
            }

            // Process material type template files, injecting properties found in material input nodes.
            for (const auto& templatePath : templatePaths)
            {
                // Remove any aliases to resolve the absolute path to the template file
                const AZStd::string templateInputPath = AtomToolsFramework::GetPathWithoutAlias(templatePath);
                const AZStd::string templateOutputPath = GetOutputPathFromTemplatePath(templateInputPath);
                if (!AZ::StringFunc::EndsWith(templateOutputPath, ".materialtype"))
                {
                    continue;
                }

                if (!BuildMaterialTypeFromTemplate(currentNode, instructionNodesForAllBlocks, templateInputPath, templateOutputPath))
                {
                    m_slotValueTable.clear();
                    m_generatedFiles.clear();
                    return false;
                }
                m_generatedFiles.push_back(templateOutputPath);
            }

            // After the material types have been processed and saved, we can save the materials that reference them.
            for (const auto& templateFileData : templateFileDataVec)
            {
                if (AZ::StringFunc::EndsWith(templateFileData.m_outputPath, ".material"))
                {
                    if (!templateFileData.Save())
                    {
                        m_slotValueTable.clear();
                        m_generatedFiles.clear();
                        return false;
                    }
                    m_generatedFiles.push_back(templateFileData.m_outputPath);
                }
            }
        }

        MaterialCanvasDocumentNotificationBus::Event(
            m_toolId, &MaterialCanvasDocumentNotificationBus::Events::OnCompileGraphCompleted, m_id);
        return true;
    }

    void MaterialCanvasDocument::BuildSlotValueTable(const AZStd::vector<GraphModel::ConstNodePtr>& allNodes) const
    {
        // Build a table of all values for every slot in the graph.
        m_slotValueTable.clear();
        for (const auto& currentNode : allNodes)
        {
            for (const auto& currentSlotPair : currentNode->GetSlots())
            {
                const auto& currentSlot = currentSlotPair.second;
                const auto& currentSlotValue = currentSlot->GetValue();
                m_slotValueTable[currentSlot] = currentSlotValue;
            }

            // If this is a dynamic node with slot data type groups, we will search for the largest vector or other data type and convert
            // all of the values in the group to the same type. 
            if (auto dynamicNode = azrtti_cast<const AtomToolsFramework::DynamicNode*>(currentNode.get()))
            {
                const auto& nodeConfig = dynamicNode->GetConfig();
                for (const auto& slotDataTypeGroup : nodeConfig.m_slotDataTypeGroups)
                {
                    unsigned int vectorSize = 0;

                    // The slot data group string is separated by vertical bars and can be treated like a regular expression to compare
                    // against slot names. The largest vector size is recorded for each slot group. 
                    const AZStd::regex slotDataTypeGroupRegex(slotDataTypeGroup, AZStd::regex::flag_type::icase);
                    for (const auto& currentSlotPair : currentNode->GetSlots())
                    {
                        const auto& currentSlot = currentSlotPair.second;
                        const auto& currentSlotValue = currentSlot->GetValue();
                        if (currentSlot->GetSlotDirection() == GraphModel::SlotDirection::Input &&
                            AZStd::regex_match(currentSlot->GetName(), slotDataTypeGroupRegex))
                        {
                            vectorSize = AZStd::max(vectorSize, GetVectorSize(currentSlotValue));
                        }
                    }

                    // Once all of the container sizes have been recorded for each slot data group, iterate over all of these slot values
                    // and upgrade entries in the map to the bigger type.
                    for (const auto& currentSlotPair : currentNode->GetSlots())
                    {
                        const auto& currentSlot = currentSlotPair.second;
                        const auto& currentSlotValue = currentSlot->GetValue();
                        if (AZStd::regex_match(currentSlot->GetName(), slotDataTypeGroupRegex))
                        {
                            m_slotValueTable[currentSlot] = ConvertToVector(currentSlotValue, vectorSize);
                        }
                    }
                }
            }
        }
    }

    void MaterialCanvasDocument::QueueCompileGraph() const
    {
        if (m_graph && !m_compileGraphQueued)
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

    bool MaterialCanvasDocument::TemplateFileData::Load()
    {
        AZ_TracePrintf_IfTrue("MaterialCanvasDocument", IsCompileLoggingEnabled(), "Loading template file: %s\n", m_inputPath.c_str());

        // Attempt to load the template file to do symbol substitution and inject any code or data
        if (auto result = AZ::Utils::ReadFile(m_inputPath))
        {
            // Tokenize the entire template file into individual lines that can be evaluated, removed, replaced, and have
            // content injected between them
            AZ::StringFunc::Tokenize(result.GetValue(), m_lines, '\n', true, true);
            AZ_TracePrintf_IfTrue(
                "MaterialCanvasDocument", IsCompileLoggingEnabled(), "Loading template file succeeded: %s\n", m_inputPath.c_str());
            return true;
        }

        AZ_Error("MaterialCanvasDocument", false, "Loading template file failed: %s\n", m_inputPath.c_str());
        return false;
    }

    bool MaterialCanvasDocument::TemplateFileData::Save() const
    {
        AZ_TracePrintf_IfTrue("MaterialCanvasDocument", IsCompileLoggingEnabled(), "Saving generated file: %s\n", m_outputPath.c_str());

        AZStd::string templateOutputText;
        AZ::StringFunc::Join(templateOutputText, m_lines, '\n');
        templateOutputText += '\n';

        // Save the file generated from the template to the same folder as the graph.
        if (AZ::Utils::WriteFile(templateOutputText, m_outputPath).IsSuccess())
        {
            AZ_TracePrintf_IfTrue(
                "MaterialCanvasDocument", IsCompileLoggingEnabled(), "Saving generated file succeeded: %s\n", m_outputPath.c_str());
            return true;
        }

        AZ_Error("MaterialCanvasDocument", false, "Saving generated file failed: %s\n", m_outputPath.c_str());
        return false;
    }

    void MaterialCanvasDocument::TemplateFileData::ReplaceLinesInBlock(
        const AZStd::string& blockBeginToken, const AZStd::string& blockEndToken, const LineGenerationFn& lineGenerationFn)
    {
        AZ_TracePrintf_IfTrue(
            "MaterialCanvasDocument",
            IsCompileLoggingEnabled(),
            "Inserting %s lines into template file: %s\n",
            blockBeginToken.c_str(),
            m_inputPath.c_str());

        auto blockBeginItr = AZStd::find_if(
            m_lines.begin(),
            m_lines.end(),
            [&blockBeginToken](const AZStd::string& line)
            {
                return AZ::StringFunc::Contains(line, blockBeginToken);
            });

        while (blockBeginItr != m_lines.end())
        {
            AZ_TracePrintf_IfTrue("MaterialCanvasDocument", IsCompileLoggingEnabled(), "*blockBegin: %s\n", (*blockBeginItr).c_str());

            // We have to insert one line at a time because AZStd::vector does not include a standard
            // range insert that returns an iterator
            const auto& linesToInsert = lineGenerationFn(*blockBeginItr);
            for (const auto& lineToInsert : linesToInsert)
            {
                ++blockBeginItr;
                blockBeginItr = m_lines.insert(blockBeginItr, lineToInsert);

                AZ_TracePrintf_IfTrue("MaterialCanvasDocument", IsCompileLoggingEnabled(), "lineToInsert: %s\n", lineToInsert.c_str());
            }

            if (linesToInsert.empty())
            {
                AZ_TracePrintf_IfTrue(
                    "MaterialCanvasDocument", IsCompileLoggingEnabled(), "Nothing was generated. This block will remain unmodified.\n");
            }

            ++blockBeginItr;

            // From the last line that was inserted, locate the end of the insertion block
            auto blockEndItr = AZStd::find_if(
                blockBeginItr,
                m_lines.end(),
                [&blockEndToken](const AZStd::string& line)
                {
                    return AZ::StringFunc::Contains(line, blockEndToken);
                });

            AZ_TracePrintf_IfTrue("MaterialCanvasDocument", IsCompileLoggingEnabled(), "*blockEnd: %s\n", (*blockEndItr).c_str());

            if (!linesToInsert.empty())
            {
                // If any new lines were inserted, erase pre-existing lines the template might have had between the begin and end blocks
                blockEndItr = m_lines.erase(blockBeginItr, blockEndItr);
            }

            // Search for another insertion point
            blockBeginItr = AZStd::find_if(
                blockEndItr,
                m_lines.end(),
                [&blockBeginToken](const AZStd::string& line)
                {
                    return AZ::StringFunc::Contains(line, blockBeginToken);
                });
        }
    }
} // namespace MaterialCanvas
