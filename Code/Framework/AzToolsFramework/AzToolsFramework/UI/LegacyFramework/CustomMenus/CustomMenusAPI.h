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

#ifndef CUSTOMMENUSAPI_H
#define CUSTOMMENUSAPI_H

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Crc.h>
#include <QtCore/QString>

#pragma once

class QMenu;

namespace AZ
{
    class ComponentApplication;
}

namespace LegacyFramework
{
    namespace CustomMenusCommon
    {
        struct WorldEditor
        {
            static const AZ::Crc32 Woodpecker;
            static const AZ::Crc32 File;
            static const AZ::Crc32 Debug;
            static const AZ::Crc32 Edit;
            static const AZ::Crc32 Build;
        };
        struct Driller
        {
            static const AZ::Crc32 Woodpecker;
            static const AZ::Crc32 DrillerMenu;
            static const AZ::Crc32 Channels;
        };
        struct LUAEditor
        {
            static const AZ::Crc32 Woodpecker;
            static const AZ::Crc32 File;
            static const AZ::Crc32 Edit;
            static const AZ::Crc32 View;
            static const AZ::Crc32 Debug;
            static const AZ::Crc32 SourceControl;
            static const AZ::Crc32 Options;
        };
        struct Viewport
        {
            static const AZ::Crc32 Layout;
            static const AZ::Crc32 Grid;
            static const AZ::Crc32 View;
        };
    }

    class CustomMenusMessages
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