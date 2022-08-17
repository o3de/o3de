/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>

namespace GradientSignal
{
    class EditorImageGradientComponentMode
        : public AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode
    {
    public:
        EditorImageGradientComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
        ~EditorImageGradientComponentMode() override;

        // EditorBaseComponentMode
        void Refresh() override
        {
        }
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;
        bool HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
        AZStd::string GetComponentModeName() const override;
    };
} // namespace GradientSignal
