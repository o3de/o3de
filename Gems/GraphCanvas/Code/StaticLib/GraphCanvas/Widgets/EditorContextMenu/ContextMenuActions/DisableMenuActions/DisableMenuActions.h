/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
        
    private:
    
        bool m_enableState;
    };
}