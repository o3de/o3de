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

#include <Editor/EditorGradientPainterComponentMode.h>
#include <GradientSignal/Editor/EditorGradientPainterRequestBus.h>

#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace GradientSignal
{
    EditorGradientPainterComponentMode::EditorGradientPainterComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
    }

    EditorGradientPainterComponentMode::~EditorGradientPainterComponentMode()
    {
        GradientPainterRequestBus::Event(GetEntityId(), &GradientPainterRequests::SaveImage);
    }

    AZStd::vector<AzToolsFramework::ActionOverride> EditorGradientPainterComponentMode::PopulateActionsImpl()
    {
        // MAB TODO: FIXME - what should this return?
        return {};
    }

    AZStd::string EditorGradientPainterComponentMode::GetComponentModeName() const
    {
        return "Gradient Painter Paint Mode";
    }

    bool EditorGradientPainterComponentMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        bool result = false;

        if (mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Down)
        {
            AZStd::vector<float>* pixelBuffer;
            GradientPainterRequestBus::EventResult(pixelBuffer, GetEntityId(), &GradientPainterRequests::GetPixelBuffer);

            for (auto& pixel : *pixelBuffer)
            {
                pixel = (1.0f - pixel);
            }
        }

        GradientPainterRequestBus::Event(GetEntityId(), &GradientPainterRequests::RefreshPreview);

        return result;
    }

} // namespace GradientSignal
