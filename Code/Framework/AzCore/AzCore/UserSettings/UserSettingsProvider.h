/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_USER_SETTINGS_PROVIDER_H
#define AZCORE_USER_SETTINGS_PROVIDER_H

#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    class SerializeContext;
    class ReflectContext;

    /**
     * Messages to get/set user data. We use smart pointers to make sure
     * we don't delete any data before it's saved and safe for delete.
     */
    class UserSettingsMessages
        : public EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits settings
        // Do we need more than one user data management system ???
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        typedef u32 BusIdType;
        //////////////////////////////////////////////////////////////////////////

        UserSettingsMessages() {}
        virtual ~UserSettingsMessages() {}

        /// Returns pointer to stored user data, or nullptr if no data has been stored.
        virtual AZStd::intrusive_ptr<UserSettings>  FindUserSettings(u32 id) = 0;

        /**
         * Add user settings into the system, they are stored on exit and loaded on
         * start.
         * IMPORTANT: Your derived user settings MUST be reflected in the SerializeContext (usually in
         * your component serialize function).
         * IMPORTANT: Make sure the user settings handler is started otherwise you data will not be stored!
         * \returns true if user settings was set and false, if user settings with this ID already exist.
         * IMPORTANT: After calling AddUserSettings the handler(s) will assume ownership of the setting
         * and free it using 'delete' (which can be changed if needed)
         */
        virtual void            AddUserSettings(u32 id, UserSettings* settings) = 0;

        /**
         * Save the current settings to the current path specified
         */
        virtual bool            Save(const char* settingsPath, SerializeContext* sc) = 0;
    };
    typedef EBus<UserSettingsMessages> UserSettingsBus;

    class UserSettingsOwnerRequests
        : public EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits settings
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        typedef u32 BusIdType;
        //////////////////////////////////////////////////////////////////////////

        virtual void SaveSettings() = 0;
    };

    typedef EBus<UserSettingsOwnerRequests> UserSettingsOwnerRequestBus;

    class UserSettingsNotifications
        : public EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits settings
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        typedef u32 BusIdType;
        //////////////////////////////////////////////////////////////////////////

        virtual void OnUserSettingsActivated() {};
    };

    typedef EBus<UserSettingsNotifications> UserSettingsNotificationBus;

    /**
     *
     */
    struct UserSettingsContainer
    {
        AZ_CLASS_ALLOCATOR(UserSettingsContainer, SystemAllocator);
        AZ_TYPE_INFO(UserSettingsContainer, "{42C087CF-4F19-4DAF-B2B1-2927D94AA295}")

        typedef AZStd::unordered_map<AZ::u32, AZStd::intrusive_ptr<UserSettings> >  MapType;
        MapType             m_map;
    };

    /**
     *
     */
    class UserSettingsProvider
        : public UserSettingsBus::Handler
    {
    public:
        void    Activate(u32 bindToProviderId);
        void    Deactivate();
        bool    Load(const char* settingsPath, SerializeContext* sc);

        //////////////////////////////////////////////////////////////////////////
        // UserSettingsBus
        AZStd::intrusive_ptr<UserSettings>  FindUserSettings(u32 id) override;
        void                                AddUserSettings(u32 id, UserSettings* settings) override;
        bool                                Save(const char* settingsPath, SerializeContext* sc) override;
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(ReflectContext* reflection);

    protected:
        //////////////////////////////////////////////////////////////////////////
        // Callbacks for loading user settings
        void    OnSettingLoaded(void* classPtr, const Uuid& classId, const SerializeContext* sc);
        //////////////////////////////////////////////////////////////////////////

        UserSettingsContainer   m_settings;
    };
}   // namespace AZ

#endif  // AZCORE_USER_SETTINGS_PROVIDER_H
#pragma once
