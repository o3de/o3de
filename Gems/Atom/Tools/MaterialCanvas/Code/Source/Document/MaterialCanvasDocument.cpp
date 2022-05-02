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
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Document/MaterialCanvasDocument.h>
#include <Document/MaterialCanvasGraphDataTypes.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/GraphCanvasBus.h>

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

        // Listen for GraphController notifications on the new graph.
        GraphModelIntegration::GraphControllerNotificationBus::Handler::BusConnect(m_graphId);

        MaterialCanvasDocumentRequestBus::Handler::BusConnect(m_id);
    }

    MaterialCanvasDocument::~MaterialCanvasDocument()
    {
        MaterialCanvasDocumentRequestBus::Handler::BusDisconnect();

        // Stop listening for GraphController notifications for this graph.
        GraphModelIntegration::GraphControllerNotificationBus::Handler::BusDisconnect();

        // Remove the controller for this graph.
        GraphModelIntegration::GraphManagerRequestBus::Broadcast(
            &GraphModelIntegration::GraphManagerRequests::DeleteGraphController, m_graphId);

        m_graph.reset();

        delete m_sceneEntity;
    }

    AtomToolsFramework::DocumentTypeInfo MaterialCanvasDocument::BuildDocumentTypeInfo()
    {
        // Setting up placeholder document type info and extensions. This is not representative of final data.
        AtomToolsFramework::DocumentTypeInfo documentType;
        documentType.m_documentTypeName = "Material Canvas";
        documentType.m_documentFactoryCallback = [](const AZ::Crc32& toolId, const AtomToolsFramework::DocumentTypeInfo& documentTypeInfo) {
            // Creating a graph context per document by default but this can and probably should be overridden in the application to provide
            // a shared context.
            auto graphContext = AZStd::make_shared<GraphModel::GraphContext>("Material Canvas", ".materialcanvas", CreateAllDataTypes());
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

        m_graph.reset(serializeContext->CloneObject(AZStd::any_cast<const GraphModel::Graph>(&loadResult.GetValue())));
        m_graph->PostLoadSetup(m_graphContext);

        GraphModelIntegration::GraphManagerRequestBus::Broadcast(
            &GraphModelIntegration::GraphManagerRequests::CreateGraphController, m_graphId, m_graph);
        GraphModelIntegration::GraphControllerNotificationBus::Handler::BusConnect(m_graphId);
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

        return SaveSucceeded();
    }

    bool MaterialCanvasDocument::IsOpen() const
    {
        return AtomToolsDocument::IsOpen() && m_graph && m_graphId.IsValid();
    }

    bool MaterialCanvasDocument::IsModified() const
    {
        bool result = false;
        return result;
    }

    bool MaterialCanvasDocument::BeginEdit()
    {
        // Save the current properties as a momento for undo before any changes are applied
        return true;
    }

    bool MaterialCanvasDocument::EndEdit()
    {
        return true;
    }

    // MaterialCanvasDocumentRequestBus::Handler overrides...

    inline GraphCanvas::GraphId MaterialCanvasDocument::GetGraphId() const
    {
        return m_graphId;
    }

    void MaterialCanvasDocument::Clear()
    {
        GraphModelIntegration::GraphControllerNotificationBus::Handler::BusDisconnect();
        GraphModelIntegration::GraphManagerRequestBus::Broadcast(
            &GraphModelIntegration::GraphManagerRequests::DeleteGraphController, m_graphId);

        m_graph.reset();

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
} // namespace MaterialCanvas
