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
        mActivePlugins.reserve(50);
        mPlugins.reserve(50);
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
        PluginVector::const_iterator itPlugin = AZStd::find(mActivePlugins.begin(), mActivePlugins.end(), plugin);
        if (itPlugin == mActivePlugins.end())
        {
            MCore::LogWarning("Failed to remove plugin '%s'", plugin->GetName());
            return;
        }

        for (EMStudioPlugin* activePlugin : mActivePlugins)
        {
            activePlugin->OnBeforeRemovePlugin(plugin->GetClassID());
        }

        mActivePlugins.erase(itPlugin);
        delete plugin;
    }


    // unload the plugin libraries
    void PluginManager::UnloadPlugins()
    {
        // process any remaining events
        QApplication::processEvents();

        // delete all plugins
        for (EMStudioPlugin* plugin : mPlugins)
        {
            delete plugin;
        }
        mPlugins.clear();

        // delete all active plugins
        for (auto plugin = mActivePlugins.rbegin(); plugin != mActivePlugins.rend(); ++plugin)
        {
            for (EMStudioPlugin* pluginToNotify : mActivePlugins)
            {
                pluginToNotify->OnBeforeRemovePlugin((*plugin)->GetClassID());
            }

            delete *plugin;
            mActivePlugins.pop_back();
        }
    }


    // register the plugin
    void PluginManager::RegisterPlugin(EMStudioPlugin* plugin)
    {
        mPlugins.push_back(plugin);
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
        EMStudioPlugin* newPlugin = mPlugins[ pluginIndex ]->Clone();

        // init the plugin
        newPlugin->CreateBaseInterface(objectName);

        // register as active plugin. This has to be done at this point since
        // the initialization could try to access the plugin and assume that
        // is active.
        mActivePlugins.push_back(newPlugin);

        newPlugin->Init();

        return newPlugin;
    }


    // find a given plugin by its name (type string)
    size_t PluginManager::FindPluginByTypeString(const char* pluginType) const
    {
        const auto foundPlugin = AZStd::find_if(begin(mPlugins), end(mPlugins), [pluginType](const EMStudioPlugin* plugin)
        {
            return AzFramework::StringFunc::Equal(pluginType, plugin->GetName());
        });
        return foundPlugin != end(mPlugins) ? AZStd::distance(begin(mPlugins), foundPlugin) : InvalidIndex;
    }

    EMStudioPlugin* PluginManager::GetActivePluginByTypeString(const char* pluginType) const
    {
        const auto foundPlugin = AZStd::find_if(begin(mActivePlugins), end(mActivePlugins), [pluginType](const EMStudioPlugin* plugin)
        {
            return AzFramework::StringFunc::Equal(pluginType, plugin->GetName());
        });
        return foundPlugin != end(mActivePlugins) ? *foundPlugin : nullptr;
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
            const bool hasConflict = AZStd::any_of(begin(mActivePlugins), end(mActivePlugins), [&randomString](EMStudioPlugin* plugin)
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
        return AZStd::accumulate(mActivePlugins.begin(), mActivePlugins.end(), size_t{0}, [pluginType](size_t total, const EMStudioPlugin* plugin)
        {
            return total + AzFramework::StringFunc::Equal(pluginType, plugin->GetName());
        });
    }


    // find the first active plugin of a given type
    EMStudioPlugin* PluginManager::FindActivePlugin(uint32 classID) const
    {
        const auto foundPlugin = AZStd::find_if(begin(mActivePlugins), end(mActivePlugins), [classID](const EMStudioPlugin* plugin)
        {
            return plugin->GetClassID() == classID;
        });
        return foundPlugin != end(mActivePlugins) ? *foundPlugin : nullptr;
    }



    // find the number of active plugins of a given type
    size_t PluginManager::GetNumActivePluginsOfType(uint32 classID) const
    {
        return AZStd::accumulate(mActivePlugins.begin(), mActivePlugins.end(), size_t{0}, [classID](size_t total, const EMStudioPlugin* plugin)
        {
            return total + (plugin->GetClassID() == classID);
        });
    }
}   // namespace EMStudio

