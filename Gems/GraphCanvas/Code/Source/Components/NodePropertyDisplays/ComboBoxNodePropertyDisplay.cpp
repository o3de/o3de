/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QGraphicsProxyWidget>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsView>
#include <QMenu>
#include <QMimeData>
#include <QSignalBlocker>

#include <Components/NodePropertyDisplays/ComboBoxNodePropertyDisplay.h>

#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Utils/ConversionUtils.h>
#include <GraphCanvas/Widgets/NodePropertyBus.h>
#include <Widgets/GraphCanvasLabel.h>

#include <GraphCanvas/Utils/QtDrawingUtils.h>

namespace GraphCanvas
{
    ////////////////////////////////
    // ComboBoxNodePropertyDisplay
    ////////////////////////////////
    ComboBoxNodePropertyDisplay::ComboBoxNodePropertyDisplay(ComboBoxDataInterface* dataInterface)
        : NodePropertyDisplay(dataInterface)
        , m_valueDirty(false)
        , m_menuDisplayDirty(true)
        , m_dataInterface(dataInterface)
        , m_comboBox(nullptr)
        , m_proxyWidget(nullptr)
        , m_dataTypeOutlineEnabled(true)
    {
        m_disabledLabel = aznew GraphCanvasLabel();
        m_displayLabel = aznew GraphCanvasLabel();
    }

    void ComboBoxNodePropertyDisplay::ShowContextMenu(const QPoint& pos)
    {
        if (m_comboBox)
        {
            m_dataInterface->OnShowContextMenu(m_comboBox, pos);
        }
        else
        {
            AZ_Error("GraphCanvas", false, "m_propertyEntityIdCtrl doesn't exist!");
        }
    }

    ComboBoxNodePropertyDisplay::~ComboBoxNodePropertyDisplay()
    {
        CleanupProxyWidget();
        delete m_disabledLabel;
        delete m_displayLabel;

        delete m_dataInterface;
    }

    void ComboBoxNodePropertyDisplay::SetDataTypeOutlineEnabled(bool dataTypeOutlineEnabled)
    {
        if (m_dataTypeOutlineEnabled != dataTypeOutlineEnabled)
        {
            m_dataTypeOutlineEnabled = dataTypeOutlineEnabled;

            if (GetSlotId().IsValid())
            {
                UpdateOutlineColor();
            }
        }
    }

    void ComboBoxNodePropertyDisplay::RefreshStyle()
    {
        m_disabledLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisabledLabelStyle("entityId").c_str());

        AZStd::string styleName = NodePropertyDisplay::CreateDisplayLabelStyle("entityId");
        m_displayLabel->SetSceneStyle(GetSceneId(), styleName.c_str());

        QSizeF minimumSize = m_displayLabel->minimumSize();
        QSizeF maximumSize = m_displayLabel->maximumSize();

        if (m_comboBox)
        {
            m_comboBox->setMinimumSize(aznumeric_cast<int>(minimumSize.width()), aznumeric_cast<int>(minimumSize.height()));
            m_comboBox->setMaximumSize(aznumeric_cast<int>(maximumSize.width()), aznumeric_cast<int>(maximumSize.height()));
        }

