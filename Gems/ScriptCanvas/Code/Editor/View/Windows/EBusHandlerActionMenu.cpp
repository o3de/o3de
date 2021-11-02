/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QEvent>
#include <QLineEdit>
#include <QListView>
#include <QWidgetAction>
#include <QKeyEvent>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeBus.h>

#include "Editor/View/Windows/EBusHandlerActionMenu.h"
#include <Editor/View/Windows/ui_ebushandleractionlistwidget.h>

#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/View/Widgets/NodePalette/ScriptEventsNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h>

#include <ScriptCanvas/Bus/RequestBus.h>

namespace ScriptCanvasEditor
{
    /////////////////////////////////
    // EBusHandlerActionSourceModel
    /////////////////////////////////
    
    EBusHandlerActionSourceModel::EBusHandlerActionSourceModel(QObject* parent)
        : QAbstractListModel(parent)
    {
    }
    
    EBusHandlerActionSourceModel::~EBusHandlerActionSourceModel()
    {
    }    
    
    int EBusHandlerActionSourceModel::rowCount([[maybe_unused]] const QModelIndex& parent) const
    {
        return static_cast<int>(m_actionItems.size());
    }
    
    QVariant EBusHandlerActionSourceModel::data(const QModelIndex& index, int role) const
    {
        int row = index.row();
        const EBusHandlerActionItem& actionItem = GetActionItemForRow(row);
        
        switch (role)
        {
            case Qt::DisplayRole:
                return actionItem.m_displayName;
            case Qt::CheckStateRole:
                if (actionItem.m_active)
                {
                    return Qt::Checked;
                }
                else
                {
                    return Qt::Unchecked;
                }
                break;
            default:
                break;
        }
        
        return QVariant();
    }    
    
    QVariant EBusHandlerActionSourceModel::headerData([[maybe_unused]] int section, [[maybe_unused]] Qt::Orientation orientation, [[maybe_unused]] int role) const
    {
        return QVariant();
    }
    
    Qt::ItemFlags EBusHandlerActionSourceModel::flags([[maybe_unused]] const QModelIndex &index) const
    {
        return Qt::ItemFlags(Qt::ItemIsUserCheckable
                             | Qt::ItemIsEnabled
                             | Qt::ItemIsSelectable);
    }

    void EBusHandlerActionSourceModel::OnItemClicked(const QModelIndex& index)
    {
        EBusHandlerActionItem& actionItem = GetActionItemForRow(index.row());
        actionItem.m_active = !actionItem.m_active;

        UpdateEBusItem(actionItem);

        emit dataChanged(index, index);
    }
    
    void EBusHandlerActionSourceModel::SetEBusNodeSource(const AZ::EntityId& ebusNode)
    {
        layoutAboutToBeChanged();
        m_actionItems.clear();

        m_ebusNode = ebusNode;

        EBusHandlerNodeDescriptorRequestBus::EventResult(m_busName, m_ebusNode, &EBusHandlerNodeDescriptorRequests::GetBusName);
        
        AZStd::vector< HandlerEventConfiguration > eventConfigurations;
        EBusHandlerNodeDescriptorRequestBus::EventResult(eventConfigurations, m_ebusNode, &EBusHandlerNodeDescriptorRequests::GetEventConfigurations);
        
        m_actionItems.resize(eventConfigurations.size());
         
        for (unsigned int i=0; i < eventConfigurations.size(); ++i)
        {
            EBusHandlerActionItem& actionItem = m_actionItems[i];
            actionItem.m_name = QString(eventConfigurations[i].m_eventName.c_str());
            actionItem.m_eventId = eventConfigurations[i].m_eventId;

            GraphCanvas::TranslationKey key;
            key << "EBusHandler" << m_busName.c_str() << "methods" << eventConfigurations[i].m_eventName << "details";

            GraphCanvas::TranslationRequests::Details details;
            details.Name = actionItem.m_name.toUtf8().data();
            GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);

            actionItem.m_displayName = QString(details.Name.c_str());
            
            actionItem.m_index = i;
            
            EBusHandlerNodeDescriptorRequestBus::EventResult(actionItem.m_active, ebusNode, &EBusHandlerNodeDescriptorRequests::ContainsEvent, eventConfigurations[i].m_eventId);
        }

