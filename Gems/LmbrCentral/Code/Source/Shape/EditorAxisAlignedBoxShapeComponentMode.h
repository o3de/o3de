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

        EditorAxisAlignedBoxShapeComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);

        //! AzToolsFramework::BoxComponentMode overrides ...
        AZStd::string GetComponentModeName() const override;
    };
} // namespace LmbrCentral
