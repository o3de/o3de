/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CUSTOMMENUSAPI_H
#define CUSTOMMENUSAPI_H

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Crc.h>
#include <QtCore/QString>
#include <AzToolsFramework/UI/LegacyFramework/CustomMenus/CustomMenusCommon.h>
#include <AzToolsFramework/AzToolsFrameworkAPI.h>
#pragma once

class QMenu;

namespace AZ
{
    class ComponentApplication;
}

namespace LegacyFramework
{
    class AZTF_API CustomMenusMessages
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Single;
        typedef AZ::EBus<CustomMenusMessages> Bus;

        virtual ~CustomMenusMessages() {}

        //when a menu item is clicked, the orignal entryId used to register this callback will be passed back to you.
        using MenuSelectedType = AZStd::function<void(AZ::Crc32)>;

        virtual void RegisterMenu(AZ::Crc32 menuId, QMenu* menu) = 0;
        //can pass AZ::Crc32() (i.e. 0) for hotkeyId if you don't want to assign a hotkey
        virtual void AddMenuEntry(AZ::Crc32 menuId, AZ::Crc32 entryId, const QString& menuText, AZ::Crc32 hotkeyId, MenuSelectedType callback) = 0;
    };

    //Typically want a component factory to listen for this, and register its menus using CustomMenusMessages bus when it gets the
    //RegisterMenuEntries call. RegisterMenuEntries only gets called once with the CustomMenusComponent is activated, so only
    //things that are brought up early can listen in on this, like a component factory.
    class CustomMenusRegistration
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple;
        typedef AZ::EBus<CustomMenusRegistration> Bus;

        virtual ~CustomMenusRegistration() {}

        virtual void RegisterMenuEntries() = 0;
    };
}

#endif
