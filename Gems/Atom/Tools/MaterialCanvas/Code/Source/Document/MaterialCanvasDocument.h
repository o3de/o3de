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
#include <GraphModel/GraphModelBus.h>
#include <GraphModel/Model/GraphContext.h>
#include <GraphModel/Model/Node.h>

#include <AtomToolsFramework/Document/AtomToolsDocument.h>
#include <AtomToolsFramework/DynamicNode/DynamicNodeConfig.h>
#include <Document/MaterialCanvasDocumentRequestBus.h>

namespace MaterialCanvas
{
    //! MaterialCanvasDocument
    class MaterialCanvasDocument
        : public AtomToolsFramework::AtomToolsDocument
        , public MaterialCanvasDocumentRequestBus::Handler
        , public GraphModelIntegration::GraphControllerNotificationBus::Handler
    {
    public:
        AZ_RTTI(MaterialCanvasDocument, "{90299628-AD02-4FEB-9527-7278FA2817AD}", AtomToolsFramework::AtomToolsDocument);
        AZ_CLASS_ALLOCATOR(MaterialCanvasDocument, AZ::SystemAllocator, 0);
        AZ_DISABLE_COPY_MOVE(MaterialCanvasDocument);

        static void Reflect(AZ::ReflectContext* context);

        MaterialCanvasDocument() = default;
        MaterialCanvasDocument(
            const AZ::Crc32& toolId,
            const AtomToolsFramework::DocumentTypeInfo& documentTypeInfo,
            AZStd::shared_ptr<GraphModel::GraphContext> graphContext);
        virtual ~MaterialCanvasDocument();

        // AtomToolsFramework::AtomToolsDocument overrides...
        static AtomToolsFramework::DocumentTypeInfo BuildDocumentTypeInfo();
        AtomToolsFramework::DocumentObjectInfoVector GetObjectInfo() const override;
        bool Open(const AZStd::string& loadPath) override;
        bool Save() override;
        bool SaveAsCopy(const AZStd::string& savePath) override;
        bool SaveAsChild(const AZStd::string& savePath) override;
        bool IsOpen() const override;
        bool IsModified() const override;
        bool BeginEdit() override;
        bool EndEdit() override;

        // MaterialCanvasDocumentRequestBus::Handler overrides...
        GraphCanvas::GraphId GetGraphId() const override;
        const AZStd::vector<AZStd::string>& GetGeneratedFilePaths() const override;

    private:
        // AtomToolsFramework::AtomToolsDocument overrides...
        void Clear() override;
        bool ReopenRecordState() override;
        bool ReopenRestoreState() override;

        // GraphModelIntegration::GraphControllerNotificationBus::Handler overrides...
        void OnGraphModelRequestUndoPoint() override;
        void OnGraphModelTriggerUndo() override;
        void OnGraphModelTriggerRedo() override;

        void RecordGraphState();
        void RestoreGraphState(const AZStd::vector<AZ::u8>& graphState);

        void CreateGraph(GraphModel::GraphPtr graph);
        void DestroyGraph();

        AZStd::string GetGraphNamePrefixFromDocumentName() const;

        AZStd::string GetOutputPathFromTemplatePath(const AZStd::string& templatePath) const;

        AZStd::vector<GraphModel::ConstNodePtr> GetNodesInExecutionOrder() const;

        // Perform string substitutions on a container of strings.
        void ReplaceStringsInContainer(
            const AZStd::string& findText, const AZStd::string& replaceText, AZStd::vector<AZStd::string>& container) const;

        // Slots with special types, like color, need to be converted into a type compatible with shader code.
        AZStd::string ConvertSlotTypeToAZSL(const AZStd::string& slotTypeName) const;

        // Disconnected input and property slots need to have their values converted into a format that can be injected into the shader.
        AZStd::string ConvertSlotValueToAZSL(const AZStd::any& slotValue) const;

        // Each instruction block needs to have name substitutions performed on it. Variable names need to be unique per
        // node and will be prepended with the node ID. Slot types and values will be determined based on connections and
        // then substituted.
        void ReplaceStringsInSlotInstructions(
            GraphModel::ConstNodePtr node, GraphModel::ConstSlotPtr slot, AZStd::vector<AZStd::string>& instructionsForSlot) const;

        void AddSlotInstructions(
            GraphModel::ConstNodePtr node,
            const AtomToolsFramework::DynamicNodeSlotConfig& slotConfig,
            AZStd::vector<AZStd::string>& instructions) const;

        using LineGenerationFn = AZStd::function<AZStd::vector<AZStd::string>(const AZStd::string&)>;

        void ReplaceLinesInTemplateBlock(
            const AZStd::string& blockBeginToken,
            const AZStd::string& blockEndToken,
            const LineGenerationFn& lineGenerationFn,
            AZStd::vector<AZStd::string>& lines) const;

        bool ShouldUseInstructionsFromInputNode(
            GraphModel::ConstNodePtr targetNode,
            GraphModel::ConstNodePtr inputNode,
            const AZStd::vector<AZStd::string>& inputSlotNames) const;

        AZStd::vector<AZStd::string> GetInstructionsFromConnectedNodes(
            GraphModel::ConstNodePtr targetNode,
            const AZStd::vector<GraphModel::ConstNodePtr>& nodes,
            const AZStd::vector<AZStd::string>& inputSlotNames) const;

        bool CompileGraph() const;

        AZ::Entity* m_sceneEntity = {};
        GraphCanvas::GraphId m_graphId;
        GraphModel::GraphPtr m_graph;
        AZStd::shared_ptr<GraphModel::GraphContext> m_graphContext;
        AZStd::vector<AZ::u8> m_graphStateForUndoRedo;
        bool m_modified = {};
        mutable AZStd::vector<AZStd::string> m_generatedFiles;
    };
} // namespace MaterialCanvas
