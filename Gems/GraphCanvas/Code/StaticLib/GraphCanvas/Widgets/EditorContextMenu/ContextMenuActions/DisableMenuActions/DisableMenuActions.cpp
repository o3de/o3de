/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/DisableMenuActions/DisableMenuActions.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>

namespace GraphCanvas
{
    //////////////////////////////
    // SetEnabledStateMenuAction
    //////////////////////////////
    
    SetEnabledStateMenuAction::SetEnabledStateMenuAction(QObject* object)
        : DisableContextMenuAction("Disable", object)
        , m_enableState(false)        
    {
    }

    void SetEnabledStateMenuAction::SetEnableState(bool enableState)
    {
        if (m_enableState != enableState)
        {
            m_enableState = enableState;

            if (m_enableState)
            {
                setText("Enable");
            }
            else
            {
                setText("Disable");
            }
        }
    }
    
    ContextMenuAction::SceneReaction SetEnabledStateMenuAction::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {
        const GraphId& graphId = GetGraphId();

        if (m_enableState)
        {
            SceneRequestBus::Event(graphId, &SceneRequests::EnableSelection);
        }
        else
        {
            SceneRequestBus::Event(graphId, &SceneRequests::DisableSelection);
        }
        
        return SceneReaction::PostUndo;
    }
}
