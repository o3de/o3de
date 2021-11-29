/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/PlatformDef.h>
// qdatetime.h(331): warning C4251: 'QDateTime::d': class 'QSharedDataPointer<QDateTimePrivate>' needs to have dll-interface to be used by clients of class 'QDateTime'
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <QIcon>
#include <QTime>
#include <QTimer>
AZ_POP_DISABLE_WARNING

#include <AzCore/Component/NamedEntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/chrono/chrono.h>

#include <GraphCanvas/Widgets/GraphCanvasTreeItem.h>
#include <GraphCanvas/Components/StyleBus.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Core/Endpoint.h>

#include <Editor/View/Widgets/LoggingPanel/LoggingTypes.h>
#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>

namespace ScriptCanvasEditor
{
    class DebugLogFilter
    {
    public:
        QRegExp m_filter;

        bool IsEmpty() const
        {
            return m_filter.isEmpty();
        }

    };

    class DebugLogTreeItem
        : public GraphCanvas::GraphCanvasTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(DebugLogTreeItem, AZ::SystemAllocator, 0);
        AZ_RTTI(DebugLogTreeItem, "{E0B2A52B-47A4-40FF-A76F-4655125D01CC}", GraphCanvas::GraphCanvasTreeItem);

        enum Column
        {
            IndexForce = -1,

            NodeName,

            Input,
            Output,

            TimeStep,
            ScriptName,
            SourceEntity,

            Count
        };

        bool MatchesFilter(const DebugLogFilter& treeFilter);

        const ScriptCanvas::Endpoint& GetIncitingEndpoint() const;
        bool IsTriggeredBy(const ScriptCanvas::Endpoint& endpoint) const;

        Qt::ItemFlags Flags(const QModelIndex& index) const override final;
        int GetColumnCount() const override final;

    protected:

        void SetIncitingEndpoint(const ScriptCanvas::Endpoint& endpoint);

        virtual bool OnMatchesFilter(const DebugLogFilter& treeFilter) = 0;

