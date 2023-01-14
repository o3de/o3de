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
    AZ_CLASS_ALLOCATOR_IMPL(EditorAxisAlignedBoxShapeComponentMode, AZ::SystemAllocator, 0)

    EditorAxisAlignedBoxShapeComponentMode::EditorAxisAlignedBoxShapeComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType, bool allowAsymmetricalEditing)
        : BoxComponentMode(entityComponentIdPair, componentType, allowAsymmetricalEditing)
    {
    }

    AZStd::string EditorAxisAlignedBoxShapeComponentMode::GetComponentModeName() const
    {
        return "Axis Aligned Box Edit Mode";
    }

    AZ::Uuid EditorAxisAlignedBoxShapeComponentMode::GetComponentModeType() const
    {
        return azrtti_typeid<EditorAxisAlignedBoxShapeComponentMode>();
    }
} // namespace LmbrCentral
