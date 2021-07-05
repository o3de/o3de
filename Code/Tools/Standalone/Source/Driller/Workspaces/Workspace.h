/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/UserSettings/UserSettings.h>

namespace Driller
{
    /**
     * The purpose of the WorkspaceSettingsProvider is to save and restore a workspace.
     * the workspace can then be used to restore a view of data at a later time, given a file containing that workspace data.
     * It is generally overlayed on top of the data it is replacing.
     */
    class WorkspaceSettingsProvider
    {
    public:
        AZ_RTTI(WorkspaceSettingsProvider, "{E0BFC3FF-B040-49C3-B618-F2C1B7D45230}");
        AZ_CLASS_ALLOCATOR(WorkspaceSettingsProvider, AZ::SystemAllocator, 0);

        /// The main means of interaction:  Creating from a given file name (full path) and reading it back from it.
        /// Could return NULL if it fails
        static WorkspaceSettingsProvider* CreateFromFile(const AZStd::string& filename);

        /// Will return false if it fails.
        bool WriteToFile(const AZStd::string& filename);

        /// Convenience function - casts what it finds to T.  returns null if it cannot find that setting.
        template<typename T>
        T* FindSetting(AZ::u32 key)
        {
            auto it = m_WorkspaceSaveData.find(key);
            if (it == m_WorkspaceSaveData.end())
            {
                return NULL;
            }

            return AZ::RttiCast<T*>(it->second);
        }

        /// Convenience function - creates a new T, will always succeed unless critical out of memory error or exception.
        template<typename T>
        T* CreateSetting(AZ::u32 key)
        {
            AZ::UserSettings* oldSetting = FindSetting<T>(key);
            if (oldSetting != NULL)
            {
                AZ_WarningOnce("Driller", false, "A workspace save data is being written to a save file even though that CRC key already exists: 0x%08x - should not occur\n", key);
                m_WorkspaceSaveData.erase(key);
                delete oldSetting;
            }

            T* newSetting = aznew T();
            m_WorkspaceSaveData[key] = newSetting;
            return newSetting;
        }

        static void Reflect(AZ::ReflectContext* context);
        virtual ~WorkspaceSettingsProvider();

    protected:

        // we internally store our data in a map of CRC name to pointer.
        typedef  AZStd::unordered_map<AZ::u32, AZ::UserSettings*> SavedWorkspaceMap;
        SavedWorkspaceMap m_WorkspaceSaveData;
    };
}
