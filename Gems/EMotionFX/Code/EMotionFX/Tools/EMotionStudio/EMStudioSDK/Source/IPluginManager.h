/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QString>

namespace EMStudio
{
    class EMStudioPlugin;
    class PersistentPlugin;
} // namespace EMStudio

namespace EMStudio
{
    class IPluginManager
    {
    public:
        AZ_RTTI(IPluginManager, "{556595c0-f830-11ec-b939-0242ac120002}");

        using PluginVector = AZStd::vector<EMStudio::EMStudioPlugin*>;
        using PersistentPluginVector = AZStd::vector<AZStd::unique_ptr<EMStudio::PersistentPlugin>>;
        
        template<class PluginType>
        typename AZStd::enable_if_t<AZStd::is_convertible_v<PluginType*, EMStudioPlugin*>, PluginType*> FindActivePlugin() const
        {
            return static_cast<PluginType*>(FindActivePlugin(PluginType::CLASS_ID));
        }

        IPluginManager() = default;
        virtual ~IPluginManager() = default;

        // Plugin prototypes (persistent plugins are not included)
        virtual void RegisterPlugin(EMStudio::EMStudioPlugin* plugin) = 0;
        virtual size_t GetNumRegisteredPlugins() const = 0;
        virtual EMStudio::EMStudioPlugin* GetRegisteredPlugin(size_t index) = 0;
        virtual size_t FindRegisteredPluginIndex(const char* pluginType) const = 0;
        virtual const PluginVector& GetRegisteredPlugins() = 0;

        // Active window plugins
        virtual EMStudioPlugin* CreateWindowOfType(const char* pluginType, const char* objectName = nullptr) = 0;
        virtual void RemoveActivePlugin(EMStudioPlugin* plugin) = 0;

        virtual size_t GetNumActivePlugins() const = 0;
        virtual EMStudioPlugin* GetActivePlugin(AZ::u32 index) = 0;
        virtual const PluginVector& GetActivePlugins() = 0;

        virtual EMStudioPlugin* FindActivePluginByTypeString(const char* pluginType) const = 0;
        virtual EMStudio::EMStudioPlugin* FindActivePlugin(AZ::u32 classID) const = 0;
        
        virtual size_t CalcNumActivePluginsOfType(const char* pluginType) const = 0;
        virtual size_t CalcNumActivePluginsOfType(AZ::u32 classID) const = 0;

        virtual void AddPersistentPlugin(PersistentPlugin* plugin) = 0;
        virtual void RemovePersistentPlugin(PersistentPlugin* plugin) = 0;
        virtual size_t GetNumPersistentPlugins() const = 0;
        virtual PersistentPlugin* GetPersistentPlugin(size_t index) = 0;
        virtual const PersistentPluginVector& GetPersistentPlugins() = 0;

        virtual QString GenerateObjectName() const = 0;
        virtual void RegisterDefaultPlugins() = 0;
    };

} // namespace EMStudio