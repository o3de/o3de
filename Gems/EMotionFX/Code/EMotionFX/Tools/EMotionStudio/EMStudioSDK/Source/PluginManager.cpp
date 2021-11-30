/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include <AzCore/PlatformDef.h>
#include <AzCore/std/numeric.h>
#include "EMStudioManager.h"
#include <MysticQt/Source/MysticQtConfig.h>
#include "PluginManager.h"
#include "EMStudioPlugin.h"
#include "DockWidgetPlugin.h"

// include Qt related
#include <QApplication>
#include <QDir>
#include <QMainWindow>
#include <QRandomGenerator>
#include <QTime>
#include <QVariant>

#include <AzQtComponents/Utilities/RandomNumberGenerator.h>

// include MCore related
#include <MCore/Source/StringConversions.h>
#include <MCore/Source/LogManager.h>

namespace EMStudio
{
    // constructor
    PluginManager::PluginManager()
    {
        m_activePlugins.reserve(50);
        m_plugins.reserve(50);
    }


    // destructor
    PluginManager::~PluginManager()
    {
        MCore::LogInfo("Unloading plugins");
        UnloadPlugins();
    }


    // remove a given active plugin
    void PluginManager::RemoveActivePlugin(EMStudioPlugin* plugin)
    {
        PluginVector::const_iterator itPlugin = AZStd::find(m_activePlugins.begin(), m_activePlugins.end(), plugin);
        if (itPlugin == m_activePlugins.end())
        {
            MCore::LogWarning("Failed to remove plugin '%s'", plugin->GetName());
            return;
        }

        for (EMStudioPlugin* activePlugin : m_activePlugins)
        {
            activePlugin->OnBeforeRemovePlugin(plugin->GetClassID());
        }

        m_activePlugins.erase(itPlugin);
        delete plugin;
    }


    // unload the plugin libraries
    void PluginManager::UnloadPlugins()
    {
        // process any remaining events
        QApplication::processEvents();

        // delete all plugins
        for (EMStudioPlugin* plugin : m_plugins)
        {
            delete plugin;
        }
        m_plugins.clear();

        // delete all active plugins
        for (auto plugin = m_activePlugins.rbegin(); plugin != m_activePlugins.rend(); ++plugin)
        {
            for (EMStudioPlugin* pluginToNotify : m_activePlugins)
            {
                pluginToNotify->OnBeforeRemovePlugin((*plugin)->GetClassID());
            }

            delete *plugin;
            m_activePlugins.pop_back();
        }
    }


    // register the plugin
    void PluginManager::RegisterPlugin(EMStudioPlugin* plugin)
    {
        m_plugins.push_back(plugin);
    }


    // create a new active plugin from a given type
    EMStudioPlugin* PluginManager::CreateWindowOfType(const char* pluginType, const char* objectName)
    {
        // try to locate the plugin type
        const size_t pluginIndex = FindPluginByTypeString(pluginType);
        if (pluginIndex == InvalidIndex)
        {
            return nullptr;
        }

        // create the new plugin of this type
        EMStudioPlugin* newPlugin = m_plugins[ pluginIndex ]->Clone();

        // init the plugin
        newPlugin->CreateBaseInterface(objectName);

        // register as active plugin. This has to be done at this point since
        // the initialization could try to access the plugin and assume that
        // is active.
        m_activePlugins.push_back(newPlugin);

        newPlugin->Init();

        return newPlugin;
    }


    // find a given plugin by its name (type string)
    size_t PluginManager::FindPluginByTypeString(const char* pluginType) const
    {
        const auto foundPlugin = AZStd::find_if(begin(m_plugins), end(m_plugins), [pluginType](const EMStudioPlugin* plugin)
        {
            return AzFramework::StringFunc::Equal(pluginType, plugin->GetName());
        });
        return foundPlugin != end(m_plugins) ? AZStd::distance(begin(m_plugins), foundPlugin) : InvalidIndex;
    }

    EMStudioPlugin* PluginManager::GetActivePluginByTypeString(const char* pluginType) const
    {
        const auto foundPlugin = AZStd::find_if(begin(m_activePlugins), end(m_activePlugins), [pluginType](const EMStudioPlugin* plugin)
        {
            return AzFramework::StringFunc::Equal(pluginType, plugin->GetName());
        });
        return foundPlugin != end(m_activePlugins) ? *foundPlugin : nullptr;
    }

    // generate a unique object name
    QString PluginManager::GenerateObjectName() const
    {
        AZStd::string randomString;

        // random seed
        AzQtComponents::GetRandomGenerator()->seed(QTime(0, 0, 0).secsTo(QTime::currentTime()));

        // repeat until we found a free ID
        for (;; )
        {
            // generate a string from a set of random numbers
            randomString = AZStd::string::format("PLUGIN%d%d%d",
               AzQtComponents::GetRandomGenerator()->generate(),
               AzQtComponents::GetRandomGenerator()->generate(),
               AzQtComponents::GetRandomGenerator()->generate()
            );

            // check if we have a conflict with a current plugin
            const bool hasConflict = AZStd::any_of(begin(m_activePlugins), end(m_activePlugins), [&randomString](EMStudioPlugin* plugin)
            {
                return plugin->GetHasWindowWithObjectName(randomString);
            });

            if (hasConflict == false)
            {
                return randomString.c_str();
            }
        }
    }

    // find the number of active plugins of a given type
    size_t PluginManager::GetNumActivePluginsOfType(const char* pluginType) const
    {
        return AZStd::accumulate(m_activePlugins.begin(), m_activePlugins.end(), size_t{0}, [pluginType](size_t total, const EMStudioPlugin* plugin)
        {
            return total + AzFramework::StringFunc::Equal(pluginType, plugin->GetName());
        });
    }


    // find the first active plugin of a given type
    EMStudioPlugin* PluginManager::FindActivePlugin(uint32 classID) const
    {
        const auto foundPlugin = AZStd::find_if(begin(m_activePlugins), end(m_activePlugins), [classID](const EMStudioPlugin* plugin)
        {
            return plugin->GetClassID() == classID;
        });
        return foundPlugin != end(m_activePlugins) ? *foundPlugin : nullptr;
    }



    // find the number of active plugins of a given type
    size_t PluginManager::GetNumActivePluginsOfType(uint32 classID) const
    {
        return AZStd::accumulate(m_activePlugins.begin(), m_activePlugins.end(), size_t{0}, [classID](size_t total, const EMStudioPlugin* plugin)
        {
            return total + (plugin->GetClassID() == classID);
        });
    }
}   // namespace EMStudio

