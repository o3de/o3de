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
    AZ_TYPE_INFO_SPECIALIZE(EditorAxisAlignedBoxShapeComponentMode, "{39F7A2E2-5760-452B-A777-BAB76C66AC2E}");
    AZ_CLASS_ALLOCATOR_IMPL(EditorAxisAlignedBoxShapeComponentMode, AZ::SystemAllocator)

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
        return AZ::AzTypeInfo<EditorAxisAlignedBoxShapeComponentMode>::Uuid();
    }
} // namespace LmbrCentral
