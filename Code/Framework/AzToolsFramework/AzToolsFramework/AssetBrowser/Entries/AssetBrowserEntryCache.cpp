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

#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>

#include <AzCore/Module/Environment.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        using namespace AZ;
        const char* EntryCache::s_environmentVariableName = "AssetBrowserEntryCache";
        EnvironmentVariable<EntryCache*> EntryCache::g_globalInstance;

        EntryCache* EntryCache::GetInstance()
        {
            if (!g_globalInstance)
            {
                g_globalInstance = Environment::FindVariable<EntryCache*>(s_environmentVariableName);
            }

            return g_globalInstance ? (*g_globalInstance) : nullptr;
        }

        void EntryCache::CreateInstance()
        {
            if (!g_globalInstance)
            {
                g_globalInstance = Environment::CreateVariable<EntryCache*>(s_environmentVariableName);
                (*g_globalInstance) = nullptr;
            }

            AZ_Assert(!(*g_globalInstance), "You may not Create instance twice.");

            EntryCache* newInstance = aznew EntryCache();
            (*g_globalInstance) = newInstance;
        }

        void EntryCache::DestroyInstance()
        {
            AZ_Assert(g_globalInstance, "Invalid call to DestroyInstance - no instance exists.");
            AZ_Assert(*g_globalInstance, "You can only call DestroyInstance if you have called CreateInstance.");
            if (g_globalInstance)
            {
                delete *g_globalInstance;
            }
            (*g_globalInstance) = nullptr;
        }

        void EntryCache::Clear()
        {
            m_scanFolderIdMap.clear();
            m_fileIdMap.clear();
            m_sourceUuidMap.clear();
            m_sourceIdMap.clear();
            m_productAssetIdMap.clear();
            m_dirtyThumbnailsSet.clear();
            m_knownScanFolders.clear();
            m_absolutePathToFileId.clear();
        }
    }
}