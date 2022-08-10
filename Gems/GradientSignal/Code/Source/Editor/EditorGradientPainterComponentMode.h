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

#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>

namespace GradientSignal
{
    class EditorGradientPainterComponentMode
        : public AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode
    {
    public:
        EditorGradientPainterComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
        ~EditorGradientPainterComponentMode() override;

        // EditorBaseComponentMode
        void Refresh() override
        {
        }
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;
        bool HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
        AZStd::string GetComponentModeName() const override;
    };
} // namespace GradientSignal
