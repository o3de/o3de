/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Graph/GraphDocument.h>
#include <AtomToolsFramework/Graph/GraphDocumentNotificationBus.h>
#include <AtomToolsFramework/Graph/GraphUtil.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Jobs/JobFunction.h>
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
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
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
                ->Event("GetGraphName", &GraphDocumentRequests::GetGraphName)
                ->Event("SetGeneratedFilePaths", &GraphDocumentRequests::SetGeneratedFilePaths)
                ->Event("GetGeneratedFilePaths", &GraphDocumentRequests::GetGeneratedFilePaths)
                ->Event("CompileGraph", &GraphDocumentRequests::CompileGraph)
                ->Event("QueueCompileGraph", &GraphDocumentRequests::QueueCompileGraph)
                ->Event("IsCompileGraphQueued", &GraphDocumentRequests::IsCompileGraphQueued)
                ;
        }
    }

    GraphDocument::GraphDocument(
        const AZ::Crc32& toolId,
        const DocumentTypeInfo& documentTypeInfo,
        AZStd::shared_ptr<GraphModel::GraphContext> graphContext,
        AZStd::shared_ptr<GraphCompiler> graphCompiler)
        : AtomToolsDocument(toolId, documentTypeInfo)
        , m_graphContext(graphContext)
        , m_graphCompiler(graphCompiler)
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
        AZ::SystemTickBus::Handler::BusConnect();

        m_graphCompiler->SetStateChangeHandler(
            [toolId, documentId = m_id](const GraphCompiler* graphCompiler)
            {
                AZ::SystemTickBus::QueueFunction(
                    [toolId, documentId, state = graphCompiler->GetState(), generatedFiles = graphCompiler->GetGeneratedFilePaths()]()
                    {
                        switch (state)
                        {
                        case GraphCompiler::State::Idle:
                            break;
                        case GraphCompiler::State::Compiling:
                            GraphDocumentRequestBus::Event(
                                documentId, &GraphDocumentRequestBus::Events::SetGeneratedFilePaths, AZStd::vector<AZStd::string>{});
                            GraphDocumentNotificationBus::Event(
                                toolId, &GraphDocumentNotificationBus::Events::OnCompileGraphStarted, documentId);
                            break;
                        case GraphCompiler::State::Processing:
                            break;
                        case GraphCompiler::State::Complete:
                            GraphDocumentRequestBus::Event(
                                documentId, &GraphDocumentRequestBus::Events::SetGeneratedFilePaths, generatedFiles);
                            GraphDocumentNotificationBus::Event(
                                toolId, &GraphDocumentNotificationBus::Events::OnCompileGraphCompleted, documentId);
                            break;
                        case GraphCompiler::State::Failed:
                            GraphDocumentNotificationBus::Event(
                                toolId, &GraphDocumentNotificationBus::Events::OnCompileGraphFailed, documentId);
                            break;
                        case GraphCompiler::State::Canceled:
                            break;
                        }
                    });
            });
    }

    GraphDocument::~GraphDocument()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
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
        AZStd::shared_ptr<GraphModel::GraphContext> graphContext,
        AZStd::function<AZStd::shared_ptr<GraphCompiler>()> graphCompilerCreateFn)
    {
        DocumentTypeInfo documentType;
        documentType.m_documentTypeName = documentTypeName;
        documentType.m_documentFactoryCallback = [graphContext, graphCompilerCreateFn](const AZ::Crc32& toolId, const DocumentTypeInfo& documentTypeInfo) {
            return aznew GraphDocument(
                toolId, documentTypeInfo, graphContext, graphCompilerCreateFn ? graphCompilerCreateFn() : AZStd::shared_ptr<GraphCompiler>());
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

        // Build a container of reflected object info specifically for the specialized graph canvas nodes that are not covered by graph model.
        DocumentObjectInfoVector objectInfoForGraphCanvasNodes = GetObjectInfoForGraphCanvasNodes();
        objects.insert(objects.end(), objectInfoForGraphCanvasNodes.begin(), objectInfoForGraphCanvasNodes.end());

        // Reserve and register reflected objects for all of the property group in the document.
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
        m_compileGraphQueued |= GetSettingsValue("/O3DE/AtomToolsFramework/GraphCompiler/CompileOnOpen", true);
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
        m_compileGraphQueued |= GetSettingsValue("/O3DE/AtomToolsFramework/GraphCompiler/CompileOnSave", true);
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
        m_compileGraphQueued |= GetSettingsValue("/O3DE/AtomToolsFramework/GraphCompiler/CompileOnSave", true);
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
        m_compileGraphQueued |= GetSettingsValue("/O3DE/AtomToolsFramework/GraphCompiler/CompileOnSave", true);
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
            m_compileGraphQueued |= GetSettingsValue("/O3DE/AtomToolsFramework/GraphCompiler/CompileOnEdit", true);
        }
        return true;
    }

    void GraphDocument::Clear()
    {
        DestroyGraph();
        m_graphStateForUndoRedo.clear();
        m_groups.clear();
        m_modified = false;
        AtomToolsDocument::Clear();
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

    void GraphDocument::SetGeneratedFilePaths(const AZStd::vector<AZStd::string>& pathas)
    {
        m_generatedFiles = pathas;
    }

    const AZStd::vector<AZStd::string>& GraphDocument::GetGeneratedFilePaths() const
    {
        return m_generatedFiles;
    }

    bool GraphDocument::CompileGraph()
    {
        // If a compiler was supplied But not in a state that can be reinitialized then return failure. If compiling was queued, attempts
        // will continue to be made until the background compilation job is cancelled or complete.
        if (!m_graphCompiler || !m_graphCompiler->Reset())
        {
            return false;
        }

        m_compileGraphQueued = false;

        // Serialize the graph data into a buffer that's copied and deserialized in the compilation job. This will allow
        // editing to continue while the last serialized version of the graph is compiled in the background.
        AZStd::vector<AZ::u8> graphBuffer;
        AZ::IO::ByteContainerStream<decltype(graphBuffer)> graphBufferStream(&graphBuffer);
        AZ::Utils::SaveObjectToStream(graphBufferStream, AZ::ObjectStream::ST_BINARY, m_graph.get());

        auto compileJobFn = [graphBuffer,
                             graphCompiler = m_graphCompiler,
                             graphContext = m_graphContext,
                             graphName = GetGraphName(),
                             graphPath = GetAbsolutePath()]()
        {
            // Deserialize the buffer to create a copy of the graph that can be safely transformed from the job thread.
            GraphModel::GraphPtr graph = AZStd::make_shared<GraphModel::Graph>(graphContext);
            AZ::Utils::LoadObjectFromBufferInPlace(graphBuffer.data(), graphBuffer.size(), *graph.get());
            graph->PostLoadSetup(graphContext);

            graphCompiler->CompileGraph(graph, graphName, graphPath);
        };

        auto job = AZ::CreateJobFunction(compileJobFn, true);
        job->Start();
        return true;
    }

    void GraphDocument::QueueCompileGraph()
    {
        m_compileGraphQueued = true;
    }

    bool GraphDocument::IsCompileGraphQueued() const
    {
        return m_compileGraphQueued;
    }

    void GraphDocument::OnSystemTick()
    {
        if (m_buildPropertiesQueued)
        {
            BuildEditablePropertyGroups();
        }

        if (IsCompileGraphQueued())
        {
            if (m_compileGraphQueueTime <= AZStd::chrono::steady_clock::now())
            {
                if (CompileGraph())
                {
                    const AZ::u64 intervalMs =
                        GetSettingsValue("/O3DE/AtomToolsFramework/GraphCompiler/QueueGraphCompileIntervalMs", (AZ::u64)500);
                    m_compileGraphQueueTime = AZStd::chrono::steady_clock::now() + AZStd::chrono::milliseconds(intervalMs);
                }
            }
        }
    }

    void GraphDocument::OnGraphModelSlotModified([[maybe_unused]] GraphModel::SlotPtr slot)
    {
        m_modified = true;
        m_buildPropertiesQueued = true;
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
            m_buildPropertiesQueued = true;
            AtomToolsDocumentNotificationBus::Event(m_toolId, &AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
            m_compileGraphQueued |= GetSettingsValue("/O3DE/AtomToolsFramework/GraphCompiler/CompileOnEdit", true);
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
        m_buildPropertiesQueued = true;
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
        GraphModel::GraphPtr graph = AZStd::make_shared<GraphModel::Graph>(m_graphContext);
        AZ::Utils::LoadObjectFromBufferInPlace(m_graphStateForUndoRedo.data(), m_graphStateForUndoRedo.size(), *graph.get());

        m_modified = true;
        CreateGraph(graph);
        AtomToolsDocumentNotificationBus::Event(m_toolId, &AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
        m_compileGraphQueued |= GetSettingsValue("/O3DE/AtomToolsFramework/GraphCompiler/CompileOnEdit", true);
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
            m_buildPropertiesQueued = true;
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
        m_buildPropertiesQueued = false;

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
                        propertyConfig.m_dataChangeCallback = [currentSlot, graphId = m_graphId](const AZStd::any& value)
                        {
                            currentSlot->SetValue(value);

                            // Retrieve and refresh the node property displays with the updated slot value.
                            GraphCanvas::SlotId slotId{};
                            GraphModelIntegration::GraphControllerRequestBus::EventResult(
                                slotId, graphId, &GraphModelIntegration::GraphControllerRequests::GetSlotIdBySlot, currentSlot);

                            GraphCanvas::NodePropertyRequestBus::Event(slotId, [](GraphCanvas::NodePropertyRequests* nodePropertyRequests) {
                                if (auto display = nodePropertyRequests->GetNodePropertyDisplay())
                                {
                                    display->UpdateDisplay();
                                }
                            });
                            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
                        };

                        group->m_properties.emplace_back(AZStd::move(propertyConfig));
                    }
                }
            }

            m_groups.emplace_back(group);
        }

        AtomToolsDocumentNotificationBus::Event(m_toolId, &AtomToolsDocumentNotificationBus::Events::OnDocumentObjectInfoInvalidated, m_id);
    }

    DocumentObjectInfoVector GraphDocument::GetObjectInfoForGraphCanvasNodes() const
    {
        DocumentObjectInfoVector objects;

        // Reserve and register reflected objects for all of the selected graph canvas nodes that do not mirror any of the graph model nodes
        // that have been added to the graph. This should cover bookmarks, comments, and groups.
        AZStd::vector<AZ::EntityId> selectedItems;
        GraphCanvas::SceneRequestBus::EventResult(selectedItems, m_graphId, &GraphCanvas::SceneRequests::GetSelectedItems);

        // Optimizing the container to only have nodes with property components before sorting.
        AZStd::erase_if(
            selectedItems,
            [](const auto& selectedItem)
            {
                return GraphCanvas::GraphCanvasPropertyBus::FindFirstHandler(selectedItem) == nullptr;
            });

        objects.reserve(objects.size() + selectedItems.size());

        // The order that selected nodes appear in the container is not deterministic. To compensate for this, we sort by position to ensure
        // that nodes always appear in the inspector in a consistent order.
        AZStd::sort(
            selectedItems.begin(),
            selectedItems.end(),
            [](const auto& selectedItem1, const auto& selectedItem2)
            {
                AZ::Vector2 selectedItemPosition1{};
                GraphCanvas::GeometryRequestBus::EventResult(
                    selectedItemPosition1, selectedItem1, &GraphCanvas::GeometryRequests::GetPosition);

                AZ::Vector2 selectedItemPosition2{};
                GraphCanvas::GeometryRequestBus::EventResult(
                    selectedItemPosition2, selectedItem2, &GraphCanvas::GeometryRequests::GetPosition);

                return selectedItemPosition1.IsLessThan(selectedItemPosition2);
            });

        // Some graph canvas node property components do not have any visible properties, like the bookmark anchor visual component. These
        // will not be added to the graph document inspector.
        const AZStd::unordered_set<AZ::Uuid> ignoredTypeIds{
            AZ::Uuid("{AD921E77-962B-417F-88FB-500FA679DFDF}") // BookmarkAnchorVisualComponent
        };

        // After all of the selected graph canvas nodes have been sorted, search for those with editable property components and add them to
        // the list of reflected objects.
        for (const auto& selectedItem : selectedItems)
        {
            // Some graph canvas nodes have multiple editable property components, like groups and bookmarks. All of the property components
            // will be added in relative order except for those in the ignore list.
            DocumentObjectInfoVector selectedItemObjects;
            GraphCanvas::GraphCanvasPropertyBus::EnumerateHandlersId(
                selectedItem,
                [&](GraphCanvas::GraphCanvasPropertyInterface* propertyInterface) -> bool
                {
                    AZ::Component* component = propertyInterface->GetPropertyComponent();
                    if (AzToolsFramework::ShouldInspectorShowComponent(component) && !ignoredTypeIds.contains(component->RTTI_GetType()))
                    {
                        DocumentObjectInfo objectInfo;
                        objectInfo.m_visible = true;
                        objectInfo.m_name = GetSymbolNameFromText(component->RTTI_GetTypeName());
                        objectInfo.m_displayName = objectInfo.m_description = GetDisplayNameFromText(component->RTTI_GetTypeName());
                        objectInfo.m_objectType = component->RTTI_GetType();
                        objectInfo.m_objectPtr = component;
                        selectedItemObjects.emplace_back(AZStd::move(objectInfo));
                    }

                    // Continue enumeration.
                    return true;
                });

            // In addition to presorting nodes by position we will sort all of the property components by name to guarantee a consistent
            // order in the inspector.
            AZStd::sort(
                selectedItemObjects.begin(),
                selectedItemObjects.end(),
                [](const auto& objectInfo1, const auto& objectInfo2)
                {
                    return objectInfo1.m_displayName < objectInfo2.m_displayName;
                });

            objects.insert(objects.end(), selectedItemObjects.begin(), selectedItemObjects.end());
        }

        return objects;
    }
} // namespace AtomToolsFramework
