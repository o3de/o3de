/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#if !defined(Q_MOC_RUN)
#include "EMStudioConfig.h"
#include "EMStudioPlugin.h"
#include <AzCore/PlatformIncl.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/PersistentPlugin.h>
#endif

namespace EMStudio
{
    class EMSTUDIO_API PluginManager
    {
    public:
        typedef AZStd::vector<EMStudioPlugin*> PluginVector;
        typedef AZStd::vector<AZStd::unique_ptr<PersistentPlugin>> PersistentPluginVector;

        PluginManager() = default;
        ~PluginManager();

        // Plugin prototypes (persistent plugins are not included)
        void RegisterPlugin(EMStudioPlugin* plugin) { m_registeredPlugins.push_back(plugin); }
        size_t GetNumRegisteredPlugins() const { return m_registeredPlugins.size(); }
        EMStudioPlugin* GetRegisteredPlugin(size_t index) { return m_registeredPlugins[index]; }
        size_t FindRegisteredPluginIndex(const char* pluginType) const;
        const PluginVector& GetRegisteredPlugins() { return m_registeredPlugins; }

        // Active window plugins
        EMStudioPlugin* CreateWindowOfType(const char* pluginType, const char* objectName = nullptr);
        void RemoveActivePlugin(EMStudioPlugin* plugin);

        size_t GetNumActivePlugins() const { return m_activePlugins.size(); }
        EMStudioPlugin* GetActivePlugin(uint32 index) { return m_activePlugins[index]; }
        const PluginVector& GetActivePlugins() { return m_activePlugins; }

        EMStudioPlugin* FindActivePluginByTypeString(const char* pluginType) const;

        // Reqire that PluginType is a subclass of EMStudioPlugin
        template<class PluginType>
        typename AZStd::enable_if_t<AZStd::is_convertible_v<PluginType*, EMStudioPlugin*>, PluginType*> FindActivePlugin() const
        {
            return static_cast<PluginType*>(FindActivePlugin(PluginType::CLASS_ID));
        }
        EMStudioPlugin* FindActivePlugin(uint32 classID) const;

        size_t CalcNumActivePluginsOfType(const char* pluginType) const;
        size_t CalcNumActivePluginsOfType(uint32 classID) const;

        // Persistent plugins
        void AddPersistentPlugin(PersistentPlugin* plugin) { m_persistentPlugins.push_back(AZStd::unique_ptr<PersistentPlugin>(plugin)); }
        void RemovePersistentPlugin(PersistentPlugin* plugin);
        size_t GetNumPersistentPlugins() const { return m_persistentPlugins.size(); }
        PersistentPlugin* GetPersistentPlugin(size_t index) { return m_persistentPlugins[index].get(); }
        const PersistentPluginVector& GetPersistentPlugins() { return m_persistentPlugins; }

        QString GenerateObjectName() const;
        void RegisterDefaultPlugins();

    private:
        PluginVector m_registeredPlugins;
        PluginVector m_activePlugins;

        PersistentPluginVector m_persistentPlugins;

        void UnloadPlugins();
    };
} // namespace EMStudio
