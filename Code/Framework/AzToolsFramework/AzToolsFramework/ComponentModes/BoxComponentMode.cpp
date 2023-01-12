/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BoxComponentMode.h"

namespace AzToolsFramework
{
    AZ_CLASS_ALLOCATOR_IMPL(BoxComponentMode, AZ::SystemAllocator, 0)

    void BoxComponentMode::Reflect(AZ::ReflectContext* context)
    {
        AzToolsFramework::ComponentModeFramework::ReflectEditorBaseComponentModeDescendant<BoxComponentMode>(context);
    }

    BoxComponentMode::BoxComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType, bool allowAsymmetricalEditing)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
        m_boxEdit.Setup(entityComponentIdPair, allowAsymmetricalEditing);
    }

    BoxComponentMode::~BoxComponentMode()
    {
        m_boxEdit.Teardown();
    }

    void BoxComponentMode::Refresh()
    {
        m_boxEdit.UpdateManipulators();
    }

    AZStd::string BoxComponentMode::GetComponentModeName() const
    {
        return "Box Edit Mode";
    }

    AZ::Uuid BoxComponentMode::GetComponentModeType() const
    {
        return azrtti_typeid<BoxComponentMode>();
    }
} // namespace AzToolsFramework
