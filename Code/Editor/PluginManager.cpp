/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "PluginManager.h"

// Qt
#include <QLibrary>

// Editor
#include "Include/IPlugin.h"

using TPfnCreatePluginInstance = IPlugin* (*)(PLUGIN_INIT_PARAM* pInitParam);
using TPfnQueryPluginSettings = void (*)(SPluginSettings&);

CPluginManager::CPluginManager()
{
    m_currentUUID = 0;
}

CPluginManager::~CPluginManager()
{
    ReleaseAllPlugins();
    UnloadAllPlugins();
}


void CPluginManager::ReleaseAllPlugins()
{
    CLogFile::WriteLine("[Plugin Manager] Releasing all previous plugins");

    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it)
    {
        if (it->pPlugin)
        {
            it->pPlugin->Release();
            it->pPlugin = nullptr;
        }
    }

    m_pluginEventMap.clear();
    m_uuidPluginMap.clear();
}

void CPluginManager::UnloadAllPlugins()
{
    CLogFile::WriteLine("[Plugin Manager] Unloading all previous plugins");

    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it)
    {
        if (it->pPlugin)
        {
            it->pPlugin->Release();
            it->pPlugin = nullptr;
        }

        if (it->hLibrary)
        {
            it->hLibrary->unload();
            delete it->hLibrary;
        }
    }

    m_plugins.clear();
    m_pluginEventMap.clear();
    m_uuidPluginMap.clear();
}


namespace
{
    IPlugin* SafeCallFactory(
        TPfnCreatePluginInstance pfnFactory,
        PLUGIN_INIT_PARAM* pInitParam,
        const char* szFilePath)
    {
        IPlugin* pIPlugin = pfnFactory(pInitParam);
        if (!pIPlugin)
        {
            if (AZ::Debug::Trace::Instance().IsDebuggerPresent())
            {
                AZ::Debug::Trace::Instance().Break();
            }
            CLogFile::FormatLine("Can't initialize plugin '%s'! Possible binary version incompatibility. Please reinstall this plugin.", szFilePath);
        }

        return pIPlugin;
    }
}

namespace
{
    struct SPlugin
    {
        QString m_path;
        QString m_name;
    };

    // This does a topological sort on the plugin list. It will also remove plugins that have
    // missing dependencies or there is a cycle in the dependency tree.
    void SortPluginsByDependency(std::list<SPlugin>& plugins)
    {
        std::list<SPlugin> finalList;
        std::set<QString, stl::less_stricmp<QString> > loadedPlugins;

        while (!plugins.empty())
        {
            for (auto iter = plugins.begin(); iter != plugins.end(); )
            {
                finalList.push_back(*iter);
                loadedPlugins.insert(iter->m_name);
                iter = plugins.erase(iter);
            }
        }
        plugins = finalList;
    }
}

