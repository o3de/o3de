/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Pass/State/EditorStateParentPassBase.h>

namespace AZ::Render
{
    //! Class for the Selected Entity outline editor state effect.
    class SelectedEntityParentPass
        : public EditorStateParentPassBase
    {
    public:
        SelectedEntityParentPass();

        // EditorModeStateParentPass overrides ...
        void UpdatePassData(RPI::ParentPass* parentPass) override;

        // EditorStateParentPassBase overrides ...
        AzToolsFramework::EntityIdList GetMaskedEntities() const override;
    };
} // namespace AZ::Render
