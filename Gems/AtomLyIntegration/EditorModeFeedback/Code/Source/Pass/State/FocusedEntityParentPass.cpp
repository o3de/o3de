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
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelectionRequestBus.h>

namespace AZ::Render
{
    static PassDescriptorList CreateFocusedEntityChildPasses()
    {
        return PassDescriptorList
        {
            // Black and white effect for unfocused entities (scaled by distance)
            AZ::Name("EditorModeDesaturationTemplate"),

            // Darkening effect for unfocused entities (scaled by distance)
            AZ::Name("EditorModeTintTemplate"),

            // Blurring effect for unfocused entities (scaled by distance)
            AZ::Name("EditorModeBlurParentTemplate")
        };
    }

    FocusedEntityParentPass::FocusedEntityParentPass()
        : EditorStateParentPassBase(EditorState::FocusMode, "FocusMode", CreateFocusedEntityChildPasses())
    {
        AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusConnect(AzToolsFramework::GetEntityContextId());
    }

    FocusedEntityParentPass::~FocusedEntityParentPass()
    {
        AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
    }

    void FocusedEntityParentPass::OnEditorModeActivated(
        [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
        AzToolsFramework::ViewportEditorMode mode)
    {
        if (mode == AzToolsFramework::ViewportEditorMode::Focus)
        {
            m_inFocusMode = true;
        }
    }
    void FocusedEntityParentPass::OnEditorModeDeactivated(
        [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode)
    {   
        if (mode == AzToolsFramework::ViewportEditorMode::Focus)
        {
            m_inFocusMode = false;
        }
    }

    bool FocusedEntityParentPass::IsEnabled() const
    {
        return EditorStateParentPassBase::IsEnabled() && m_inFocusMode;
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
