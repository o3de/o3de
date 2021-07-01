/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_USER_SETTINGS_H
#define AZCORE_USER_SETTINGS_H

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/Memory.h>

#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
    class SerializeContext;

    /**
     * Base class for all user settings, user settings are data
     * data is stored locally to the machine. It should contain
     * only locally stored data.
     * Important: All user settings are owned by the UserSettings
     * components. Creation and Destruction should be view aznew/delete.
     * Because the component will assume we can just delete the object. (we can change that if needed)
     */
    class UserSettings
    {
        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;

    public:
        enum ContainerType
        {
            CT_LOCAL,
            CT_GLOBAL,

            CT_MAX,
        };

        AZ_RTTI(AZ::UserSettings, "{CCA8EFE4-C4D6-49AD-80A1-70700118A9ED}");

        UserSettings()
            : m_refCount(0)  {}
        virtual ~UserSettings() {}

        /**
         * Create or find a user data with the specific ID and serialization type.
         * Uses UserSettingsBus
         * \param id - user data instance id
         * \param typeId - UUID of the data type, registered with the SerializeContext. We can't manage (load/save)
         * user settings if they are NOT stored in the system. You should use RTTI AZ::Internal::TypeToUUID<T>::s_uuid to get UUID or use
         * templated function.
         * \returns pointer to UserSettings. This function should succeed when there is a handler and stored user data (if already added)
         * can be casted to T* (using azdynamic_cast<T*>), otherwise the result is NULL.
         * You don't need to worry about the deletion/memory release as the UserSettingsHandler (component) owns the data.
         */
        template<typename T>
        static AZStd::intrusive_ptr<T>   CreateFind(u32 settingId, u32 providerId);

        template<typename T>
        static AZStd::intrusive_ptr<T>   Find(u32 settingId, u32 providerId);

        template<typename T>
        static void Release(AZStd::intrusive_ptr<T>& userSetting);

        static bool Save(u32 providerId, const char* settingsPath, SerializeContext* sc);

        static void Reflect(ReflectContext* context);

    protected:
        void    add_ref();
        void    release();

        size_t  m_refCount;
    };

    /**
     * Implemented by the application.
     * The user settings loader queries this interface
     * for to resolve the file path for each user settings container
     */
    class UserSettingsFileLocator
        : public EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits settings
        // Do we need more than one user data management system ???
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual ~UserSettingsFileLocator()  {}

        virtual AZStd::string ResolveFilePath(u32 providerId) = 0;
    };

    typedef EBus<UserSettingsFileLocator> UserSettingsFileLocatorBus;

    namespace UserSettingsInternal
    {
        void                                AddSettings(UserSettings* settings, u32 settingsId, u32 groupId);
        AZStd::intrusive_ptr<UserSettings>  FindSettings(u32 settingsId, u32 groupId);
    }   // namespace UserSettingsInternal

    //=========================================================================
    // CreateFind
    // [11/5/2012]
    //=========================================================================
    template<typename T>
    AZStd::intrusive_ptr<T> UserSettings::CreateFind(u32 settingId, u32 providerId)
    {
        AZStd::intrusive_ptr<UserSettings> settings = UserSettingsInternal::FindSettings(settingId, providerId);
        if (settings)
        {
            T* ptr = azdynamic_cast<T*>(settings.get());
            AZ_Assert(ptr, "Failed to cast UserSettings pointer to the desired type. Make sure that your type has the proper AZ_RTTI information!");
            return AZStd::intrusive_ptr<T>(ptr);
        }

        AZStd::intrusive_ptr<T> newSettings = aznew T;
        UserSettingsInternal::AddSettings(newSettings.get(), settingId, providerId);
        return newSettings;
    }

    //=========================================================================
    // Find
    // [11/5/2012]
    //=========================================================================
    template<typename T>
    AZStd::intrusive_ptr<T> UserSettings::Find(u32 settingId, u32 providerId)
    {
        AZStd::intrusive_ptr<UserSettings> settings = UserSettingsInternal::FindSettings(settingId, providerId);
        if (settings)
        {
            T* ptr = azdynamic_cast<T*>(settings.get());
            AZ_Assert(ptr, "Failed to cast UserSettings pointer to the desired type. Make sure that your type has the proper AZ_RTTI information!");
            return ptr;
        }
        return nullptr;
    }

    //=========================================================================
    // Release
    // [2/20/2013]
    //=========================================================================
    template<typename T>
    void UserSettings::Release(AZStd::intrusive_ptr<T>& ptr)
    {
        ptr = nullptr;
    }
}   // namespace AZ

#endif  // AZCORE_USER_SETTINGS_H
#pragma once
