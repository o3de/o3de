/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiTransformBus.h>
#include <AzFramework/Input/Channels/InputChannelDigitalWithSharedModifierKeyStates.h>
#include <LyShine/UiBase.h>

namespace AzFramework
{
    class InputChannelId;
}

namespace UiNavigationHelpers
{
    enum class Command
    {
        Up,
        Down,
        Left,
        Right,
        Enter,
        Back,
        NavEnd,
        NavHome,

        Unknown
    };

    //! Map input channel ids to interactable ui commands
    UiNavigationHelpers::Command MapInputChannelIdToUiNavigationCommand(const AzFramework::InputChannelId& inputChannelId, AzFramework::ModifierKeyMask activeModifierKeys);

    using ValidationFunction = AZStd::function<bool(AZ::EntityId)>;

    //! Find the next element given the current element and a direction
    AZ::EntityId GetNextElement(AZ::EntityId curEntityId, Command command,
        const LyShine::EntityArray& navigableElements, AZ::EntityId defaultEntityId,
        ValidationFunction isValidResult, AZ::EntityId parentElement = AZ::EntityId());

    //! Find the next element in the given direction for automatic mode
    AZ::EntityId SearchForNextElement(AZ::EntityId curElement, Command command,
        const LyShine::EntityArray& navigableElements, AZ::EntityId parentElement = AZ::EntityId());

    //! Follow one of the custom links (used when navigation mode is custom)
    AZ::EntityId FollowCustomLink(AZ::EntityId curEntityId, Command command);

    //! Check if an interactable can be navigated to
    bool IsInteractableNavigable(AZ::EntityId interactableEntityId);

    //! Check if an element is an interactable that can be navigated to
    bool IsElementInteractableAndNavigable(AZ::EntityId entityId);

    //! Make a list of all navigable & interactable elements under the specified parent
    void FindNavigableInteractables(AZ::EntityId parentElement, AZ::EntityId ignoreElement, LyShine::EntityArray& result);

    //! Find the first ancestor that's a navigable interactable
    AZ::EntityId FindAncestorNavigableInteractable(AZ::EntityId childInteractable, bool ignoreAutoActivatedAncestors = false);
} // namespace UiNavigationHelpers
