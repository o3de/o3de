/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include <AzCore/PlatformDef.h>
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
        const int32 numActivePlugins = static_cast<int32>(mActivePlugins.size());
        if (numActivePlugins > 0)
        {
            // iterate from back to front, destructing the plugins and removing them directly from the array of active plugins
            for (int32 a = numActivePlugins - 1; a >= 0; a--)
            {
                EMStudioPlugin* plugin = mActivePlugins[a];

                const int32 currentNumPlugins = static_cast<int32>(mActivePlugins.size());
                for (int32 p = 0; p < currentNumPlugins; ++p)
                {
                    mActivePlugins[p]->OnBeforeRemovePlugin(plugin->GetClassID());
                }

                mActivePlugins.erase(mActivePlugins.begin() + a);
                delete plugin;
            }

            MCORE_ASSERT(mActivePlugins.empty());
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
        const uint32 pluginIndex = FindPluginByTypeString(pluginType);
        if (pluginIndex == MCORE_INVALIDINDEX32)
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
    uint32 PluginManager::FindPluginByTypeString(const char* pluginType) const
    {
        const size_t numPlugins = mPlugins.size();
        for (size_t i = 0; i < numPlugins; ++i)
        {
            if (AzFramework::StringFunc::Equal(pluginType, mPlugins[i]->GetName()))
            {
                return static_cast<uint32>(i);
            }
        }

        return MCORE_INVALIDINDEX32;
    }

    EMStudioPlugin* PluginManager::GetActivePluginByTypeString(const char* pluginType) const
    {
        const size_t numPlugins = mActivePlugins.size();
        for (size_t i = 0; i < numPlugins; ++i)
        {
            if (AzFramework::StringFunc::Equal(pluginType, mActivePlugins[i]->GetName()))
            {
                return mActivePlugins[i];
            }
        }

        return nullptr;
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
            bool hasConflict = false;
            const size_t numActivePlugins = mActivePlugins.size();
            for (size_t i = 0; i < numActivePlugins; ++i)
            {
                EMStudioPlugin* plugin = mActivePlugins[i];

                // if the object name of a current plugin is equal to the one
                if (plugin->GetHasWindowWithObjectName(randomString))
                {
                    hasConflict = true;
                    break;
                }
            }

            if (hasConflict == false)
            {
                return randomString.c_str();
            }
        }

        //return QString("INVALID");
    }

    // find the number of active plugins of a given type
    uint32 PluginManager::GetNumActivePluginsOfType(const char* pluginType) const
    {
        uint32 total = 0;

        // check all active plugins to see if they are from the given type
        const size_t numActivePlugins = mActivePlugins.size();
        for (size_t i = 0; i < numActivePlugins; ++i)
        {
            if (AzFramework::StringFunc::Equal(pluginType, mActivePlugins[i]->GetName()))
            {
                total++;
            }
        }

        return total;
    }


    // find the first active plugin of a given type
    EMStudioPlugin* PluginManager::FindActivePlugin(uint32 classID) const
    {
        const size_t numActivePlugins = mActivePlugins.size();
        for (size_t i = 0; i < numActivePlugins; ++i)
        {
            if (mActivePlugins[i]->GetClassID() == classID)
            {
                return mActivePlugins[i];
            }
        }

        return nullptr;
    }



    // find the number of active plugins of a given type
    uint32 PluginManager::GetNumActivePluginsOfType(uint32 classID) const
    {
        uint32 total = 0;

        // check all active plugins to see if they are from the given type
        const size_t numActivePlugins = mActivePlugins.size();
        for (size_t i = 0; i < numActivePlugins; ++i)
        {
            if (mActivePlugins[i]->GetClassID() == classID)
            {
                total++;
            }
        }

        return total;
    }
}   // namespace EMStudio

