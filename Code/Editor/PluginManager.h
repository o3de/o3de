/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once
#ifndef CRYINCLUDE_EDITOR_PLUGINMANAGER_H
#define CRYINCLUDE_EDITOR_PLUGINMANAGER_H

class QLibrary;
struct IPlugin;

struct SPluginEntry
{
    QLibrary *hLibrary = nullptr;
    IPlugin* pPlugin = nullptr;
};

typedef std::list<SPluginEntry> TPluginList;

// Event IDs associated with event handlers
typedef std::map<int, IUIEvent*> TEventHandlerMap;
typedef TEventHandlerMap::iterator TEventHandlerIt;

// Plugins associated with ID / handler maps
typedef std::map<IPlugin*, TEventHandlerMap> TPluginEventMap;
typedef TPluginEventMap::iterator TPluginEventIt;

// UI IDs associated with plugin pointers. When a plugin UI element is
// activated, the ID is used to determine which plugin should handle
// the event
typedef std::map<uint8, IPlugin*> TUIIDPluginMap;
typedef TUIIDPluginMap::iterator TUIIDPluginIt;

class SANDBOX_API CPluginManager
{
public:
    CPluginManager();
    virtual ~CPluginManager();

    bool LoadPlugins(const char* pluginsPath);

    // release all plugins (ie, call Release() on them) - but don't drop their DLL
    void ReleaseAllPlugins();

    // actually drop their DLL.
    void UnloadAllPlugins();

    bool CanAllPluginsExitNow();
    void AddHandlerForCmdID(IPlugin* pPlugin, uint8 aCmdID, IUIEvent* pEvent);
    void NotifyPlugins(EEditorNotifyEvent aEventId);

    IPlugin* GetPluginByGUID(const char* pGUID);
    IPlugin* GetPluginByUIID(uint8 iUserInterfaceID);
    IUIEvent* GetEventByIDAndPluginID(uint8 iPluginID, uint8 iEvtID);
    void RegisterPlugin(QLibrary* dllHandle, IPlugin* pPlugin);
    const TPluginList& GetPluginList() const
    {
        return m_plugins;
    }

protected:

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    TPluginList m_plugins;
    TPluginEventMap m_pluginEventMap;
    TUIIDPluginMap m_uuidPluginMap;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    int m_currentUUID;
};
#endif // CRYINCLUDE_EDITOR_PLUGINMANAGER_H
