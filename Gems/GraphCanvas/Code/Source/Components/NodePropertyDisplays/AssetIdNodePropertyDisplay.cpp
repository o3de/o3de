/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QGraphicsProxyWidget>

#include <Components/NodePropertyDisplays/AssetIdNodePropertyDisplay.h>

#include <Widgets/GraphCanvasLabel.h>

#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>

namespace GraphCanvas
{
    AssetIdNodePropertyDisplay::AssetIdNodePropertyDisplay(AssetIdDataInterface* dataInterface)
        : NodePropertyDisplay(dataInterface)
        , m_dataInterface(dataInterface)
        , m_propertyAssetCtrl(nullptr)
        , m_proxyWidget(nullptr)
    {
        m_dataInterface->RegisterDisplay(this);

        m_disabledLabel = aznew GraphCanvasLabel();
        m_displayLabel = aznew GraphCanvasLabel();
        m_displayLabel->SetWrapMode(GraphCanvasLabel::WrapMode::ResizeToContent);
    }

    AssetIdNodePropertyDisplay::~AssetIdNodePropertyDisplay()
    {
        CleanupProxyWidget();
        delete m_displayLabel;
        delete m_disabledLabel;

        delete m_dataInterface;
    }

    void AssetIdNodePropertyDisplay::RefreshStyle()
    {
        m_disabledLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisabledLabelStyle("assetId").c_str());

        AZStd::string styleName = NodePropertyDisplay::CreateDisplayLabelStyle("assetId");
        m_displayLabel->SetSceneStyle(GetSceneId(), styleName.c_str());
    }

    void AssetIdNodePropertyDisplay::UpdateDisplay()
    {
        AZ::Data::AssetId valueAssetId = m_dataInterface->GetAssetId();

        if (m_propertyAssetCtrl)
        {
            m_propertyAssetCtrl->SetSelectedAssetID(valueAssetId);

            QString displayLabel = m_propertyAssetCtrl->GetCurrentAssetHint().c_str();
            if (displayLabel.isEmpty())
            {
                displayLabel = "<None>";
            }

            m_displayLabel->SetLabel(displayLabel.toUtf8().data());

            QSizeF minimumSize = m_displayLabel->minimumSize();
            QSizeF maximumSize = m_displayLabel->maximumSize();

            m_propertyAssetCtrl->setMinimumSize(aznumeric_cast<int>(minimumSize.width()), aznumeric_cast<int>(minimumSize.height()));
            m_propertyAssetCtrl->setMaximumSize(aznumeric_cast<int>(maximumSize.width()), aznumeric_cast<int>(maximumSize.height()));
        } 

        if (m_proxyWidget)
        {
            m_proxyWidget->update();
        }
    }

    QGraphicsLayoutItem* AssetIdNodePropertyDisplay::GetDisabledGraphicsLayoutItem()
    {
        return m_disabledLabel;
    }

    QGraphicsLayoutItem* AssetIdNodePropertyDisplay::GetDisplayGraphicsLayoutItem()
    {
        return m_displayLabel;
    }

    QGraphicsLayoutItem* AssetIdNodePropertyDisplay::GetEditableGraphicsLayoutItem()
    {
        SetupProxyWidget();
        return m_proxyWidget;
    }

    void AssetIdNodePropertyDisplay::OnDragDropStateStateChanged(const DragDropState& dragState)
    {
        Styling::StyleHelper& styleHelper = m_displayLabel->GetStyleHelper();
        UpdateStyleForDragDrop(dragState, styleHelper);
        m_displayLabel->update();
    }

    void AssetIdNodePropertyDisplay::EditStart()
    {
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::LockEditState, this);
        TryAndSelectNode();
    }

    void AssetIdNodePropertyDisplay::EditFinished()
    {
        SubmitValue();
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::UnlockEditState, this);
    }

    void AssetIdNodePropertyDisplay::SubmitValue()
    {
        AZ_Assert(m_propertyAssetCtrl, "m_propertyAssetCtrl doesn't exist!");

        m_dataInterface->SetAssetId(m_propertyAssetCtrl->GetCurrentAssetID());

        UpdateDisplay();
    }

    void AssetIdNodePropertyDisplay::SetupProxyWidget()
    {
        if (!m_propertyAssetCtrl)
        {
            m_proxyWidget = new QGraphicsProxyWidget();

            m_proxyWidget->setFlag(QGraphicsItem::ItemIsFocusable, true);
            m_proxyWidget->setFocusPolicy(Qt::StrongFocus);

            m_propertyAssetCtrl = aznew AzToolsFramework::PropertyAssetCtrl(nullptr, m_dataInterface->GetStringFilter().c_str());
            m_propertyAssetCtrl->setProperty("HasNoWindowDecorations", true);
            m_propertyAssetCtrl->setProperty("DisableFocusWindowFix", true);
            m_propertyAssetCtrl->SetCurrentAssetType(m_dataInterface->GetAssetType());
            m_propertyAssetCtrl->SetDefaultAssetID(AZ::Data::AssetId());
            
            QObject::connect(m_propertyAssetCtrl, &AzToolsFramework::PropertyAssetCtrl::OnAssetIDChanged, [this]() { SubmitValue(); });

            m_proxyWidget->setWidget(m_propertyAssetCtrl);

            RegisterShortcutDispatcher(m_propertyAssetCtrl);
            UpdateDisplay();
            RefreshStyle();
        }
    }

    void AssetIdNodePropertyDisplay::CleanupProxyWidget()
    {
        if (m_propertyAssetCtrl)
        {
            UnregisterShortcutDispatcher(m_propertyAssetCtrl);
            delete m_propertyAssetCtrl; // this implicitly deletes m_proxyWidget
            m_propertyAssetCtrl = nullptr;
            m_proxyWidget = nullptr;
        }
    }
}
