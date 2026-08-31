/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/State/FocusedEntityState.h>

#include <AzToolsFramework/API/ViewportEditorModeTrackerInterface.h>
#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>

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

    FocusedEntityState::FocusedEntityState(AZStd::function<AzFramework::EntityContextId()> worldIdProvider)
        : EditorStateBase(EditorState::FocusMode, "FocusMode", CreateFocusedEntityChildPasses())
        , m_worldIdProvider(AZStd::move(worldIdProvider))
    {
        SetEnabled(AzToolsFramework::PrefabEditModeEffectEnabled());
    }

    bool FocusedEntityState::IsEnabled() const
    {
        auto* editorModeTracker = AZ::Interface<AzToolsFramework::ViewportEditorModeTrackerInterface>::Get();
        const auto* editorModes =
            editorModeTracker ? editorModeTracker->GetViewportEditorModes({ m_worldIdProvider() }) : nullptr;
        return EditorStateBase::IsEnabled() && editorModes &&
            editorModes->IsModeActive(AzToolsFramework::ViewportEditorMode::Focus);
    }

    AzToolsFramework::EntityIdList FocusedEntityState::GetMaskedEntities() const
    {
        const auto focusModeInterface = AZ::Interface<AzToolsFramework::FocusModeInterface>::Get();
        if (!focusModeInterface)
        {
            return {};
        }

        return focusModeInterface->GetFocusedEntities(m_worldIdProvider());
    }
} // namespace AZ::Render
