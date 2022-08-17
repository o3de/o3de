/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/EditorImageGradientComponentMode.h>
#include <Editor/EditorImageGradientRequestBus.h>
#include <GradientSignal/Ebuses/ImageGradientModificationBus.h>

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
        ImageGradientModificationBus::Event(GetEntityId(), &ImageGradientModifications::StartImageModification);
    }

    EditorImageGradientComponentMode::~EditorImageGradientComponentMode()
    {
        ImageGradientModificationBus::Event(GetEntityId(), &ImageGradientModifications::EndImageModification);
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
            ImageGradientModificationBus::EventResult(pixelBuffer, GetEntityId(), &ImageGradientModifications::GetImageModificationBuffer);

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
