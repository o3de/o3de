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

#ifndef __EMSTUDIO_PLUGINMANAGER_H
#define __EMSTUDIO_PLUGINMANAGER_H

#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/is_convertible.h>

#if !defined(Q_MOC_RUN)
#include "EMStudioConfig.h"
#include "EMStudioPlugin.h"
#include <AzCore/PlatformIncl.h>
#endif

namespace EMStudio
{
    /**
     *
     *
     */
    class EMSTUDIO_API PluginManager
    {
        MCORE_MEMORYOBJECTCATEGORY(PluginManager, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        typedef AZStd::vector<EMStudioPlugin*> PluginVector;

        PluginManager();
        ~PluginManager();

        void RegisterPlugin(EMStudioPlugin* plugin);
        EMStudioPlugin* CreateWindowOfType(const char* pluginType, const char* objectName = nullptr);
        uint32 FindPluginByTypeString(const char* pluginType) const;
        EMStudioPlugin* GetActivePluginByTypeString(const char* pluginType) const;

        // Reqire that PluginType is a subclass of EMStudioPlugin
        template<class PluginType>
        typename AZStd::enable_if_t<AZStd::is_convertible_v<PluginType*, EMStudioPlugin*>, PluginType*> FindActivePlugin() const
        {
            return static_cast<PluginType*>(FindActivePlugin(PluginType::CLASS_ID));
        }
        EMStudioPlugin* FindActivePlugin(uint32 classID) const;   // find first active plugin, or nullptr when not found

        MCORE_INLINE uint32 GetNumPlugins() const                           { return static_cast<uint32>(mPlugins.size()); }
        MCORE_INLINE EMStudioPlugin* GetPlugin(const uint32 index)          { return mPlugins[index]; }

        MCORE_INLINE uint32 GetNumActivePlugins() const                     { return static_cast<uint32>(mActivePlugins.size()); }
        MCORE_INLINE EMStudioPlugin* GetActivePlugin(const uint32 index)    { return mActivePlugins[index]; }
        MCORE_INLINE const PluginVector& GetActivePlugins() { return mActivePlugins; }

        uint32 GetNumActivePluginsOfType(const char* pluginType) const;
        uint32 GetNumActivePluginsOfType(uint32 classID) const;
        void RemoveActivePlugin(EMStudioPlugin* plugin);

        QString GenerateObjectName() const;

    private:
        PluginVector mPlugins;

        PluginVector mActivePlugins;

        void UnloadPlugins();
    };
}   // namespace EMStudio

#endif
