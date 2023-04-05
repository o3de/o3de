/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QLineEdit>
#include <QGraphicsProxyWidget>

#include <Components/NodePropertyDisplays/StringNodePropertyDisplay.h>

#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Widgets/NodePropertyBus.h>
#include <Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    //////////////////////////////
    // StringNodePropertyDisplay
    //////////////////////////////
    StringNodePropertyDisplay::StringNodePropertyDisplay(StringDataInterface* dataInterface)
        : NodePropertyDisplay(dataInterface)        
        , m_dataInterface(dataInterface)
        , m_displayLabel(nullptr)
        , m_lineEdit(nullptr)
        , m_proxyWidget(nullptr)        
        , m_isNudging(false)
    {
        m_dataInterface->RegisterDisplay(this);        
        
        m_disabledLabel = aznew GraphCanvasLabel();
        m_displayLabel = aznew GraphCanvasLabel();

        if (m_dataInterface->ResizeToContents())
        {
            m_displayLabel->SetWrapMode(GraphCanvasLabel::WrapMode::ResizeToContent);
            m_displayLabel->SetElide(false);
        }
    }
    
    StringNodePropertyDisplay::~StringNodePropertyDisplay()
    {
        CleanupProxyWidget();
        delete m_dataInterface;
        delete m_displayLabel;
        delete m_disabledLabel;
    }

    void StringNodePropertyDisplay::RefreshStyle()
    {
        m_disabledLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisabledLabelStyle("string").c_str());
        m_displayLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisplayLabelStyle("string").c_str());

        QSizeF minimumSize = m_displayLabel->minimumSize();

        if (m_lineEdit)
        {
            m_lineEdit->setMinimumSize(aznumeric_cast<int>(minimumSize.width()), aznumeric_cast<int>(minimumSize.height()));

            if (m_dataInterface->ResizeToContents())
            {
                ResizeToContents();
            }
            else
            {
                int minWidth = aznumeric_cast<int>(m_displayLabel->size().width());
                m_lineEdit->setFixedWidth(AZStd::max(minWidth, 10));
            }
        }
    }
    
    void StringNodePropertyDisplay::UpdateDisplay()
    {
        AZStd::string value = m_dataInterface->GetString();
        m_displayLabel->SetLabel(value);
        
        if (m_lineEdit)
        {
            QSignalBlocker signalBlocker(m_lineEdit);
            m_lineEdit->setText(value.c_str());
            m_lineEdit->setCursorPosition(0);
        }

        ResizeToContents();
    }

    QGraphicsLayoutItem* StringNodePropertyDisplay::GetDisabledGraphicsLayoutItem()
    {
        CleanupProxyWidget();
        return m_disabledLabel;
    }

    QGraphicsLayoutItem* StringNodePropertyDisplay::GetDisplayGraphicsLayoutItem()
    {
        CleanupProxyWidget();
        return m_displayLabel;
    }

    QGraphicsLayoutItem* StringNodePropertyDisplay::GetEditableGraphicsLayoutItem()
    {
        SetupProxyWidget();
        return m_proxyWidget;
    }

    void StringNodePropertyDisplay::OnDragDropStateStateChanged(const DragDropState& dragState)
    {
        Styling::StyleHelper& styleHelper = m_displayLabel->GetStyleHelper();
        UpdateStyleForDragDrop(dragState, styleHelper);
        m_displayLabel->update();
    }

    void StringNodePropertyDisplay::OnSystemTick()
    {
        EditFinished();
        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void StringNodePropertyDisplay::EditStart()
    {
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::LockEditState, this);

        SceneRequestBus::Event(GetSceneId(), &SceneRequests::CancelNudging);

        TryAndSelectNode();
    }

    void StringNodePropertyDisplay::SubmitValue()
    {
        if (m_lineEdit)
        {
            AZStd::string value = m_lineEdit->text().toUtf8().data();
            m_dataInterface->SetString(value);

            m_lineEdit->setCursorPosition(m_lineEdit->text().size());
            m_lineEdit->selectAll();
        }
        else
        {
            AZ_Error("GraphCanvas", false, "line edit doesn't exist!");
        }
    }
    
    void StringNodePropertyDisplay::EditFinished()
    {
        SubmitValue();
        UpdateDisplay();
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::UnlockEditState, this);

        if (m_isNudging)
        {
            m_isNudging = false;
            SceneRequestBus::Event(GetSceneId(), &SceneRequests::FinalizeNudging);
        }
    }

    void StringNodePropertyDisplay::OnFocusOut()
    {
        // String property changes can sometimes change the visual layouts of nodes.
        // Need to delay this to the start of the next tick to avoid running into
        // issues with Qt processing.
        AZ::SystemTickBus::Handler::BusConnect();
    }

    void StringNodePropertyDisplay::SetupProxyWidget()
    {
        if (!m_lineEdit)
        {
            m_proxyWidget = new QGraphicsProxyWidget();
            m_lineEdit = aznew Internal::FocusableLineEdit();
            m_lineEdit->setProperty("HasNoWindowDecorations", true);
            m_lineEdit->setEnabled(true);

            QObject::connect(m_lineEdit, &QLineEdit::textChanged, [this]() { ResizeToContents(); });
            QObject::connect(m_lineEdit, &Internal::FocusableLineEdit::OnFocusIn, [this]() { EditStart(); });
            QObject::connect(m_lineEdit, &Internal::FocusableLineEdit::OnFocusOut, [this]() { EditFinished(); });
            QObject::connect(m_lineEdit, &QLineEdit::editingFinished, [this]() {
                SubmitValue();
                UpdateDisplay();
            });

            m_proxyWidget->setWidget(m_lineEdit);
            UpdateDisplay();
            RefreshStyle();
            RegisterShortcutDispatcher(m_lineEdit);
        }
    }

    void StringNodePropertyDisplay::CleanupProxyWidget()
    {
        if (m_lineEdit)
        {
            UnregisterShortcutDispatcher(m_lineEdit);
            delete m_lineEdit; // NB: this implicitly deletes m_proxy widget
            m_lineEdit = nullptr;
            m_proxyWidget = nullptr;
        }
    }

    void StringNodePropertyDisplay::ResizeToContents()
    {
        if (m_lineEdit && m_dataInterface->ResizeToContents())
        {
            int originalWidth = m_lineEdit->width();

            m_displayLabel->SetLabel(m_lineEdit->text().toUtf8().data());

            QFontMetrics fm = m_lineEdit->fontMetrics();
            int width = aznumeric_cast<int>(fm.boundingRect(m_lineEdit->text()).width());

            if (width < m_displayLabel->minimumSize().width())
            {
                width = aznumeric_cast<int>(m_displayLabel->minimumSize().width()) + 2;
            }

            m_lineEdit->setFixedWidth(width);

            // Don't want to start nudging unless we actually have the focus
            if (width != originalWidth && m_lineEdit->hasFocus())
            {
                NodeUIRequestBus::Event(GetNodeId(), &NodeUIRequests::AdjustSize);

                if (!m_isNudging)
                {
                    m_isNudging = true;

                    AZStd::unordered_set< NodeId > fixedNodes = { GetNodeId() };
                    SceneRequestBus::Event(GetSceneId(), &SceneRequests::StartNudging, fixedNodes);
                }
            }
        }
        
        if (m_proxyWidget)
        {
            m_proxyWidget->update();
        }
    }

#include <Source/Components/NodePropertyDisplays/moc_StringNodePropertyDisplay.cpp>
}
