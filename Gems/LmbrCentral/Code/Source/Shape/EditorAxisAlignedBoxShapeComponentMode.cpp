/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorAxisAlignedBoxShapeComponentMode.h"

namespace LmbrCentral
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorAxisAlignedBoxShapeComponentMode, AZ::SystemAllocator)

    EditorAxisAlignedBoxShapeComponentMode::EditorAxisAlignedBoxShapeComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType, bool allowAsymmetricalEditing)
        : BoxComponentMode(entityComponentIdPair, componentType, allowAsymmetricalEditing)
    {
    }

    void EditorAxisAlignedBoxShapeComponentMode::Reflect(AZ::ReflectContext* context)
    {
        AzToolsFramework::ComponentModeFramework::ReflectEditorBaseComponentModeDescendant<EditorAxisAlignedBoxShapeComponentMode>(context);
    }

    void EditorAxisAlignedBoxShapeComponentMode::BindActionsToModes()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        BaseShapeComponentMode::BindActionsToModes(
            "box", serializeContext->FindClassData(azrtti_typeid<EditorAxisAlignedBoxShapeComponentMode>())->m_name);
    }

    AZStd::string EditorAxisAlignedBoxShapeComponentMode::GetComponentModeName() const
    {
        return "Axis Aligned Box Edit Mode";
    }

    AZ::Uuid EditorAxisAlignedBoxShapeComponentMode::GetComponentModeType() const
    {
        return AZ::AzTypeInfo<EditorAxisAlignedBoxShapeComponentMode>::Uuid();
    }
} // namespace LmbrCentral
