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
#include <EMotionStudio/EMStudioSDK/Source/IPluginManager.h>
#endif

namespace EMStudio
{
    class EMSTUDIO_API PluginManager final : 
        public EMStudio::IPluginManager
    {
    public:
        AZ_RTTI(PluginManager, "{5c60f9be-f835-11ec-b939-0242ac120002}", IPluginManager);

        PluginManager() = default;
        ~PluginManager() override;

        // Plugin prototypes (persistent plugins are not included)
        virtual void RegisterPlugin(EMStudioPlugin* plugin) override { m_registeredPlugins.push_back(plugin); }
        virtual size_t GetNumRegisteredPlugins() const override { return m_registeredPlugins.size(); }
        virtual EMStudioPlugin* GetRegisteredPlugin(size_t index) override { return m_registeredPlugins[index]; }
        virtual size_t FindRegisteredPluginIndex(const char* pluginType) const override;
        virtual const PluginVector& GetRegisteredPlugins() override { return m_registeredPlugins; }

        // Active window plugins
        virtual EMStudioPlugin* CreateWindowOfType(const char* pluginType, const char* objectName = nullptr) override;
        virtual void RemoveActivePlugin(EMStudioPlugin* plugin) override;

        virtual size_t GetNumActivePlugins() const override{ return m_activePlugins.size(); }
        virtual EMStudioPlugin* GetActivePlugin(uint32 index) override { return m_activePlugins[index]; }
        virtual const PluginVector& GetActivePlugins() override{ return m_activePlugins; }

        virtual EMStudioPlugin* FindActivePluginByTypeString(const char* pluginType) const override;
        virtual EMStudioPlugin* FindActivePlugin(uint32 classID) const override;

        virtual size_t CalcNumActivePluginsOfType(const char* pluginType) const override;
        virtual size_t CalcNumActivePluginsOfType(uint32 classID) const override;

        // Persistent plugins
        virtual void AddPersistentPlugin(PersistentPlugin* plugin) override { m_persistentPlugins.push_back(AZStd::unique_ptr<PersistentPlugin>(plugin)); }
        virtual void RemovePersistentPlugin(PersistentPlugin* plugin) override;
        virtual size_t GetNumPersistentPlugins() const override { return m_persistentPlugins.size(); }
        virtual PersistentPlugin* GetPersistentPlugin(size_t index) override { return m_persistentPlugins[index].get(); }
        virtual const PersistentPluginVector& GetPersistentPlugins() override { return m_persistentPlugins; }

        virtual QString GenerateObjectName() const override;
        virtual void RegisterDefaultPlugins() override;

    private:
        PluginVector m_registeredPlugins;
        PluginVector m_activePlugins;

        PersistentPluginVector m_persistentPlugins;

        void UnloadPlugins();
    };
} // namespace EMStudio
