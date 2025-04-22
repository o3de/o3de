/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/RTTI/ReflectContext.h>

namespace AZ
{
    namespace UserSettingsInternal
    {
        //-------------------------------------------------------------------------
        // AddSettings
        //-------------------------------------------------------------------------
        void AddSettings(UserSettings* settings, u32 settingsId, u32 groupId)
        {
            AZ_Warning("UserSettings", UserSettingsBus::GetNumOfEventHandlers(groupId) > 0, "There is no UserSettings handler for group %u, settings %u will not be tracked!", groupId, settingsId);
            UserSettingsBus::Event(groupId, &UserSettingsBus::Events::AddUserSettings, settingsId, settings);
        }

        //-------------------------------------------------------------------------
        // FindSettings
        //-------------------------------------------------------------------------
        AZStd::intrusive_ptr<UserSettings> FindSettings(u32 settingsId, u32 groupId)
        {
            // if we don't have any handlers exit
            AZStd::intrusive_ptr<UserSettings> ret;
            UserSettingsBus::EventResult(ret, groupId, &UserSettingsBus::Events::FindUserSettings, settingsId);
            return ret;
        }
    }   // namespace UserSettingsInternal

    //-------------------------------------------------------------------------
    // UserSettings
    //-------------------------------------------------------------------------
    bool UserSettings::Save(u32 providerId, const char* settingsPath, SerializeContext* sc)
    {
        bool settingsSaved = false;
        UserSettingsBus::EventResult(settingsSaved, providerId, &UserSettingsBus::Events::Save, settingsPath, sc);
        return settingsSaved;
    }
    //-------------------------------------------------------------------------
    void UserSettings::add_ref()
    {
        ++m_refCount;
    }
    //-------------------------------------------------------------------------
    void UserSettings::release()
    {
        AZ_Assert(m_refCount > 0, "m_refCount is already 0!");
        if (--m_refCount == 0)
        {
            delete this;
        }
    }

    void UserSettings::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<UserSettings>()
                ;
        }
    }

    //-------------------------------------------------------------------------
}   // namespace AZ
