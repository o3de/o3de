/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/unordered_map.h>

#include <Editor/View/Widgets/LoggingPanel/PivotTree/PivotTreeWidget.h>

#include <Editor/View/Widgets/LoggingPanel/LoggingDataAggregator.h>
#endif

namespace ScriptCanvasEditor
{
    class EntityPivotTreeGraphItem
        : public PivotTreeGraphItem
    {
        friend class EntityPivotTreeEntityItem;
    public:
        AZ_CLASS_ALLOCATOR(EntityPivotTreeGraphItem, AZ::SystemAllocator);
        AZ_RTTI(EntityPivotTreeGraphItem, "{CE064D69-D478-4594-A596-5DBE0DE46F6E}", PivotTreeGraphItem);

        EntityPivotTreeGraphItem(const ScriptCanvas::GraphIdentifier& graphIdentifier);

        Qt::CheckState GetCheckState() const override final;
        void SetCheckState(Qt::CheckState checkState) override final;

        const ScriptCanvas::GraphIdentifier& GetGraphIdentifier() const;

    private:

        Qt::CheckState m_checkState;

        ScriptCanvas::GraphIdentifier m_graphIdentifier;
    };

    class EntityPivotTreeEntityItem
        : public PivotTreeEntityItem
        , public LoggingDataNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(EntityPivotTreeEntityItem, AZ::SystemAllocator);
        AZ_RTTI(EntityPivotTreeEntityItem, "{027A8617-4095-46F1-B9AD-49E360C90C73}", PivotTreeEntityItem);

        EntityPivotTreeEntityItem(const AZ::NamedEntityId& entityId);

        void RegisterGraphIdentifier(const ScriptCanvas::GraphIdentifier& graphIdentifier);
        void UnregisterGraphIdentifier(const ScriptCanvas::GraphIdentifier& graphIdentifier);

        void OnChildDataChanged(GraphCanvasTreeItem* treeItem) override;

        Qt::CheckState GetCheckState() const override final;
        void SetCheckState(Qt::CheckState debugging) override final;

        EntityPivotTreeGraphItem* FindGraphTreeItem(const ScriptCanvas::GraphIdentifier& registrationData);

        // LoggingDataNotificationBus
        void OnEnabledStateChanged(bool isEnabled, const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& registrationData) override;
        ////

    protected:

        void OnLoggingDataIdSet() override;

    private:

        void CalculateCheckState();

        Qt::CheckState m_checkState;
        AZStd::unordered_map< ScriptCanvas::GraphIdentifier, EntityPivotTreeGraphItem*> m_pivotItems;
    };

    class EntityPivotTreeRoot
        : public PivotTreeRoot
        , public LoggingDataNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(EntityPivotTreeRoot, AZ::SystemAllocator);
        AZ_RTTI(EntityPivotTreeRoot, "{93DE206E-CE31-4A59-BEBF-87B26E5A28D2}", PivotTreeRoot);
        
        EntityPivotTreeRoot();

        // PivotTreeRoot
        void OnDataSourceChanged(const LoggingDataId& aggregateDataSource) override;
        ////

        // LoggedDataNotifications
        void OnDataCaptureBegin() override;
        void OnDataCaptureEnd() override;

        void OnEntityGraphRegistered(const AZ::NamedEntityId& entityId, const ScriptCanvas::GraphIdentifier& assetId) override;
        void OnEntityGraphUnregistered(const AZ::NamedEntityId& entityId, const ScriptCanvas::GraphIdentifier& assetId) override;

        void OnEnabledStateChanged(bool isEnabled, const AZ::NamedEntityId& namedEntityId, const ScriptCanvas::GraphIdentifier& registrationData) override;
        ////

    private:

        void ClearData();

        LoggingDataId m_dataSource;
        AZStd::unordered_map< AZ::EntityId, EntityPivotTreeEntityItem* > m_entityTreeItemMapping;
        AZStd::vector < AZStd::pair < AZ::NamedEntityId, ScriptCanvas::GraphIdentifier > > m_delayedUnregistrations;

        bool m_capturingData;
    };

    class EntityPivotTreeWidget
        : public PivotTreeWidget
    {
        Q_OBJECT
    public:
        EntityPivotTreeWidget(QWidget* parent);
    };
}
