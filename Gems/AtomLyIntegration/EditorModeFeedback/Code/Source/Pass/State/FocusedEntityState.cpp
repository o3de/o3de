/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/State/FocusedEntityState.h>

#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelectionRequestBus.h>

namespace AZ::Render
{
    static PassNameList CreateFocusedEntityChildPasses()
    {
        // Effect chain for the non-focused entities.
        return PassNameList
        {
            // Black and white effect for unfocused entities (scaled by distance)
            AZ::Name("EditorModeDesaturationTemplate"),

            // Darkening effect for unfocused entities (scaled by distance)
            AZ::Name("EditorModeTintTemplate"),

            // Blurring effect for unfocused entities (scaled by distance)
            AZ::Name("EditorModeBlurParentTemplate")
        };
    }

    FocusedEntityState::FocusedEntityState()
        : EditorStateBase(EditorState::FocusMode, "FocusMode", CreateFocusedEntityChildPasses())
    {
        AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusConnect(AzToolsFramework::GetEntityContextId());
        SetEnabled(AzToolsFramework::PrefabEditModeEffectEnabled());
    }

    FocusedEntityState::~FocusedEntityState()
    {
        AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
    }

    void FocusedEntityState::OnEditorModeActivated(
        [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
        AzToolsFramework::ViewportEditorMode mode)
    {
        if (AzToolsFramework::ViewportEditorMode::Focus == mode)
        {
            m_inFocusMode = true;
        }
    }
    void FocusedEntityState::OnEditorModeDeactivated(
        [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode)
    {
        if (AzToolsFramework::ViewportEditorMode::Focus == mode)
        {
            m_inFocusMode = false;
        }
    }

    bool FocusedEntityState::IsEnabled() const
    {
        return EditorStateBase::IsEnabled() && m_inFocusMode;
    }

    AzToolsFramework::EntityIdList FocusedEntityState::GetMaskedEntities() const
    {
        const auto focusModeInterface = AZ::Interface<AzToolsFramework::FocusModeInterface>::Get();
        if (!focusModeInterface)
        {
            return {};
        }

        return focusModeInterface->GetFocusedEntities(AzToolsFramework::GetEntityContextId());
    }
} // namespace AZ::Render

