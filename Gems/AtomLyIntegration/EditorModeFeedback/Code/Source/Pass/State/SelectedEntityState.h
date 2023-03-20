/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Pass/State/EditorStateBase.h>

namespace AZ::Render
{
    //! Class for the Selected Entity outline editor state effect.
    class SelectedEntityState
        : public EditorStateBase
    {
    public:
        SelectedEntityState();

        // EditorModeStateParentPass overrides ...
        void UpdatePassData(RPI::ParentPass* parentPass) override;

        // EditorStateBase overrides ...
        AzToolsFramework::EntityIdList GetMaskedEntities() const override;
    };
} // namespace AZ::Render
