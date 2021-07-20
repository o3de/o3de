/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

// Forward declarations
class ISprite;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that listeners need to implement to be notified of changes to the sprite settings
class UiSpriteSettingsChangeNotification
    : public AZ::EBusTraits
{
public:
    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides
    /**
        * Overrides the default AZ::EBusTraits address policy so that the bus
        * has multiple addresses at which to receive messages. This bus is 
        * identified by Sprite pointer. Messages addressed to an ID are received by 
        * handlers connected to that ID.
        */
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    /**
        * Overrides the default AZ::EBusTraits ID type so that Sprite pointers are 
        * used to access the addresses of the bus.
        */
    typedef ISprite* BusIdType;
    //////////////////////////////////////////////////////////////////////////

    virtual ~UiSpriteSettingsChangeNotification() {}

    //! Called when the sprite settings such as number of cells etc change
    virtual void OnSpriteSettingsChanged() = 0;
};

typedef AZ::EBus<UiSpriteSettingsChangeNotification> UiSpriteSettingsChangeNotificationBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Notify listeners when sprite image sources change.
class UiSpriteSourceNotificationInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiSpriteSourceNotificationInterface() {}

    //! A sprite image (or sequence of images) has changed file sources.
    virtual void OnSpriteSourceChanged() = 0;
};

typedef AZ::EBus<UiSpriteSourceNotificationInterface> UiSpriteSourceNotificationBus;
