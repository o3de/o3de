/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/DisableMenuActions/DisableMenuAction.h>

namespace GraphCanvas
{
    class SetEnabledStateMenuAction
        : public DisableContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(SetEnabledStateMenuAction, AZ::SystemAllocator, 0);

        SetEnabledStateMenuAction(QObject* parent);
        virtual ~SetEnabledStateMenuAction() = default;

        void SetEnableState(bool enableState);

        using DisableContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
        
    private:
    
        bool m_enableState;
    };
}
