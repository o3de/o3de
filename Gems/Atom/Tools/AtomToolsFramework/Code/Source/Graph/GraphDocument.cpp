/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Graph/GraphDocument.h>
#include <AtomToolsFramework/Graph/GraphUtil.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/sort.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphModel/Model/Connection.h>
#include <GraphModel/Model/Graph.h>

namespace AtomToolsFramework
{
    void GraphDocument::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<GraphDocument, AtomToolsDocument>()
                ->Version(0);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<GraphDocumentRequestBus>("GraphDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "atomtools")
                ->Event("GetGraph", &GraphDocumentRequests::GetGraph)
                ->Event("GetGraphId", &GraphDocumentRequests::GetGraphId)
                ->Event("GetGraphName", &GraphDocumentRequests::GetGraphName);
        }
    }

    GraphDocument::GraphDocument(
        const AZ::Crc32& toolId,
        const DocumentTypeInfo& documentTypeInfo,
        AZStd::shared_ptr<GraphModel::GraphContext> graphContext)
        : AtomToolsDocument(toolId, documentTypeInfo)
        , m_graphContext(graphContext)
    {
        AZ_Assert(m_graphContext, "Graph context must be valid in order to create a graph document.");

        // Creating the scene entity and graph for this document. This may end up moving to the view.
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
        GraphDocumentRequestBus::Handler::BusConnect(m_id);
    }

    GraphDocument::~GraphDocument()
    {
        GraphDocumentRequestBus::Handler::BusDisconnect();
        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
        GraphModelIntegration::GraphControllerNotificationBus::Handler::BusDisconnect();

        DestroyGraph();

        m_graphId = GraphCanvas::GraphId();
        delete m_sceneEntity;
        m_sceneEntity = {};
    }

    DocumentTypeInfo GraphDocument::BuildDocumentTypeInfo(
        const AZStd::string& documentTypeName,
        const AZStd::vector<AZStd::string>& documentTypeExtensions,
        const AZStd::vector<AZStd::string>& documentTypeTemplateExtensions,
        const AZStd::string& defaultDocumentTypeTemplatePath,
        AZStd::shared_ptr<GraphModel::GraphContext> graphContext)
    {
        DocumentTypeInfo documentType;
        documentType.m_documentTypeName = documentTypeName;
        documentType.m_documentFactoryCallback = [graphContext](const AZ::Crc32& toolId, const DocumentTypeInfo& documentTypeInfo)
        {
            return aznew GraphDocument(toolId, documentTypeInfo, graphContext);
        };

        for (const auto& extension : documentTypeExtensions)
        {
            documentType.m_supportedExtensionsToOpen.push_back({ documentTypeName, extension });
            documentType.m_supportedExtensionsToSave.push_back({ documentTypeName, extension });
        }

        for (const auto& extension : documentTypeTemplateExtensions)
        {
            documentType.m_supportedExtensionsToCreate.push_back({ documentTypeName + " Template", extension });
        }

        documentType.m_defaultDocumentTemplate = defaultDocumentTypeTemplatePath;

        return documentType;
    }

    DocumentObjectInfoVector GraphDocument::GetObjectInfo() const
    {
        DocumentObjectInfoVector objects = AtomToolsDocument::GetObjectInfo();
        objects.reserve(objects.size() + m_groups.size());

        for (const auto& group : m_groups)
        {
            if (!group->m_properties.empty())
            {
                DocumentObjectInfo objectInfo;
                objectInfo.m_visible = group->m_visible;
                objectInfo.m_name = group->m_name;
                objectInfo.m_displayName = group->m_displayName;
                objectInfo.m_description = group->m_description;
                objectInfo.m_objectType = azrtti_typeid<DynamicPropertyGroup>();
                objectInfo.m_objectPtr = const_cast<DynamicPropertyGroup*>(group.get());
                objectInfo.m_nodeIndicatorFunction = [](const AzToolsFramework::InstanceDataNode* /*node*/)
                {
                    // There are currently no indicators for graph nodes.
                    return nullptr;
                };
                objects.emplace_back(AZStd::move(objectInfo));
            }
        }

        return objects;
    }

    bool GraphDocument::Open(const AZStd::string& loadPath)
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
        return OpenSucceeded();
    }

    bool GraphDocument::Save()
    {
        if (!AtomToolsDocument::Save())
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        AZ_Error("GraphDocument", m_graph, "Attempting to save invalid graph object.");
        if (!m_graph || !AZ::JsonSerializationUtils::SaveObjectToFile(m_graph.get(), m_savePathNormalized))
        {
            return SaveFailed();
        }

        m_modified = false;
        m_absolutePath = m_savePathNormalized;
        return SaveSucceeded();
    }

    bool GraphDocument::SaveAsCopy(const AZStd::string& savePath)
    {
        if (!AtomToolsDocument::SaveAsCopy(savePath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        AZ_Error("GraphDocument", m_graph, "Attempting to save invalid graph object.");
        if (!m_graph || !AZ::JsonSerializationUtils::SaveObjectToFile(m_graph.get(), m_savePathNormalized))
        {
            return SaveFailed();
        }

        m_modified = false;
        m_absolutePath = m_savePathNormalized;
        return SaveSucceeded();
    }

    bool GraphDocument::SaveAsChild(const AZStd::string& savePath)
    {
        if (!AtomToolsDocument::SaveAsChild(savePath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        AZ_Error("GraphDocument", m_graph, "Attempting to save invalid graph object. ");
        if (!m_graph || !AZ::JsonSerializationUtils::SaveObjectToFile(m_graph.get(), m_savePathNormalized))
        {
            return SaveFailed();
        }

        m_modified = false;
        m_absolutePath = m_savePathNormalized;
        return SaveSucceeded();
    }

    bool GraphDocument::IsModified() const
    {
        return m_modified;
    }

    bool GraphDocument::BeginEdit()
    {
        RecordGraphState();
        return true;
    }

    bool GraphDocument::EndEdit()
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
            AtomToolsDocumentNotificationBus::Event(m_toolId, &AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
            GraphCanvas::ViewRequestBus::Event(m_graphId, &GraphCanvas::ViewRequests::RefreshView);
        }
        return true;
    }

    GraphModel::GraphPtr GraphDocument::GetGraph() const
    {
        return m_graph;
    }

    GraphCanvas::GraphId GraphDocument::GetGraphId() const
    {
        return m_graphId;
    }

    AZStd::string GraphDocument::GetGraphName() const
    {
        if (m_absolutePath.empty())
        {
            return "untitled";
        }

        // Sanitize the document name to remove any illegal characters that could not be used as symbols in generated code
        AZStd::string documentName;
        AZ::StringFunc::Path::GetFileName(m_absolutePath.c_str(), documentName);
        return GetSymbolNameFromText(documentName);
    }

    void GraphDocument::Clear()
    {
        DestroyGraph();
        m_graphStateForUndoRedo.clear();
        m_groups.clear();
        m_modified = false;

        AtomToolsDocument::Clear();
    }

    void GraphDocument::OnGraphModelSlotModified([[maybe_unused]] GraphModel::SlotPtr slot)
    {
        m_modified = true;
        BuildEditablePropertyGroups();
        AtomToolsDocumentNotificationBus::Event(m_toolId, &AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
    }

    void GraphDocument::OnGraphModelRequestUndoPoint()
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
            AtomToolsDocumentNotificationBus::Event(m_toolId, &AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
        }
    }

    void GraphDocument::OnGraphModelTriggerUndo()
    {
        Undo();
    }

    void GraphDocument::OnGraphModelTriggerRedo()
    {
        Redo();
    }

    void GraphDocument::OnSelectionChanged()
    {
        BuildEditablePropertyGroups();
    }

    void GraphDocument::RecordGraphState()
    {
        // Forcing all of the graph model metadata to be updated before serializing to the binary stream. This will ensure that data for
        // bookmarks, comments, and groups is recorded.
        GraphCanvas::GraphModelRequestBus::Event(m_graphId, &GraphCanvas::GraphModelRequests::OnSaveDataDirtied, m_graphId);

        // Serialize the current graph to a byte stream so that it can be restored with undo redo operations.
        m_graphStateForUndoRedo.clear();
        AZ::IO::ByteContainerStream<decltype(m_graphStateForUndoRedo)> undoGraphStateStream(&m_graphStateForUndoRedo);
        AZ::Utils::SaveObjectToStream(undoGraphStateStream, AZ::ObjectStream::ST_BINARY, m_graph.get());
    }

    void GraphDocument::RestoreGraphState(const AZStd::vector<AZ::u8>& graphState)
    {
        // Restore a version of the graph that was previously serialized to a byte stream
        m_graphStateForUndoRedo = graphState;
        AZ::IO::ByteContainerStream<decltype(m_graphStateForUndoRedo)> undoGraphStateStream(&m_graphStateForUndoRedo);

        GraphModel::GraphPtr graph = AZStd::make_shared<GraphModel::Graph>(m_graphContext);
        AZ::Utils::LoadObjectFromStreamInPlace(undoGraphStateStream, *graph.get());

        m_modified = true;
        CreateGraph(graph);
        AtomToolsDocumentNotificationBus::Event(m_toolId, &AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
    }

    void GraphDocument::CreateGraph(GraphModel::GraphPtr graph)
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

    void GraphDocument::DestroyGraph()
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

    void GraphDocument::BuildEditablePropertyGroups()
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
            // Create a new property group and set up the header to match the node
            AZStd::shared_ptr<DynamicPropertyGroup> group;
            group.reset(aznew DynamicPropertyGroup);
            group->m_displayName = GetDisplayNameFromText(AZStd::string::format("Node%u %s", currentNode->GetId(), currentNode->GetTitle()));
            group->m_name = GetSymbolNameFromText(group->m_displayName);
            group->m_description = currentNode->GetSubTitle();
            group->m_properties.reserve(currentNode->GetSlotDefinitions().size());

            // Visit all of the slots in the order to add properties to the container for the inspector.
            for (const auto& slotDefinition : currentNode->GetSlotDefinitions())
            {
                if (auto currentSlot = currentNode->GetSlot(slotDefinition->GetName()))
                {
                    if (currentSlot->GetSlotDirection() == GraphModel::SlotDirection::Input)
                    {
                        // Create and add a dynamic property for each input slot on the node
                        DynamicPropertyConfig propertyConfig;
                        propertyConfig.m_id = currentSlot->GetName();
                        propertyConfig.m_name = currentSlot->GetName();
                        propertyConfig.m_displayName = currentSlot->GetDisplayName();
                        propertyConfig.m_groupName = group->m_name;
                        propertyConfig.m_groupDisplayName = group->m_displayName;
                        propertyConfig.m_description = currentSlot->GetDescription();
                        propertyConfig.m_enumValues = currentSlot->GetEnumValues();
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
            }

            m_groups.emplace_back(group);
        }

        AtomToolsDocumentNotificationBus::Event(m_toolId, &AtomToolsDocumentNotificationBus::Events::OnDocumentObjectInfoInvalidated, m_id);
    };
} // namespace AtomToolsFramework
