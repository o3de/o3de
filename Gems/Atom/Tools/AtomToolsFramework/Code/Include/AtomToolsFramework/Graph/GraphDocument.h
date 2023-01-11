/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphModel/GraphModelBus.h>
#include <GraphModel/Model/GraphContext.h>
#include <GraphModel/Model/Node.h>

#include <AtomToolsFramework/Document/AtomToolsDocument.h>
#include <AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNodeConfig.h>
#include <AtomToolsFramework/Graph/GraphDocumentRequestBus.h>

namespace AtomToolsFramework
{
    //! GraphDocument implements support for creating, loading, saving, and manipulating graph model and canvas graphs
    class GraphDocument
        : public AtomToolsDocument
        , public GraphDocumentRequestBus::Handler
        , public GraphModelIntegration::GraphControllerNotificationBus::Handler
        , public GraphCanvas::SceneNotificationBus::Handler
    {
    public:
        AZ_RTTI(GraphDocument, "{7AFB5F8B-2E83-47E6-9DC8-AB70E0194D3E}", AtomToolsDocument);
        AZ_CLASS_ALLOCATOR(GraphDocument, AZ::SystemAllocator, 0);
        AZ_DISABLE_COPY_MOVE(GraphDocument);

        static void Reflect(AZ::ReflectContext* context);

        GraphDocument() = default;
        GraphDocument(
            const AZ::Crc32& toolId,
            const DocumentTypeInfo& documentTypeInfo,
            AZStd::shared_ptr<GraphModel::GraphContext> graphContext);
        virtual ~GraphDocument();

        // AtomToolsDocument overrides...
        static DocumentTypeInfo BuildDocumentTypeInfo(
            const AZStd::string& documentTypeName,
            const AZStd::vector<AZStd::string>& documentTypeExtensions,
            const AZStd::vector<AZStd::string>& documentTypeTemplateExtensions,
            const AZStd::string& defaultDocumentTypeTemplatePath,
            AZStd::shared_ptr<GraphModel::GraphContext> graphContext);

        DocumentObjectInfoVector GetObjectInfo() const override;
        bool Open(const AZStd::string& loadPath) override;
        bool Save() override;
        bool SaveAsCopy(const AZStd::string& savePath) override;
        bool SaveAsChild(const AZStd::string& savePath) override;
        bool IsModified() const override;
        bool BeginEdit() override;
        bool EndEdit() override;
        void Clear() override;

        // GraphDocumentRequestBus::Handler overrides...
        GraphModel::GraphPtr GetGraph() const override;
        GraphCanvas::GraphId GetGraphId() const override;
        AZStd::string GetGraphName() const override;

    private:
        // GraphModelIntegration::GraphControllerNotificationBus::Handler overrides...
        void OnGraphModelSlotModified(GraphModel::SlotPtr slot) override;
        void OnGraphModelRequestUndoPoint() override;
        void OnGraphModelTriggerUndo() override;
        void OnGraphModelTriggerRedo() override;

        // GraphCanvas::SceneNotificationBus::Handler overrides...
        void OnSelectionChanged() override;

        void RecordGraphState();
        void RestoreGraphState(const AZStd::vector<AZ::u8>& graphState);

        void CreateGraph(GraphModel::GraphPtr graph);
        void DestroyGraph();

        void BuildEditablePropertyGroups();

        AZ::Entity* m_sceneEntity = {};
        GraphCanvas::GraphId m_graphId;
        GraphModel::GraphPtr m_graph;
        AZStd::shared_ptr<GraphModel::GraphContext> m_graphContext;
        AZStd::vector<AZ::u8> m_graphStateForUndoRedo;
        bool m_modified = {};

        // A container of root level dynamic property groups that represents the reflected, editable data within the document.
        // These groups will be mapped to document object info so they can populate and be edited directly in the inspector.
        AZStd::vector<AZStd::shared_ptr<DynamicPropertyGroup>> m_groups;
    };
} // namespace AtomToolsFramework
