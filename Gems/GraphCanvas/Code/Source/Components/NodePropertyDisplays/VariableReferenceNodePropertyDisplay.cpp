/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <qgraphicsproxywidget.h>

#include <Components/NodePropertyDisplays/VariableReferenceNodePropertyDisplay.h>

#include <GraphCanvas/Components/Nodes/Variable/VariableNodeBus.h>
#include <GraphCanvas/Widgets/NodePropertyBus.h>
#include <Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    //////////////////////
    // VariableItemModel
    //////////////////////
    
    VariableItemModel::VariableItemModel()
    {
    }

    VariableItemModel::~VariableItemModel()
    {
    }

    int VariableItemModel::rowCount(const QModelIndex& parent) const
    {
        return static_cast<int>(m_variableIds.size());
    }

    QVariant VariableItemModel::data(const QModelIndex& index, int role) const
    {
        int row = index.row();
        AZ::EntityId variableId = FindVariableIdForRow(row);

        if (variableId.IsValid())
        {
            switch (role)
            {
            case Qt::DisplayRole:
            {
                AZStd::string variableName;
                VariableRequestBus::EventResult(variableName, variableId, &VariableRequests::GetVariableName);
                return QString(variableName.c_str());
            }
            case Qt::EditRole:
            {
                AZStd::string variableName;
                VariableRequestBus::EventResult(variableName, variableId, &VariableRequests::GetVariableName);
                return QString(variableName.c_str());
            }
            default:
                break;
            }
        }
        else
        {
            switch (role)
            {
            case Qt::DisplayRole:
            {
                return QString("Unreferenced");
            }
            case Qt::EditRole:
            {
                return QString("Unreferenced");
            }
            default:
                break;
            }
        }

        return QVariant();
    }

    QVariant VariableItemModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        return QVariant();
    }

    Qt::ItemFlags VariableItemModel::flags(const QModelIndex& index) const
    {
        return Qt::ItemFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    }

    void VariableItemModel::SetSceneId(const AZ::EntityId& sceneId)
    {
        m_sceneId = sceneId;
    }

    void VariableItemModel::SetDataType(const AZ::Uuid& variableType)
    {
        if (m_dataType != variableType)
        {
            m_dataType = variableType;
        }
    }

    void VariableItemModel::RefreshData()
    {
        layoutAboutToBeChanged();
        ClearData();

        m_variableIds.emplace_back(AZ::EntityId());

        if (m_dataType == AZStd::any::TYPEINFO_Uuid())
        {
            SceneVariableRequestBus::EnumerateHandlersId(m_sceneId, [this](SceneVariableRequests* variableRequests)
            {
                m_variableIds.emplace_back(variableRequests->GetVariableId());
                return true;
            });
        }
        else
        {
            SceneVariableRequestBus::EnumerateHandlersId(m_sceneId, [this](SceneVariableRequests* variableRequests)
            {
                AZ::EntityId variableId = variableRequests->GetVariableId();
                AZ::Uuid dataType;
                VariableRequestBus::EventResult(dataType, variableId, &VariableRequests::GetDataType);

                if (dataType == m_dataType)
                {
                    m_variableIds.emplace_back(variableRequests->GetVariableId());
                }
                return true;
            });
        }

        layoutChanged();
    }

    void VariableItemModel::ClearData()
    {
        layoutAboutToBeChanged();
        m_variableIds.clear();
        layoutChanged();
    }

    int VariableItemModel::FindRowForVariable(const AZ::EntityId& variableId) const
    {
        int row = -1;

        for (int i = 0; i < static_cast<int>(m_variableIds.size()); ++i)
        {
            if (m_variableIds[i] == variableId)
            {
                row = i;
                break;
            }
        }

        return row;
    }

    AZ::EntityId VariableItemModel::FindVariableIdForRow(int row) const
    {
        if (row >= 0 && row < m_variableIds.size())
        {
            return m_variableIds[row];
        }

        return AZ::EntityId();
    }

    AZ::EntityId VariableItemModel::FindVariableIdForName(const AZStd::string& variableName) const
    {
        AZ::EntityId variableId;        
        
        QString lowerName = QString(variableName.c_str()).toLower();

        GraphCanvas::SceneVariableRequestBus::EnumerateHandlersId(m_sceneId, [&lowerName, &variableId](GraphCanvas::SceneVariableRequests* sceneVariable)
        {
            AZ::EntityId testId = sceneVariable->GetVariableId();

            AZStd::string testName;
            GraphCanvas::VariableRequestBus::EventResult(testName, testId, &GraphCanvas::VariableRequests::GetVariableName);

            QString lowerTextName = QString(testName.c_str()).toLower();

            if (lowerTextName.compare(lowerName) == 0)
            {
                variableId = testId;
            }

            return !variableId.IsValid();
        });

        return variableId;
    }

    ////////////////////////////
    // VariableSelectionWidget
    ////////////////////////////

    VariableSelectionWidget::VariableSelectionWidget()
        : QWidget(nullptr)
        , m_lineEdit(aznew Internal::FocusableLineEdit())
        , m_itemModel(aznew VariableItemModel())
    {
        m_completer = new QCompleter(m_itemModel, this);
        m_completer->setCaseSensitivity(Qt::CaseInsensitive);
        m_completer->setCompletionMode(QCompleter::InlineCompletion);

        m_lineEdit->setCompleter(m_completer);
        m_lineEdit->setPlaceholderText("Select Variable...");

        m_layout = new QVBoxLayout(this);
        m_layout->addWidget(m_lineEdit);

        setContentsMargins(0, 0, 0, 0);
        m_layout->setContentsMargins(0, 0, 0, 0);

        setLayout(m_layout);

        QObject::connect(m_lineEdit, &Internal::FocusableLineEdit::OnFocusIn, this, &VariableSelectionWidget::HandleFocusIn);
        QObject::connect(m_lineEdit, &Internal::FocusableLineEdit::OnFocusOut, this, &VariableSelectionWidget::HandleFocusOut);
        QObject::connect(m_lineEdit, &Internal::FocusableLineEdit::returnPressed, this, &VariableSelectionWidget::SubmitName);
    }

    VariableSelectionWidget::~VariableSelectionWidget()
    {
    }

    void VariableSelectionWidget::SetSceneId(const AZ::EntityId& sceneId)
    {
        m_itemModel->SetSceneId(sceneId);
    }

    void VariableSelectionWidget::SetDataType(const AZ::Uuid& dataType)
    {
        m_itemModel->SetDataType(dataType);
    }

    void VariableSelectionWidget::OnEscape()
    {
        SetSelectedVariable(m_initialVariable);
        m_lineEdit->selectAll();
    }

    void VariableSelectionWidget::HandleFocusIn()
    {
        m_itemModel->RefreshData();
        emit OnFocusIn();

        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void VariableSelectionWidget::HandleFocusOut()
    {
        m_itemModel->ClearData();
        emit OnFocusOut();

        SetSelectedVariable(m_initialVariable);

        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    void VariableSelectionWidget::SubmitName()
    {
        AZStd::string variableName = m_lineEdit->text().toUtf8().data();
        AZ::EntityId variableId = m_itemModel->FindVariableIdForName(variableName);

        if (variableId.IsValid())
        {
            SetSelectedVariable(variableId);
            m_lineEdit->selectAll();
        }
        else
        {
            QSignalBlocker block(m_lineEdit);
            m_lineEdit->setText("");
        }

        emit OnVariableSelected(variableId);        
    }

    void VariableSelectionWidget::SetSelectedVariable(const AZ::EntityId& variableId)
    {
        QSignalBlocker blocker(m_lineEdit);

        AZStd::string variableName;
        VariableRequestBus::EventResult(variableName, variableId, &VariableRequests::GetVariableName);            

        m_lineEdit->setText(variableName.c_str());

        m_initialVariable = variableId;
    }

    /////////////////////////////////////////
    // VariableReferenceNodePropertyDisplay
    /////////////////////////////////////////
    
    VariableReferenceNodePropertyDisplay::VariableReferenceNodePropertyDisplay(VariableReferenceDataInterface* dataInterface)
        : m_variableReferenceDataInterface(dataInterface)
    {
        m_variableReferenceDataInterface->RegisterDisplay(this);

        m_disabledLabel = aznew GraphCanvasLabel();
        m_displayLabel = aznew GraphCanvasLabel();

        m_proxyWidget = new QGraphicsProxyWidget();

        m_variableSelectionWidget = aznew VariableSelectionWidget();
        m_variableSelectionWidget->setProperty("HasNoWindowDecorations", true);

        m_proxyWidget->setWidget(m_variableSelectionWidget);

        m_variableSelectionWidget->SetDataType(dataInterface->GetVariableDataType());

        QObject::connect(m_variableSelectionWidget, &VariableSelectionWidget::OnFocusIn, [this]() { EditStart(); });
        QObject::connect(m_variableSelectionWidget, &VariableSelectionWidget::OnFocusOut, [this]() { EditFinished(); });
        QObject::connect(m_variableSelectionWidget, &VariableSelectionWidget::OnVariableSelected, [this](const AZ::EntityId& variableId) { AssignVariable(variableId); });

        RegisterShortcutDispatcher(m_variableSelectionWidget);
    }
    
    VariableReferenceNodePropertyDisplay::~VariableReferenceNodePropertyDisplay()
    {
        delete m_variableReferenceDataInterface;

        delete m_disabledLabel;
        delete m_displayLabel;
    }
    
    void VariableReferenceNodePropertyDisplay::RefreshStyle()
    {
        m_disabledLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisabledLabelStyle("variable").c_str());
        m_displayLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisplayLabelStyle("variable").c_str());

        m_variableSelectionWidget->setMinimumSize(m_displayLabel->GetStyleHelper().GetMinimumSize().toSize());
    }
    
    void VariableReferenceNodePropertyDisplay::UpdateDisplay()
    {
        VariableNotificationBus::Handler::BusDisconnect();

        AZ::EntityId variableId = m_variableReferenceDataInterface->GetVariableReference();

        DisplayVariableString(variableId);

        m_variableSelectionWidget->SetSelectedVariable(variableId);

        if (variableId.IsValid())
        {
            VariableNotificationBus::Handler::BusConnect(variableId);
        }
    }
    
    QGraphicsLayoutItem* VariableReferenceNodePropertyDisplay::GetDisabledGraphicsLayoutItem() const
    {
        return m_disabledLabel;
    }

    QGraphicsLayoutItem* VariableReferenceNodePropertyDisplay::GetDisplayGraphicsLayoutItem() const
    {
        return m_displayLabel;
    }

    QGraphicsLayoutItem* VariableReferenceNodePropertyDisplay::GetEditableGraphicsLayoutItem() const
    {
        return m_proxyWidget;
    }

    void VariableReferenceNodePropertyDisplay::OnNameChanged()
    {
        AZ::EntityId variableId = m_variableReferenceDataInterface->GetVariableReference();

        DisplayVariableString(variableId);
    }

    void VariableReferenceNodePropertyDisplay::OnVariableActivated()
    {
        AZ::EntityId variableId = m_variableReferenceDataInterface->GetVariableReference();

        DisplayVariableString(variableId);
    }

    void VariableReferenceNodePropertyDisplay::OnIdSet()
    {
        m_variableSelectionWidget->SetSceneId(GetSceneId());
    }

    void VariableReferenceNodePropertyDisplay::DisplayVariableString(const AZ::EntityId& variableId)
    {
        AZStd::string variableName = "Select Variable";

        if (variableId.IsValid())
        {
            VariableRequestBus::EventResult(variableName, variableId, &VariableRequests::GetVariableName);
        }

        m_displayLabel->SetLabel(variableName.c_str());
    }

    void VariableReferenceNodePropertyDisplay::EditStart()
    {
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::LockEditState, this);
        TryAndSelectNode();
    }

    void VariableReferenceNodePropertyDisplay::AssignVariable(const AZ::EntityId& variableId)
    {
        m_variableReferenceDataInterface->AssignVariableReference(variableId);
        DisplayVariableString(m_variableReferenceDataInterface->GetVariableReference());
    }

    void VariableReferenceNodePropertyDisplay::EditFinished()
    {
        UpdateDisplay();
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::UnlockEditState, this);
    }
    
#include <Source/Components/NodePropertyDisplays/moc_VariableReferenceNodePropertyDisplay.cpp>
}
