/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformDef.h>
// qbrush.h(118): warning C4251: 'QBrush::d': class 'QScopedPointer<QBrushData,QBrushDataPointerDeleter>' needs to have dll-interface to be used by clients of class 'QBrush'
// qwidget.h(858): warning C4800: 'uint': forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QTimer>
#include <QTreeView>
#include <QSortFilterProxyModel>
AZ_POP_DISABLE_WARNING

#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <GraphCanvas/Widgets/GraphCanvasTreeItem.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeModel.h>

#include <Editor/View/Widgets/LoggingPanel/LoggingWindowSession.h>
#endif

namespace Ui
{
    class PivotTreeWidget;
}

namespace ScriptCanvasEditor
{
    class PivotTreeItem
        : public GraphCanvas::GraphCanvasTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(PivotTreeItem, AZ::SystemAllocator);
        AZ_RTTI(PivotTreeItem, "{F310C0EA-9CFE-4A8F-9CDA-46E24673B01A}", GraphCanvas::GraphCanvasTreeItem);

        enum Column
        {
            IndexForce = -1,
            Name,
            // Seriously. Returning 1 causes the data to only ask for the tool tip.
            // No idea why.
            QT_NEEDS_A_SECOND_COLUMN_FOR_THIS_MODEL_TO_WORK_FOR_SOME_REASON,
            Count
        };

        PivotTreeItem();
        ~PivotTreeItem();

        const LoggingDataId& GetLoggingDataId() const;

        // GraphCanvasTreeItem
        int GetColumnCount() const override final;
        Qt::ItemFlags Flags(const QModelIndex& index) const override final;
        QVariant Data(const QModelIndex& index, int role) const override final;
        bool SetData(const QModelIndex& index, const QVariant& value, int role) override final;

        void OnChildAdded(GraphCanvasTreeItem* treeItem) override final;
        ////
        
        virtual Qt::CheckState GetCheckState() const = 0;
        virtual void SetCheckState(Qt::CheckState checkState) = 0;

    protected:
        virtual AZStd::string GetDisplayName() const = 0;

        virtual void OnLoggingDataIdSet();

        void SetLoggingDataId(const LoggingDataId& dataId);
        void SetIsPivotedElement(bool isPivotedElement);

    private:

        bool m_isPivotElement;
        LoggingDataId m_loggingDataId;
    };

    class PivotTreeEntityItem
        : public PivotTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(PivotTreeEntityItem, AZ::SystemAllocator);
        AZ_RTTI(PivotTreeEntityItem, "{67725865-7004-441D-84BB-D38FF491A3FD}", PivotTreeItem);

        PivotTreeEntityItem(const AZ::NamedEntityId& entityId);

        const AZ::NamedEntityId& GetNamedEntityId() const;

    protected:

        AZStd::string GetDisplayName() const override final;
        const AZ::EntityId& GetEntityId() const;

    private:
        AZ::NamedEntityId m_namedEntityId;
    };

    class PivotTreeGraphItem
        : public PivotTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(PivotTreeGraphItem, AZ::SystemAllocator);
        AZ_RTTI(PivotTreeGraphItem, "{37FCAC77-DE32-4B1B-97FD-66852EC31CAB}", PivotTreeItem);

        PivotTreeGraphItem(const AZ::Data::AssetId& assetId);

        const AZ::Data::AssetId& GetAssetId() const;

    protected:

        AZStd::string GetDisplayName() const override final;
        AZStd::string_view GetAssetPath() const;

    private:
        AZ::Data::AssetId             m_assetId;
        AZStd::string                 m_assetPath;
        AZStd::string                 m_assetName;
    };

    class PivotTreeRoot
        : public PivotTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(PivotTreeRoot, AZ::SystemAllocator);
        AZ_RTTI(PivotTreeRoot, "{E172AB89-49BA-429F-AC83-9CCBD6A3B1B9}", PivotTreeItem);

        PivotTreeRoot() = default;

        void SwitchDataSource(const LoggingDataId& aggregateDataSource);

    protected:

        Qt::CheckState GetCheckState() const override final;
        void SetCheckState(Qt::CheckState checkState) override final;

        AZStd::string GetDisplayName() const override final;

        virtual void OnDataSourceChanged(const LoggingDataId& aggregateDataSource) = 0;

    private:

        LoggingDataId m_loggingDataId;
    };

    class PivotTreeSortProxyModel
        : public QSortFilterProxyModel
    {
    public:

        AZ_CLASS_ALLOCATOR(PivotTreeSortProxyModel, AZ::SystemAllocator);

        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

        bool HasFilter() const;
        void SetFilter(const QString& filter);
        void ClearFilter();

    private:

        QString m_filter;
        QRegExp m_filterRegex;
    };

    class PivotTreeWidget
        : public QWidget
    {
        Q_OBJECT
    public:

        ~PivotTreeWidget();

        void DisplayTree();

        void SwitchDataSource(const LoggingDataId& aggregateDataSource);

    public Q_SLOT:

        void OnFilterChanged(const QString& activeTextFilter);

    protected:

        PivotTreeWidget(PivotTreeRoot* pivotRoot, const AZ::Crc32& savingId, QWidget* parent);

        PivotTreeRoot* GetTreeRoot();

        virtual void OnTreeDisplayed();

    private:

        void OnItemDoubleClicked(const QModelIndex& modelIndex);

        AZStd::unique_ptr<Ui::PivotTreeWidget> m_ui;

        PivotTreeRoot* m_pivotRoot;

        GraphCanvas::GraphCanvasTreeModel* m_treeModel;
        PivotTreeSortProxyModel* m_proxyModel;
    };
}
