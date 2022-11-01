/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <QMenu>
#include <QMimeData>

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>

#include <Components/NodePropertyDisplays/EntityIdNodePropertyDisplay.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Widgets/NodePropertyBus.h>
#include <Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    ////////////////////////////////
    // EntityIdNodePropertyDisplay
    ////////////////////////////////
    EntityIdNodePropertyDisplay::EntityIdNodePropertyDisplay(EntityIdDataInterface* dataInterface)
        : NodePropertyDisplay(dataInterface)
        , m_dataInterface(dataInterface)
        , m_propertyEntityIdCtrl(nullptr)
        , m_proxyWidget(nullptr)
    {
        m_dataInterface->RegisterDisplay(this);
        
        m_disabledLabel = aznew GraphCanvasLabel();
        m_displayLabel = aznew GraphCanvasLabel();
    }

    void EntityIdNodePropertyDisplay::ShowContextMenu(const QPoint& pos)
    {
        if (m_propertyEntityIdCtrl)
        {
            m_dataInterface->OnShowContextMenu(m_propertyEntityIdCtrl, pos);
        }
        else
        {
            AZ_Error("GraphCanvas", false, "m_propertyEntityIdCtrl doesn't exist!");
        }
    }
    
    EntityIdNodePropertyDisplay::~EntityIdNodePropertyDisplay()
    {
        CleanupProxyWidget();
        delete m_disabledLabel;        
        delete m_displayLabel;

        delete m_dataInterface;
    }
    
    void EntityIdNodePropertyDisplay::RefreshStyle()
    {
        m_disabledLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisabledLabelStyle("entityId").c_str());

        AZStd::string styleName = NodePropertyDisplay::CreateDisplayLabelStyle("entityId");
        m_displayLabel->SetSceneStyle(GetSceneId(), styleName.c_str());
        
        QSizeF minimumSize = m_displayLabel->minimumSize();
        QSizeF maximumSize = m_displayLabel->maximumSize();

        if (m_propertyEntityIdCtrl)
        {
            m_propertyEntityIdCtrl->setMinimumSize(aznumeric_cast<int>(minimumSize.width()), aznumeric_cast<int>(minimumSize.height()));
            m_propertyEntityIdCtrl->setMaximumSize(aznumeric_cast<int>(maximumSize.width()), aznumeric_cast<int>(maximumSize.height()));
        }
    }
    
    void EntityIdNodePropertyDisplay::UpdateDisplay()
    {
        AZ::EntityId valueEntityId = m_dataInterface->GetEntityId();

        if (!AZ::EntityBus::Handler::BusIsConnectedId(valueEntityId))
        {
            AZ::EntityBus::Handler::BusDisconnect();

            if (valueEntityId.IsValid())
            {
                AZ::EntityBus::Handler::BusConnect(valueEntityId);
            }
        }

        const AZStd::string& name = m_dataInterface->GetNameOverride();

        if (m_propertyEntityIdCtrl)
        {
            m_propertyEntityIdCtrl->SetCurrentEntityId(valueEntityId, false, name);
        }

        const AZStd::string entityName = AzToolsFramework::GetEntityName(valueEntityId, name);
        if (entityName.empty())
        {
            const QString notFoundMessage = QObject::tr("(Entity not found)");
            m_displayLabel->SetLabel(notFoundMessage.toUtf8().data());
        }
        else
        {
            m_displayLabel->SetLabel(entityName);
        }

        if (m_proxyWidget)
        {
            m_proxyWidget->update();
        }
    }
    
    QGraphicsLayoutItem* EntityIdNodePropertyDisplay::GetDisabledGraphicsLayoutItem()
    {
        CleanupProxyWidget();
        return m_disabledLabel;
    }
    
    QGraphicsLayoutItem* EntityIdNodePropertyDisplay::GetDisplayGraphicsLayoutItem()
    {
        CleanupProxyWidget();
        return m_displayLabel;
    }
    
    QGraphicsLayoutItem* EntityIdNodePropertyDisplay::GetEditableGraphicsLayoutItem()
    {
        SetupProxyWidget();
        return m_proxyWidget;
    }

    void EntityIdNodePropertyDisplay::OnEntityNameChanged(const AZStd::string& /*name*/)
    {
        UpdateDisplay();
    }

    void EntityIdNodePropertyDisplay::OnDragDropStateStateChanged(const DragDropState& dragState)
    {
        Styling::StyleHelper& styleHelper = m_displayLabel->GetStyleHelper();
        UpdateStyleForDragDrop(dragState, styleHelper);
        m_displayLabel->update();
    }
    
    void EntityIdNodePropertyDisplay::EditStart()
    {
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::LockEditState, this);
        TryAndSelectNode();
    }
    
    void EntityIdNodePropertyDisplay::SubmitValue()
    {
        if (m_propertyEntityIdCtrl)
        {
            m_dataInterface->SetEntityId(m_propertyEntityIdCtrl->GetEntityId());
        }
        else
        {
            AZ_Error("GraphCanvas", false, "m_propertyEntityIdCtrl doesn't exist!");
        }
        UpdateDisplay();
    }

    void EntityIdNodePropertyDisplay::EditFinished()
    {
        SubmitValue();
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::UnlockEditState, this);
    }

    void EntityIdNodePropertyDisplay::SetupProxyWidget()
    {
        if (!m_propertyEntityIdCtrl)
        {
            m_proxyWidget = new QGraphicsProxyWidget();

            m_proxyWidget->setFlag(QGraphicsItem::ItemIsFocusable, true);
            m_proxyWidget->setFocusPolicy(Qt::StrongFocus);

            m_propertyEntityIdCtrl = aznew AzToolsFramework::PropertyEntityIdCtrl();
            m_propertyEntityIdCtrl->setProperty("HasNoWindowDecorations", true);

            m_propertyEntityIdCtrl->setContextMenuPolicy(Qt::CustomContextMenu);
            QObject::connect(m_propertyEntityIdCtrl, &AzToolsFramework::PropertyEntityIdCtrl::customContextMenuRequested, [this](const QPoint& pos) { this->ShowContextMenu(pos); });

            QObject::connect(m_propertyEntityIdCtrl, &AzToolsFramework::PropertyEntityIdCtrl::OnPickStart, [this]() { EditStart(); });
            QObject::connect(m_propertyEntityIdCtrl, &AzToolsFramework::PropertyEntityIdCtrl::OnPickComplete, [this]() { EditFinished(); });
            QObject::connect(m_propertyEntityIdCtrl, &AzToolsFramework::PropertyEntityIdCtrl::OnEntityIdChanged, [this]() { SubmitValue(); });

            m_proxyWidget->setWidget(m_propertyEntityIdCtrl);

            RegisterShortcutDispatcher(m_propertyEntityIdCtrl);
            UpdateDisplay();
            RefreshStyle();
        }
    }

    void EntityIdNodePropertyDisplay::CleanupProxyWidget()
    {
        if (m_propertyEntityIdCtrl)
        {
            UnregisterShortcutDispatcher(m_propertyEntityIdCtrl);
            delete m_propertyEntityIdCtrl; // NB: this implicitly deletes m_proxy widget
            m_propertyEntityIdCtrl = nullptr;
            m_proxyWidget = nullptr;
        }
    }
}
