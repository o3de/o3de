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
#include <QMainWindow>
#include <QDir>
#include <QTime>
#include <QVariant>
#include <QApplication>
#include <AzQtComponents/Utilities/RandomNumberGenerator.h>

// include MCore related
#include <MCore/Source/StringConversions.h>
#include <MCore/Source/LogManager.h>

// plugins
#include <EMotionStudio/Plugins/StandardPlugins/Source/LogWindow/LogWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/CommandBar/CommandBarPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/ActionHistory/ActionHistoryPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/Inspector/InspectorWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MorphTargetsWindow/MorphTargetsWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/SceneManager/SceneManagerPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectWidget.h>

namespace EMStudio
{
    PluginManager::~PluginManager()
    {
        UnloadPlugins();
    }

    void PluginManager::RemoveActivePlugin(EMStudioPlugin* plugin)
    {
        auto iterator = AZStd::find(m_activePlugins.begin(), m_activePlugins.end(), plugin);
        if (iterator == m_activePlugins.end())
        {
            AZ_Warning("EMotionFX", false, "Failed to remove plugin '%s'", plugin->GetName());
            return;
        }

        for (EMStudioPlugin* activePlugin : m_activePlugins)
        {
            activePlugin->OnBeforeRemovePlugin(plugin->GetClassID());
        }

        m_activePlugins.erase(iterator);
        delete plugin;
    }

    // unload the plugin libraries
    void PluginManager::UnloadPlugins()
    {
        // process any remaining events
        QApplication::processEvents();

        for (EMStudioPlugin* plugin : m_registeredPlugins)
        {
            delete plugin;
        }
        m_registeredPlugins.clear();

        // delete all active plugins back to front
        for (auto plugin = m_activePlugins.rbegin(); plugin != m_activePlugins.rend(); ++plugin)
        {
            for (EMStudioPlugin* pluginToNotify : m_activePlugins)
            {
                pluginToNotify->OnBeforeRemovePlugin((*plugin)->GetClassID());
            }

            delete *plugin;
            m_activePlugins.pop_back();
        }

        m_persistentPlugins.clear();
    }

    // create a new active plugin from a given type
    EMStudioPlugin* PluginManager::CreateWindowOfType(const char* pluginType, const char* objectName)
    {
        // try to locate the plugin type
        const size_t pluginIndex = FindRegisteredPluginIndex(pluginType);
        if (pluginIndex == InvalidIndex)
        {
            return nullptr;
        }

        // create the new plugin of this type
        EMStudioPlugin* newPlugin = m_registeredPlugins[pluginIndex]->Clone();

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
    size_t PluginManager::FindRegisteredPluginIndex(const char* pluginType) const
    {
        const auto foundPlugin = AZStd::find_if(begin(m_registeredPlugins), end(m_registeredPlugins), [pluginType](const EMStudioPlugin* plugin)
        {
            return AzFramework::StringFunc::Equal(pluginType, plugin->GetName());
        });
        return foundPlugin != end(m_registeredPlugins) ? AZStd::distance(begin(m_registeredPlugins), foundPlugin) : InvalidIndex;

    }

    EMStudioPlugin* PluginManager::FindActivePluginByTypeString(const char* pluginType) const
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
    size_t PluginManager::CalcNumActivePluginsOfType(const char* pluginType) const
    {
        return AZStd::accumulate(m_activePlugins.begin(), m_activePlugins.end(), size_t{0}, [pluginType](size_t total, const EMStudioPlugin* plugin)
        {
            return total + AzFramework::StringFunc::Equal(pluginType, plugin->GetName());
        });
    }

    void PluginManager::RemovePersistentPlugin(PersistentPlugin* plugin)
    {
        const auto iterator = AZStd::find_if(m_persistentPlugins.begin(), m_persistentPlugins.end(), [plugin](const AZStd::unique_ptr<PersistentPlugin>& currentPlugin)
            {
                return (currentPlugin.get() == plugin);
            });

        if (iterator != m_persistentPlugins.end())
        {
            m_persistentPlugins.erase(iterator);

        }
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

    size_t PluginManager::CalcNumActivePluginsOfType(uint32 classID) const
    {
        return AZStd::accumulate(m_activePlugins.begin(), m_activePlugins.end(), size_t{0}, [classID](size_t total, const EMStudioPlugin* plugin)
        {
            return total + (plugin->GetClassID() == classID);
        });
    }

    void PluginManager::RegisterDefaultPlugins()
    {
        m_registeredPlugins.reserve(32);

        RegisterPlugin(new LogWindowPlugin());
        RegisterPlugin(new CommandBarPlugin());
        RegisterPlugin(new ActionHistoryPlugin());
        RegisterPlugin(new MorphTargetsWindowPlugin());
        RegisterPlugin(new TimeViewPlugin());
        RegisterPlugin(new SceneManagerPlugin());
        RegisterPlugin(new MotionSetsWindowPlugin());
        RegisterPlugin(new AnimGraphPlugin());
        RegisterPlugin(new EMotionFX::SkeletonOutlinerPlugin());
        RegisterPlugin(new EMotionFX::SimulatedObjectWidget());
        RegisterPlugin(new InspectorWindow());

        m_activePlugins.reserve(m_registeredPlugins.size());
    }
} // namespace EMStudio
