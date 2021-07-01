/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <QComboBox>
#include <QGraphicsProxyWidget>
#include <QPushButton>

#include <Components/NodePropertyDisplays/BooleanNodePropertyDisplay.h>

#include <Widgets/GraphCanvasCheckBox.h>
#include <Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    ///////////////////////////////
    // BooleanNodePropertyDisplay
    ///////////////////////////////
    BooleanNodePropertyDisplay::BooleanNodePropertyDisplay(BooleanDataInterface* dataInterface)
        : NodePropertyDisplay(dataInterface)
        , m_dataInterface(dataInterface)
        , m_checkBox(nullptr)
    {
        m_disabledLabel = aznew GraphCanvasLabel();
        m_checkBox = aznew GraphCanvasCheckBox();
        
        GraphCanvasCheckBoxNotificationBus::Handler::BusConnect(m_checkBox);
    }
    
    BooleanNodePropertyDisplay::~BooleanNodePropertyDisplay()
    {
        GraphCanvasCheckBoxNotificationBus::Handler::BusDisconnect();

        delete m_dataInterface;

        delete m_checkBox;
        delete m_disabledLabel;
    }

    void BooleanNodePropertyDisplay::RefreshStyle()
    {
        m_disabledLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisabledLabelStyle("boolean").c_str());
        m_checkBox->SetSceneStyle(GetSceneId());
    }
    
    void BooleanNodePropertyDisplay::UpdateDisplay()
    {
        bool value = m_dataInterface->GetBool();
        m_checkBox->SetChecked(value);
    }

    QGraphicsLayoutItem* BooleanNodePropertyDisplay::GetDisabledGraphicsLayoutItem()
{
        return m_disabledLabel;
    }

    QGraphicsLayoutItem* BooleanNodePropertyDisplay::GetDisplayGraphicsLayoutItem()
{
        return m_checkBox;
    }

    QGraphicsLayoutItem* BooleanNodePropertyDisplay::GetEditableGraphicsLayoutItem()
{
        return m_checkBox;
    }

    void BooleanNodePropertyDisplay::OnValueChanged(bool value)
    {
        m_dataInterface->SetBool(value);
        UpdateDisplay();
    }
    
    void BooleanNodePropertyDisplay::OnClicked()
    {
        TryAndSelectNode();
    }
}
