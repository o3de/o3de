/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector2.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that a scrollable component needs to implement. A scrollable component is one
//! that provides functionality to scroll its content
//! (e.g. UiScrollBoxComponent)
class UiScrollableInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiScrollableInterface() {}

    //! Get the ratio between the scrollable content size and the size of its parent
    virtual bool GetScrollableParentToContentRatio(AZ::Vector2& ratioOut) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiScrollableInterface> UiScrollableBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that listeners need to implement in order to get notifications when values of the
//! scrollable change
class UiScrollableNotifications
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiScrollableNotifications(){}

    //! Called when the scroll value (0 - 1) is changing
    virtual void OnScrollableValueChanging(AZ::Vector2 value) = 0;

    //! Called when the scroll value (0 - 1) has been changed
    virtual void OnScrollableValueChanged(AZ::Vector2 value) = 0;
};

typedef AZ::EBus<UiScrollableNotifications> UiScrollableNotificationBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that scrollers need to implement in order to get notifications of changes
//! created by the scrollable
class UiScrollableToScrollerNotifications
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiScrollableToScrollerNotifications(){}

    //! Called when the scrollable is changing the scroll value (0 - 1)
    virtual void OnValueChangingByScrollable(AZ::Vector2 value) = 0;

    //! Called when the scrollable is done changing the scroll value (0 - 1)
    virtual void OnValueChangedByScrollable(AZ::Vector2 value) = 0;

    //! Called when the content size or content parent size has been changed
    virtual void OnScrollableParentToContentRatioChanged(AZ::Vector2 parentToContentRatio) = 0;
};

typedef AZ::EBus<UiScrollableToScrollerNotifications> UiScrollableToScrollerNotificationBus;
