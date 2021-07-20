/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <Components/NodePropertyDisplays/ReadOnlyNodePropertyDisplay.h>

#include <Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    ////////////////////////////////
    // ReadOnlyNodePropertyDisplay
    ////////////////////////////////

    ReadOnlyNodePropertyDisplay::ReadOnlyNodePropertyDisplay(ReadOnlyDataInterface* dataInterface)
        : NodePropertyDisplay(dataInterface)
        , m_dataInterface(dataInterface)
    {
        m_disabledLabel = aznew GraphCanvasLabel();
        m_displayLabel = aznew GraphCanvasLabel();
    }
    
    ReadOnlyNodePropertyDisplay::~ReadOnlyNodePropertyDisplay()
    {
        delete m_dataInterface;
        
        delete m_displayLabel;
        delete m_disabledLabel;
    }

    void ReadOnlyNodePropertyDisplay::RefreshStyle()
    {
        m_disabledLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisabledLabelStyle("readOnly").c_str());
        m_displayLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisplayLabelStyle("readOnly").c_str());
    }
    
    void ReadOnlyNodePropertyDisplay::UpdateDisplay()
    {
        AZStd::string value = m_dataInterface->GetString();
        
        m_displayLabel->SetLabel(value);
        m_displayLabel->setToolTip(value.c_str());
    }

    QGraphicsLayoutItem* ReadOnlyNodePropertyDisplay::GetDisabledGraphicsLayoutItem()
{
        return m_disabledLabel;
    }

    QGraphicsLayoutItem* ReadOnlyNodePropertyDisplay::GetDisplayGraphicsLayoutItem()
{
        return m_displayLabel;
    }

    QGraphicsLayoutItem* ReadOnlyNodePropertyDisplay::GetEditableGraphicsLayoutItem()
{
        return m_displayLabel;
    }

    void ReadOnlyNodePropertyDisplay::OnDragDropStateStateChanged(const DragDropState& dragState)
    {
        Styling::StyleHelper& styleHelper = m_displayLabel->GetStyleHelper();
        UpdateStyleForDragDrop(dragState, styleHelper);
        m_displayLabel->update();
    }
}
