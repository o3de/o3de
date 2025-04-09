/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QAbstractListModel>
#include <QAbstractItemView>
#include <QMenu>
#include <qregexp.h>
#include <QSortFilterProxyModel>

#include <AzCore/Component/Entity.h>

#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#endif

namespace Ui
{
    class EBusHandlerActionListWidget;
}

namespace ScriptCanvasEditor
{
    struct EBusHandlerActionItem
    {
    public:
    
        int m_index = -1;
        QString m_name;
        QString m_displayName;
        bool m_active = false;
        ScriptCanvas::EBusEventId m_eventId;
    };
    
    class EBusHandlerActionSourceModel
        : public QAbstractListModel
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(EBusHandlerActionSourceModel, AZ::SystemAllocator);

        EBusHandlerActionSourceModel(QObject* parent = nullptr);
        ~EBusHandlerActionSourceModel();

        int rowCount(const QModelIndex& parent) const override;
        
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        
        Qt::ItemFlags flags(const QModelIndex &index) const override;

        void OnItemClicked(const QModelIndex& index);
        
        void SetEBusNodeSource(const AZ::EntityId& ebusNode);
        
        EBusHandlerActionItem& GetActionItemForRow(int row);
        const EBusHandlerActionItem& GetActionItemForRow(int row) const;
        
    private:

        void UpdateEBusItem(EBusHandlerActionItem& actionItem);
    
        AZStd::vector< EBusHandlerActionItem > m_actionItems;
    
        AZ::EntityId m_ebusNode;
        AZStd::string m_busName;
    };
    
    class EBusHandlerActionFilterProxyModel
        : public QSortFilterProxyModel
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(EBusHandlerActionFilterProxyModel, AZ::SystemAllocator);
        
        EBusHandlerActionFilterProxyModel(QObject* parent = nullptr);
        ~EBusHandlerActionFilterProxyModel() = default;

        void SetFilterSource(QLineEdit* lineEdit);
        
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;    
        
    public slots:
        void OnFilterChanged(const QString& text);
        
    private:
        QString m_filter;

        QRegExp m_regex;
    };

    class EBusHandlerActionMenu
        : public QMenu
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(EBusHandlerActionMenu, AZ::SystemAllocator);
        EBusHandlerActionMenu(QWidget* parent = nullptr);
        ~EBusHandlerActionMenu() = default;
        
        void SetEbusHandlerNode(const AZ::EntityId& ebusWrapperNode);
        
    public slots:
        void ResetFilter();
        void ItemClicked(const QModelIndex& modelIndex);
        
    protected:    
        void keyPressEvent(QKeyEvent* keyEvent) override;   
        
    private:    
        
        EBusHandlerActionFilterProxyModel* m_proxyModel;
        EBusHandlerActionSourceModel* m_model;
        Ui::EBusHandlerActionListWidget* m_listWidget;
    };
}