bool CPluginManager::LoadPlugins(const char* pluginsPath)
{
    QString strPath{ pluginsPath }; 

    CLogFile::WriteLine("[Plugin Manager] Loading plugins...");

    if (!QFileInfo::exists(strPath))
    {
        CLogFile::FormatLine("[Plugin Manager] Cannot find plugin directory '%s'", strPath.toUtf8().data());
        return false;
    }

    std::list<SPlugin> plugins;
    {
        // LY_EDITOR_PLUGINS is defined by the CMakeLists.txt. The editor plugins add themselves to a variable that
        // the editor uses to pass it to the build. Once a plugin is deleted, it will stop being in such variable producing
        // the editor to not load that plugin anymore, even if it is in the output folder.
#if defined(LY_EDITOR_PLUGINS)
        QDir qPath(strPath);
        AZStd::vector<AZStd::string> tokens;
        AZ::StringFunc::Tokenize(AZStd::string_view(LY_EDITOR_PLUGINS), tokens, ',');
        for (const AZStd::string& token : tokens)
        {
            SPlugin plugin;
            plugin.m_name = QString(token.c_str());
            plugin.m_path = qPath.absoluteFilePath(plugin.m_name);
            plugins.push_back(plugin);
        }
#endif
    }
    if (plugins.empty())
    {
        CLogFile::FormatLine("[Plugin Manager] Cannot find any plugins in plugin directory '%s'", strPath.toUtf8().data());
        return false;
    }

    // Sort plugins by dependency
    SortPluginsByDependency(plugins);

    for (auto iter = plugins.begin(); iter != plugins.end(); ++iter)
    {
        // Load the plugin's DLL
        QLibrary *hPlugin = new QLibrary(iter->m_path);
        hPlugin->setLoadHints(QLibrary::DeepBindHint);

        AZStd::string pathUtf8(iter->m_path.toUtf8().constData());

        if (!hPlugin->load())
        {
            AZStd::string errorMsg(hPlugin->errorString().toUtf8().constData());
            CLogFile::FormatLine("[Plugin Manager] Can't load plugin DLL '%s' message '%s' !", pathUtf8.c_str(), errorMsg.c_str());
            delete hPlugin;
            continue;
        }

        // Open 3D Engine:
        // Query the plugin settings, check for manual load...
        TPfnQueryPluginSettings pfnQuerySettings = reinterpret_cast<TPfnQueryPluginSettings>(hPlugin->resolve("QueryPluginSettings"));

        if (pfnQuerySettings != nullptr)
        {
            SPluginSettings settings {
                0
            };
            pfnQuerySettings(settings);
            if (!settings.autoLoad)
            {
                CLogFile::FormatLine("[Plugin Manager] Skipping plugin DLL '%s' because it is marked as non-autoLoad!", pathUtf8.c_str());
                hPlugin->unload();
                delete hPlugin;
                continue;
            }
        }

        // Query the factory pointer
        TPfnCreatePluginInstance pfnFactory = reinterpret_cast<TPfnCreatePluginInstance>(hPlugin->resolve("CreatePluginInstance"));

        if (!pfnFactory)
        {
            CLogFile::FormatLine("[Plugin Manager] Cannot query plugin DLL '%s' factory pointer (is it a Sandbox plugin?)", pathUtf8.c_str());
            hPlugin->unload();
            delete hPlugin;
            continue;
        }

        IPlugin* pPlugin = nullptr;
        PLUGIN_INIT_PARAM sInitParam =
        {
            GetIEditor(),
            SANDBOX_PLUGIN_SYSTEM_VERSION,
            IPlugin::eError_None
        };

        // Create an instance of the plugin
        pPlugin = SafeCallFactory(pfnFactory, &sInitParam, iter->m_path.toUtf8().data());
        if (!pPlugin)
        {
            CLogFile::FormatLine("[Plugin Manager] Cannot initialize plugin '%s'! Possible binary version incompatibility. Please reinstall this plugin.", pathUtf8.c_str());
            assert(pPlugin);
            hPlugin->unload();
            delete hPlugin;
            continue;
        }

        if (!pPlugin)
        {
            if (sInitParam.outErrorCode == IPlugin::eError_VersionMismatch)
            {
                CLogFile::FormatLine("[Plugin Manager] Cannot create instance of plugin DLL '%s'! Version mismatch. Please update the plugin.", pathUtf8.c_str());
            }
            else
            {
                CLogFile::FormatLine("[Plugin Manager] Cannot create instance of plugin DLL '%s'! Error code %u.", pathUtf8.c_str(), sInitParam.outErrorCode);
            }

            hPlugin->unload();
            delete hPlugin;

            continue;
        }

        RegisterPlugin(hPlugin, pPlugin);

        // Write log string about plugin
        CLogFile::FormatLine("[Plugin Manager] Successfully loaded plugin '%s', version '%i' (GUID: %s)",
            pPlugin->GetPluginName(), pPlugin->GetPluginVersion(), pPlugin->GetPluginGUID());
    }

    return true;
}

void CPluginManager::RegisterPlugin(QLibrary* dllHandle, IPlugin* pPlugin)
{
    SPluginEntry entry;

    entry.hLibrary = dllHandle;
    entry.pPlugin = pPlugin;
    m_plugins.push_back(entry);
    m_uuidPluginMap[static_cast<unsigned char>(m_currentUUID)] = pPlugin;
    ++m_currentUUID;
}

IPlugin* CPluginManager::GetPluginByGUID(const char* pGUID)
{
    for (const SPluginEntry& pluginEntry : m_plugins)
    {
        const char* pPluginGuid = pluginEntry.pPlugin->GetPluginGUID();

        if (pPluginGuid && !strcmp(pPluginGuid, pGUID))
        {
            return pluginEntry.pPlugin;
        }
    }

    return nullptr;
}

IPlugin* CPluginManager::GetPluginByUIID(uint8 iUserInterfaceID)
{
    TUIIDPluginIt it;

    it = m_uuidPluginMap.find(iUserInterfaceID);

    if (it == m_uuidPluginMap.end())
    {
        return nullptr;
    }

    return (*it).second;
}

IUIEvent* CPluginManager::GetEventByIDAndPluginID(uint8 aPluginID, uint8 aEventID)
{
    // Return the event interface of a user interface element which is
    // specified by its ID and the user interface ID of the plugin which
    // created the UI element

    IPlugin* pPlugin = nullptr;
    TEventHandlerIt eventIt;
    TPluginEventIt pluginIt;

    pPlugin = GetPluginByUIID(aPluginID);

    if (!pPlugin)
    {
        return nullptr;
    }

    pluginIt = m_pluginEventMap.find(pPlugin);

    if (pluginIt == m_pluginEventMap.end())
    {
        return nullptr;
    }

    eventIt = (*pluginIt).second.find(aEventID);

    if (eventIt == (*pluginIt).second.end())
    {
        return nullptr;
    }

    return (*eventIt).second;
}

bool CPluginManager::CanAllPluginsExitNow()
{
    for (const SPluginEntry& pluginEntry : m_plugins)
    {
        if (pluginEntry.pPlugin && !pluginEntry.pPlugin->CanExitNow())
        {
            return false;
        }
    }

    return true;
}

void CPluginManager::AddHandlerForCmdID(IPlugin* pPlugin, uint8 aCmdID, IUIEvent* pEvent)
{
    m_pluginEventMap[pPlugin][aCmdID] = pEvent;
}

void CPluginManager::NotifyPlugins(EEditorNotifyEvent aEventId)
{
    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it)
    {
        if (it->pPlugin)
        {
            it->pPlugin->OnEditorNotify(aEventId);
        }
    }
}