        layoutChanged();
    }
    
    EBusHandlerActionItem& EBusHandlerActionSourceModel::GetActionItemForRow(int row)
    {
        return const_cast<EBusHandlerActionItem&>(static_cast<const EBusHandlerActionSourceModel*>(this)->GetActionItemForRow(row));
    }
    
    const EBusHandlerActionItem& EBusHandlerActionSourceModel::GetActionItemForRow(int row) const
    {
        if (row >= 0 && row < m_actionItems.size())
        {
            return m_actionItems[row];
        }
        else
        {
            static EBusHandlerActionItem invalidItem;
            AZ_Warning("Script Canvas", false, "EBus Handler Action Source model being asked for invalid item.");
            return invalidItem;
        }
    }

    void EBusHandlerActionSourceModel::UpdateEBusItem(EBusHandlerActionItem& actionItem)
    {
        AZ::EntityId graphCanvasGraphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphCanvasGraphId, m_ebusNode, &GraphCanvas::SceneMemberRequests::GetScene);

        ScriptCanvas::EBusEventId eventId(actionItem.m_eventId);

        if (actionItem.m_active)
        {
            AZ::Vector2 dummyPosition(0, 0);
            NodeIdPair idPair;

            if (ScriptEventReceiverNodeDescriptorRequestBus::FindFirstHandler(m_ebusNode) != nullptr)
            {
                AZ::Data::AssetId assetId;
                ScriptEventReceiverNodeDescriptorRequestBus::EventResult(assetId, m_ebusNode, &ScriptEventReceiverNodeDescriptorRequests::GetAssetId);

                AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(assetId, AZ::Data::AssetLoadBehavior::Default);

                asset.BlockUntilLoadComplete();

                if (asset.IsReady())
                {
                    const ScriptEvents::ScriptEvent& definition = asset.Get()->m_definition;

                    ScriptEvents::Method methodDefinition;
                    
                    if (definition.FindMethod(actionItem.m_name.toUtf8().data(), methodDefinition))
                    {
                        eventId = methodDefinition.GetEventId();

                        CreateScriptEventsReceiverMimeEvent mimeEvent(asset.GetId(), methodDefinition);
                        idPair = mimeEvent.CreateEventNode(graphCanvasGraphId, dummyPosition);
                    }
                }
            }
            else
            {
                CreateEBusHandlerEventMimeEvent mimeEvent(m_busName, actionItem.m_name.toUtf8().data(), actionItem.m_eventId);
                idPair = mimeEvent.CreateEventNode(graphCanvasGraphId, dummyPosition);                
            }

            GraphCanvas::WrappedNodeConfiguration configuration;

            EBusHandlerNodeDescriptorRequestBus::EventResult(configuration, m_ebusNode, &EBusHandlerNodeDescriptorRequests::GetEventConfiguration, eventId);
            GraphCanvas::WrapperNodeRequestBus::Event(m_ebusNode, &GraphCanvas::WrapperNodeRequests::WrapNode, idPair.m_graphCanvasId, configuration);

            ScriptCanvas::ScriptCanvasId scriptCanvasId;
            GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

            GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, scriptCanvasId);
        }
        else
        {
            AZ::EntityId nodeId;
            EBusHandlerNodeDescriptorRequestBus::EventResult(nodeId, m_ebusNode, &EBusHandlerNodeDescriptorRequests::FindEventNodeId, eventId);

            if (nodeId.IsValid())
            {
                AZStd::unordered_set<AZ::EntityId> deleteNodes;
                deleteNodes.insert(nodeId);

                GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::Delete, deleteNodes);

                actionItem.m_active = false;
            }
        }
    }
    
    //////////////////////////////////////
    // EBusHandlerActionFilterProxyModel
    //////////////////////////////////////
    EBusHandlerActionFilterProxyModel::EBusHandlerActionFilterProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
        m_regex.setCaseSensitivity(Qt::CaseInsensitive);
    }
    
    void EBusHandlerActionFilterProxyModel::SetFilterSource(QLineEdit* lineEdit)
    {
        if (lineEdit)
        {
            QObject::connect(lineEdit, &QLineEdit::textChanged, this, &EBusHandlerActionFilterProxyModel::OnFilterChanged);
        }
    }
    
    bool EBusHandlerActionFilterProxyModel::filterAcceptsRow(int sourceRow, [[maybe_unused]] const QModelIndex& sourceParent) const
    {
        EBusHandlerActionSourceModel* sourceModel = static_cast<EBusHandlerActionSourceModel*>(this->sourceModel());

        const EBusHandlerActionItem& actionItem = sourceModel->GetActionItemForRow(sourceRow);

        return actionItem.m_name.lastIndexOf(m_regex) >= 0;
    }
    
    void EBusHandlerActionFilterProxyModel::OnFilterChanged(const QString& text)
    {
        m_filter = text;
        m_regex.setPattern(text);
        invalidate();
        layoutChanged();
    }

    //////////////////////////
    // EBusHandlerActionMenu
    //////////////////////////
    
    EBusHandlerActionMenu::EBusHandlerActionMenu(QWidget* parent)
        : QMenu(parent)
    {
        QWidgetAction* actionWidget = new QWidgetAction(this);
        
        QWidget* listWidget = new QWidget(this);
        m_listWidget = new Ui::EBusHandlerActionListWidget();
        
        m_listWidget->setupUi(listWidget);
        
        actionWidget->setDefaultWidget(listWidget);
        
        addAction(actionWidget);
        
        connect(this, &QMenu::aboutToShow, this, &EBusHandlerActionMenu::ResetFilter);

        m_model = aznew EBusHandlerActionSourceModel(this);

        m_proxyModel = aznew EBusHandlerActionFilterProxyModel(this);
        m_proxyModel->setSourceModel(m_model);

        m_proxyModel->SetFilterSource(m_listWidget->searchFilter);

        m_listWidget->actionListView->setModel(m_proxyModel);

        QObject::connect(m_listWidget->actionListView, &QListView::clicked, this, &EBusHandlerActionMenu::ItemClicked);
    }

    void EBusHandlerActionMenu::SetEbusHandlerNode(const AZ::EntityId& ebusWrapperNode)
    {
        m_proxyModel->layoutAboutToBeChanged();
        m_model->SetEBusNodeSource(ebusWrapperNode);
        m_proxyModel->layoutChanged();
        m_proxyModel->invalidate();
    }

    void EBusHandlerActionMenu::ResetFilter()
    {
        m_listWidget->actionListView->selectionModel()->clearSelection();
        m_listWidget->searchFilter->setText("");
    }

    void EBusHandlerActionMenu::ItemClicked(const QModelIndex& modelIndex)
    {
        QModelIndex sourceIndex = m_proxyModel->mapToSource(modelIndex);
        m_model->OnItemClicked(sourceIndex);
    }
    
    void EBusHandlerActionMenu::keyPressEvent(QKeyEvent* keyEvent)
    {
        // Only pass along escape keys(don't want any sort of selection state with this menu)
        if (keyEvent->key() == Qt::Key_Escape)
        {
            QMenu::keyPressEvent(keyEvent);
        }
    }
    
#include <Editor/View/Windows/moc_EBusHandlerActionMenu.cpp>
}
