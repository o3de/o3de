/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/State/FocusedEntityParentPass.h>

#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AZ::Render
{
    static PassDescriptorList CreateFocusedEntityChildPasses()
    {
        return PassDescriptorList
        {
            // Black and white effect for unfocused entities (scaled by distance)
            { AZ::Name("DesaturationPass"), AZ::Name("EditorModeDesaturationTemplate") },

            // Darkening effect for unfocused entities (scaled by distance)
            { AZ::Name("TintPass"), AZ::Name("EditorModeTintTemplate") },

            // Blurring effect for unfocused entities (scaled by distance)
            { AZ::Name("BlurPass"), AZ::Name("EditorModeBlurParentTemplate") }
        };
    }

    FocusedEntityParentPass::FocusedEntityParentPass()
        : EditorStateParentPassBase("FocusMode", CreateFocusedEntityChildPasses())
    {
    }

    // ViewportEditorModeNotificationsBus overrides ...
    void FocusedEntityParentPass::OnEditorModeActivated(
        [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
        AzToolsFramework::ViewportEditorMode mode)
    {
        if (mode == AzToolsFramework::ViewportEditorMode::Focus)
        {
            m_enabled = true;
        }
    }
    void FocusedEntityParentPass::OnEditorModeDeactivated(
        [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode)
    {
        if (mode == AzToolsFramework::ViewportEditorMode::Focus)
        {
            m_enabled = false;
        }
    }

    // EditorModeFocusModeStateRequestBus overrides ...
    // void SetUnfocusDesaturation(float intensity);
    // void SetUnfocusTint(const AZ::Color& tint);
    // void SetUnfocusBlur(float intensity);

    // EditorModeStateParentPass overrides ...
    void FocusedEntityParentPass::InitPassData([[maybe_unused]] RPI::ParentPass* parentPass)
    {

    }

    bool FocusedEntityParentPass::IsEnabled() const
    {
        return m_enabled;
    }

    AzToolsFramework::EntityIdList FocusedEntityParentPass::GetMaskedEntities() const
    {
        const auto focusModeInterface = AZ::Interface<AzToolsFramework::FocusModeInterface>::Get();
        if (!focusModeInterface)
        {
            return {};
        }

        return focusModeInterface->GetFocusedEntities(AzToolsFramework::GetEntityContextId());
    }
}