    private:
        ScriptCanvas::Endpoint m_incitingEndpoint;
    };

    class ExecutionLogTreeItem;

    class DebugLogRootItem
        : public DebugLogTreeItem
    {
    public:
        enum UpdatePolicy
        {
            RealTime,
            Batched,
            SingleTime
        };

        AZ_CLASS_ALLOCATOR(DebugLogRootItem, AZ::SystemAllocator, 0);
        AZ_RTTI(DebugLogRootItem, "{CF59F72E-04AC-415C-A2F2-99D79564730B}", DebugLogTreeItem);

        DebugLogRootItem();
        ~DebugLogRootItem();

        ExecutionLogTreeItem* CreateExecutionItem(const LoggingDataId& loggingDataId, const ScriptCanvas::NodeTypeIdentifier& nodeType, const ScriptCanvas::GraphInfo& graphInfo, const ScriptCanvas::NamedNodeId& nodeId);

        QVariant Data(const QModelIndex& index, int role) const override final;

        void ResetData();

        void SetUpdatePolicy(UpdatePolicy updatePolicy);
        UpdatePolicy GetUpdatePolicy() const;

        void RedoLayout();

    protected:

        bool OnMatchesFilter([[maybe_unused]] const DebugLogFilter& treeFilter) override { return true; }

        UpdatePolicy m_updatePolicy;
        QTimer m_additionTimer;
    };

    class ExecutionLogTreeItem
        : public DebugLogTreeItem
        , public GraphCanvas::StyleManagerNotificationBus::Handler
        , public EditorGraphNotificationBus::Handler
        , public GeneralAssetNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ExecutionLogTreeItem, AZ::SystemAllocator, 0);
        AZ_RTTI(ExecutionLogTreeItem, "{71139142-A30C-4A16-81CC-D51314AEAF7D}", DebugLogTreeItem);

        ExecutionLogTreeItem(const LoggingDataId& loggingDataId, const ScriptCanvas::NodeTypeIdentifier& nodeType, const ScriptCanvas::GraphInfo& graphInfo, const ScriptCanvas::NamedNodeId& nodeId);
        ~ExecutionLogTreeItem() override = default;

        QVariant Data(const QModelIndex& index, int role) const override final;
        AZ::EntityId GetNodeId() const;

        void RegisterAnnotation(const ScriptCanvas::AnnotateNodeSignal& annotationSignal, bool allowAddSignal);

        void RegisterDataInput(const ScriptCanvas::Endpoint& incitingEndpoint, const ScriptCanvas::SlotId& slotId, AZStd::string_view slotName, AZStd::string_view dataString, bool allowAddSignal);
        void RegisterDataOutput(const ScriptCanvas::SlotId& slotId, AZStd::string_view slotName, AZStd::string_view dataString, bool allowAddSignal);

        void RegisterExecutionInput(const ScriptCanvas::Endpoint& incitingEndpoint, const ScriptCanvas::SlotId& slotId, AZStd::string_view slotName, AZStd::chrono::milliseconds relativeExecution);
        bool HasExecutionInput() const;

        void RegisterExecutionOutput(const ScriptCanvas::SlotId& slotId, AZStd::string_view slotName, AZStd::chrono::milliseconds relativeExecution);
        bool HasExecutionOutput() const;

        // GraphCanvas::StyleManagerNotificationBus
        void OnStylesUnloaded() override;
        void OnStylesLoaded() override;
        ////

        // GeneralNotificationsBus
        void OnAssetVisualized() override;
        void OnAssetUnloaded() override;
        ////

        // EditorGraphNotificationBus
        void OnGraphCanvasSceneDisplayed() override;
        ////

        const ScriptCanvas::GraphIdentifier& GetGraphIdentifier() const;
        const AZ::Data::AssetId& GetAssetId() const;

        AZ::EntityId        GetScriptCanvasAssetNodeId() const;
        GraphCanvas::NodeId GetGraphCanvasNodeId() const;

    protected:

        bool OnMatchesFilter(const DebugLogFilter& treeFilter) override;

    private:

        void ResolveWrapperNode(bool refreshData = true);

        void ScrapeBehaviorContextData();
        void ScrapeGraphCanvasData();

        void PopulateInputSlotData();
        void PopulateOutputSlotData();

        LoggingDataId                       m_loggingDataId;
        ScriptCanvas::NodeTypeIdentifier    m_nodeType;
        ScriptCanvas::GraphInfo             m_graphInfo;
        QString                             m_sourceEntityName;
        QString                             m_graphName;
        QString                             m_relativeGraphPath;

        AZ::EntityId m_graphCanvasGraphId;

        AZ::EntityId m_scriptCanvasAssetNodeId;

        AZ::EntityId m_scriptCanvasNodeId;
        GraphCanvas::NodeId m_graphCanvasNodeId;

        QString m_displayName;

        ScriptCanvas::SlotId m_inputSlot;
        QString              m_inputName;

        ScriptCanvas::SlotId m_outputSlot;
        QString              m_outputName;

        QString m_timeString;

        GraphCanvas::PaletteIconConfiguration m_paletteConfiguration;
        const QPixmap* m_iconPixmap;
    };

    class DataLogTreeItem
        : public DebugLogTreeItem
    {
        friend class ExecutionLogTreeItem;
    public:
        AZ_CLASS_ALLOCATOR(DataLogTreeItem, AZ::SystemAllocator, 0);
        AZ_RTTI(DataLogTreeItem, "{04D997AD-E3CA-47CA-9810-8814B36AB726}", DebugLogTreeItem);

        DataLogTreeItem(const ScriptCanvas::GraphIdentifier& graphIdentifier);

        QVariant Data(const QModelIndex& index, int role) const override final;

        void RegisterDataInput(const ScriptCanvas::Endpoint& incitingEndpoint, const ScriptCanvas::Endpoint& endpoint, AZStd::string_view slotName, AZStd::string_view data);
        bool HasInput() const;

        void RegisterDataOutput(const ScriptCanvas::Endpoint& endpoint, AZStd::string_view slotName, AZStd::string_view dataString);
        bool HasOutput() const;

    protected:

        bool OnMatchesFilter(const DebugLogFilter& treeFilter) override;
        bool LessThan(const GraphCanvas::GraphCanvasTreeItem* graphItem) const override;

    private:

        AZ::Data::AssetId GetAssetId() const;

        void ScrapeData();
        void InvalidateEditorIds();
        void InvalidateGraphCanvasIds();

        void ScrapeInputName();
        void ScrapeOutputName();

        ScriptCanvas::GraphIdentifier m_graphIdentifier;
        GraphCanvas::GraphId          m_graphCanvasGraphId;

        ScriptCanvas::Endpoint m_assetInputEndpoint;
        QString          m_inputName;
        QString          m_inputData;

        ScriptCanvas::Endpoint m_assetOutputEndpoint;
        QString          m_outputName;
        QString          m_outputData;
    };

    class NodeAnnotationTreeItem
        : public DebugLogTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(NodeAnnotationTreeItem, AZ::SystemAllocator, 0);
        AZ_RTTI(NodeAnnotationTreeItem, "{4A052945-F8D1-4A96-8D52-D8C20504E30F}", DebugLogTreeItem);

        NodeAnnotationTreeItem();
        NodeAnnotationTreeItem(ScriptCanvas::AnnotateNodeSignal::AnnotationLevel annotationLevel, const AZStd::string& annotation);

        ~NodeAnnotationTreeItem() override = default;

        QVariant Data(const QModelIndex& index, int role) const override final;

    protected:

        bool OnMatchesFilter(const DebugLogFilter& treeFilter) override;

    private:

        ScriptCanvas::AnnotateNodeSignal::AnnotationLevel m_annotationLevel;
        QString m_annotation;

        QIcon m_annotationIcon;
    };
}
