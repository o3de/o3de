/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentModes/BoxComponentMode.h>

namespace LmbrCentral
{
    class EditorAxisAlignedBoxShapeComponentMode : public AzToolsFramework::BoxComponentMode
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(EditorAxisAlignedBoxShapeComponentMode, "{39F7A2E2-5760-452B-A777-BAB76C66AC2E}", EditorBaseComponentMode)

        EditorAxisAlignedBoxShapeComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType, bool allowAsymmetricalEditing);

        static void Reflect(AZ::ReflectContext* context);

        static void BindActionsToModes();

        //! AzToolsFramework::BoxComponentMode overrides ...
        AZStd::string GetComponentModeName() const override;
        AZ::Uuid GetComponentModeType() const override;
    };
} // namespace LmbrCentral
