/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_UTIL_IXMLHISTORYMANAGER_H
#define CRYINCLUDE_EDITOR_UTIL_IXMLHISTORYMANAGER_H
#pragma once


// Helper class to handle Redo/Undo on set of Xml nodes
struct IXmlUndoEventHandler
{
    virtual bool SaveToXml(XmlNodeRef& xmlNode) = 0;
    virtual bool LoadFromXml(const XmlNodeRef& xmlNode) = 0;
    virtual bool ReloadFromXml(const XmlNodeRef& xmlNode) = 0;
};

struct IXmlHistoryEventListener
{
    enum EHistoryEventType
    {
        eHET_HistoryDeleted,
        eHET_HistoryCleared,
        eHET_HistorySaved,

        eHET_VersionChanged,
        eHET_VersionAdded,

        eHET_HistoryInvalidate,

        eHET_HistoryGroupChanged,
        eHET_HistoryGroupAdded,
        eHET_HistoryGroupRemoved,
    };
    virtual void OnEvent(EHistoryEventType event, void* pData = NULL) = 0;
};

struct IXmlHistoryView
{
    virtual bool LoadXml(int typeId, const XmlNodeRef& xmlNode, IXmlUndoEventHandler*& pUndoEventHandler, uint32 userindex) = 0;
    virtual void UnloadXml(int typeId) = 0;
};

struct IXmlHistoryManager
{
    // Undo/Redo
    virtual bool Undo() = 0;
    virtual bool Redo() = 0;
    virtual bool Goto(int historyNum) = 0;
    virtual void RecordUndo(IXmlUndoEventHandler* pEventHandler, const char* desc) = 0;
    virtual void UndoEventHandlerDestroyed(IXmlUndoEventHandler* pEventHandler, uint32 typeId = 0, bool destoryForever = false) = 0;
    virtual void RestoreUndoEventHandler(IXmlUndoEventHandler* pEventHandler, uint32 typeId) = 0;

    virtual void RegisterEventListener(IXmlHistoryEventListener* pEventListener) = 0;
    virtual void UnregisterEventListener(IXmlHistoryEventListener* pEventListener) = 0;

    // History
    virtual void ClearHistory(bool flagAsSaved = false) = 0;
    virtual int GetVersionCount() const = 0;
    virtual const string& GetVersionDesc(int number) const = 0;
    virtual int GetCurrentVersionNumber() const = 0;

    // Views
    virtual void RegisterView(IXmlHistoryView* pView) = 0;
    virtual void UnregisterView(IXmlHistoryView* pView) = 0;
};

#endif // CRYINCLUDE_EDITOR_UTIL_IXMLHISTORYMANAGER_H
