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

#include <Editor/EditorImageGradientComponentMode.h>
#include <Editor/EditorImageGradientRequestBus.h>

#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace GradientSignal
{
    EditorImageGradientComponentMode::EditorImageGradientComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
    }

    EditorImageGradientComponentMode::~EditorImageGradientComponentMode()
    {
        EditorImageGradientRequestBus::Event(GetEntityId(), &EditorImageGradientRequests::SaveImage);
    }

    AZStd::vector<AzToolsFramework::ActionOverride> EditorImageGradientComponentMode::PopulateActionsImpl()
    {
        return {};
    }

    AZStd::string EditorImageGradientComponentMode::GetComponentModeName() const
    {
        return "Image Gradient Paint Mode";
    }

    bool EditorImageGradientComponentMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        bool result = false;

        if (mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Down)
        {
            AZStd::vector<float>* pixelBuffer;
            EditorImageGradientRequestBus::EventResult(pixelBuffer, GetEntityId(), &EditorImageGradientRequests::GetPixelBuffer);

            if (pixelBuffer)
            {
                for (auto& pixel : *pixelBuffer)
                {
                    pixel = (1.0f - pixel);
                }
            }
        }

        // This will eventually need to change to broadcast OnCompositionRegionChanged.
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);

        return result;
    }

} // namespace GradientSignal
