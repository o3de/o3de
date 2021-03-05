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

#include <LyShine/Bus/UiTransformBus.h>
#include <AzFramework/Input/Channels/InputChannelDigitalWithSharedModifierKeyStates.h>

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