        UpdateOutlineColor();
    }

    void ComboBoxNodePropertyDisplay::UpdateDisplay()
    {
        const QString& displayValue = m_dataInterface->GetDisplayString();

        if (m_comboBox)
        {
            QModelIndex selectedIndex = m_dataInterface->GetAssignedIndex();
            
            {
                QSignalBlocker signalBlocker(m_comboBox);
                m_comboBox->SetSelectedIndex(selectedIndex);
                m_valueDirty = false;
            }
        }

        QString displayLabel = displayValue;

        if (displayLabel.isEmpty())
        {
            displayLabel = "<None>";
        }

        m_displayLabel->SetLabel(displayLabel.toUtf8().data());

        if (m_proxyWidget)
        {
            m_proxyWidget->update();
        }
    }

    QGraphicsLayoutItem* ComboBoxNodePropertyDisplay::GetDisabledGraphicsLayoutItem()
    {
        CleanupProxyWidget();
        return m_disabledLabel;
    }

    QGraphicsLayoutItem* ComboBoxNodePropertyDisplay::GetDisplayGraphicsLayoutItem()
    {
        CleanupProxyWidget();
        return m_displayLabel;
    }

    QGraphicsLayoutItem* ComboBoxNodePropertyDisplay::GetEditableGraphicsLayoutItem()
    {
        SetupProxyWidget();
        return m_proxyWidget;
    }

    void ComboBoxNodePropertyDisplay::OnPositionChanged(const AZ::EntityId& /*targetEntity*/, const AZ::Vector2& /*position*/)
    {
        GraphId graphId;
        SceneMemberRequestBus::EventResult(graphId, GetNodeId(), &SceneMemberRequests::GetScene);

        ViewId viewId;
        SceneRequestBus::EventResult(viewId, graphId, &SceneRequests::GetViewId);

        UpdateMenuDisplay(viewId);
    }

    void ComboBoxNodePropertyDisplay::OnZoomChanged(qreal /*zoomLevel*/)
    {
        const ViewId* viewId = ViewNotificationBus::GetCurrentBusId();

        if (viewId)
        {
            UpdateMenuDisplay((*viewId));
        }
    }

    void ComboBoxNodePropertyDisplay::OnDisplayTypeChanged(const AZ::Uuid& /*dataTypes*/, const AZStd::vector<AZ::Uuid>& /*containerTypes*/)
    {
        UpdateOutlineColor();
    }

    void ComboBoxNodePropertyDisplay::OnDragDropStateStateChanged(const DragDropState& dragState)
    {
        Styling::StyleHelper& styleHelper = m_displayLabel->GetStyleHelper();
        UpdateStyleForDragDrop(dragState, styleHelper);
        m_displayLabel->update();
    }

    void ComboBoxNodePropertyDisplay::UpdateOutlineColor()
    {
        if (!m_dataTypeOutlineEnabled)
        {
            return;
        }
        
        DataValueType valueType = DataValueType::Unknown;

        DataSlotRequests* dataSlotRequests = DataSlotRequestBus::FindFirstHandler(GetSlotId());

        if (dataSlotRequests)
        {
            valueType = dataSlotRequests->GetDataValueType();

            bool updatedOutline = false;
            QBrush outlineBrush;

            if (valueType == DataValueType::Container)
            {
                size_t typeCount = dataSlotRequests->GetContainedTypesCount();

                if (typeCount != 0)
                {
                    updatedOutline = true;
                    AZStd::vector< const Styling::StyleHelper* > containerColorPalettes;
                    containerColorPalettes.reserve(typeCount);

                    for (size_t i = 0; i < typeCount; ++i)
                    {
                        const Styling::StyleHelper* colorPalette = dataSlotRequests->GetContainedTypeColorPalette(i);

                        if (colorPalette)
                        {
                            containerColorPalettes.emplace_back(colorPalette);
                        }
                    }

                    QLinearGradient penGradient;
                    QLinearGradient fillGradient;

                    if (!containerColorPalettes.empty())
                    {
                        QtDrawingUtils::GenerateGradients(containerColorPalettes, m_displayLabel->GetDisplayedSize(), penGradient, fillGradient);
                    }

                    m_displayLabel->SetBorderColorOverride(QBrush(penGradient));

                }
            }

            if (!updatedOutline)
            {
                const Styling::StyleHelper* colorPalette = dataSlotRequests->GetDataColorPalette();

                if (colorPalette)
                {
                    QColor color = colorPalette->GetColor(GraphCanvas::Styling::Attribute::LineColor);
                    m_displayLabel->SetBorderColorOverride(QBrush(color));
                }
                else
                {
                    m_displayLabel->ClearBorderColorOverride();

                    if (m_comboBox)
                    {
                        m_comboBox->ClearOutlineColor();
                    }
                }
            }
        }
    }

    void ComboBoxNodePropertyDisplay::OnSlotIdSet()
    {
        UpdateOutlineColor();
    }

    void ComboBoxNodePropertyDisplay::EditStart()
    {
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::LockEditState, this);
        TryAndSelectNode();
    }

    void ComboBoxNodePropertyDisplay::SubmitValue()
    {
        if (!m_valueDirty)
        {
            return;
        }

        m_valueDirty = false;

        if (m_comboBox)
        {
            QModelIndex index = m_comboBox->GetSelectedIndex();
            
            m_dataInterface->AssignIndex(index);
        }
        else
        {
            AZ_Error("GraphCanvas", false, "m_comboBox doesn't exist!");
        }

        UpdateDisplay();
    }

    void ComboBoxNodePropertyDisplay::EditFinished()
    {        
        SubmitValue();
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::UnlockEditState, this);
    }

    void ComboBoxNodePropertyDisplay::SetupProxyWidget()
    {
        if (!m_comboBox)
        {
            m_proxyWidget = new QGraphicsProxyWidget();

            m_proxyWidget->setFlag(QGraphicsItem::ItemIsFocusable, true);
            m_proxyWidget->setFocusPolicy(Qt::StrongFocus);

            m_comboBox = aznew GraphCanvasComboBox(m_dataInterface->GetItemInterface());
            m_comboBox->setContextMenuPolicy(Qt::CustomContextMenu);

            QObject::connect(m_comboBox, &AzToolsFramework::PropertyEntityIdCtrl::customContextMenuRequested, [this](const QPoint& pos) { ShowContextMenu(pos); });
            QObject::connect(m_comboBox, &GraphCanvasComboBox::SelectedIndexChanged, [this](const QModelIndex& /*index*/) { m_valueDirty = true; });
            QObject::connect(m_comboBox, &GraphCanvasComboBox::OnFocusIn, [this]() { EditStart(); });
            QObject::connect(m_comboBox, &GraphCanvasComboBox::OnFocusOut, [this]() { EditFinished(); });
            QObject::connect(m_comboBox, &GraphCanvasComboBox::OnUserActionComplete, [this]() { SubmitValue(); });
            QObject::connect(m_comboBox, &GraphCanvasComboBox::OnMenuAboutToDisplay, [this]() { OnMenuAboutToDisplay(); });

            m_proxyWidget->setWidget(m_comboBox);

            RegisterShortcutDispatcher(m_comboBox);
            UpdateDisplay();
            RefreshStyle();

            GraphId graphId;
            SceneMemberRequestBus::EventResult(graphId, GetNodeId(), &SceneMemberRequests::GetScene);

            ViewId viewId;
            SceneRequestBus::EventResult(viewId, graphId, &SceneRequests::GetViewId);

            m_comboBox->RegisterViewId(viewId);
            m_comboBox->SetSelectedIndex(m_dataInterface->GetAssignedIndex());

            m_valueDirty = false;
            m_menuDisplayDirty = true;

            ViewNotificationBus::Handler::BusConnect(viewId);
            GeometryNotificationBus::Handler::BusConnect(GetNodeId());
        }
    }

    void ComboBoxNodePropertyDisplay::CleanupProxyWidget()
    {
        if (m_comboBox)
        {
            UnregisterShortcutDispatcher(m_comboBox);
            delete m_comboBox; // NB: this implicitly deletes m_proxy widget
            m_comboBox = nullptr;
            m_proxyWidget = nullptr;

            m_menuDisplayDirty = false;

            ViewNotificationBus::Handler::BusDisconnect();
            GeometryNotificationBus::Handler::BusDisconnect(GetNodeId());
        }
    }

    void ComboBoxNodePropertyDisplay::OnMenuAboutToDisplay()
    {
        if (m_menuDisplayDirty)
        {
            GraphId graphId;
            SceneMemberRequestBus::EventResult(graphId, GetNodeId(), &SceneMemberRequests::GetScene);

            ViewId viewId;
            SceneRequestBus::EventResult(viewId, graphId, &SceneRequests::GetViewId);

            const bool forceUpdate = true;
            UpdateMenuDisplay(viewId, forceUpdate);

            m_menuDisplayDirty = false;
        }
    }

    void ComboBoxNodePropertyDisplay::UpdateMenuDisplay(const ViewId& viewId, bool forceUpdate)
    {
        if (m_proxyWidget && m_comboBox && (m_comboBox->IsMenuVisible() || forceUpdate))
        {
            QPointF scenePoint = m_proxyWidget->mapToScene(QPoint(0, aznumeric_cast<int>(m_proxyWidget->size().height())));
            QPointF widthPoint = m_proxyWidget->mapToScene(QPoint(aznumeric_cast<int>(m_proxyWidget->size().width()), aznumeric_cast<int>(m_proxyWidget->size().height())));

            AZ::Vector2 globalPoint;
            ViewRequestBus::EventResult(globalPoint, viewId, &ViewRequests::MapToGlobal, ConversionUtils::QPointToVector(scenePoint));

            AZ::Vector2 globalWidthPoint;
            ViewRequestBus::EventResult(globalWidthPoint, viewId, &ViewRequests::MapToGlobal, ConversionUtils::QPointToVector(widthPoint));

            m_comboBox->SetAnchorPoint(ConversionUtils::AZToQPoint(globalPoint).toPoint());

            qreal menuWidth = globalPoint.GetDistance(globalWidthPoint);
            m_comboBox->SetMenuWidth(menuWidth);
        }
        else
        {
            m_menuDisplayDirty = true;
        }
    }
}
