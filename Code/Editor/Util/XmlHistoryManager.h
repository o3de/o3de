/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_UTIL_XMLHISTORYMANAGER_H
#define CRYINCLUDE_EDITOR_UTIL_XMLHISTORYMANAGER_H
#pragma once


#include "IXmlHistoryManager.h"

class CXmlHistoryManager;

struct SXmlHistory
{
public:
    SXmlHistory(CXmlHistoryManager* pManager, const XmlNodeRef& xmlBaseVersion, uint32 typeId);

    void AddToHistory(const XmlNodeRef& newXmlVersion);

    const XmlNodeRef& Undo(bool* bVersionExist = NULL);
    const XmlNodeRef& Redo();
    const XmlNodeRef& GetCurrentVersion(bool* bVersionExist = NULL, int* iVersionNumber = NULL) const;
    bool IsModified() const;
    uint32 GetTypeId() const {return m_typeId; }
    void FlagAsDeleted();
    void FlagAsSaved();
    bool Exist() const;

private:
    friend class CXmlHistoryManager;

    void ClearRedo();
    void ClearHistory(bool flagAsSaved);

private:
    CXmlHistoryManager* m_pManager;
    uint32 m_typeId;
    int m_DeletedVersion;
    int m_SavedVersion;

    typedef std::map< int, XmlNodeRef > TXmlVersionMap;
    TXmlVersionMap m_xmlVersionList;
};


struct SXmlHistoryGroup
{
    SXmlHistoryGroup(CXmlHistoryManager* pManager, uint32 typeId);

    SXmlHistory* GetHistory(int index) const;
    int GetHistoryCount() const;

    SXmlHistory* GetHistoryByTypeId(uint32 typeId, int index = 0) const;
    int GetHistoryCountByTypeId(uint32 typeId) const;

    int CreateXmlHistory(uint32 typeId, const XmlNodeRef& xmlBaseVersion);
    uint32 GetTypeId() const {return m_typeId; }

    int GetHistoryIndex(const SXmlHistory* pHistory) const;

private:
    friend class CXmlHistoryManager;
    CXmlHistoryManager* m_pManager;
    uint32 m_typeId;

    typedef std::list< SXmlHistory* > THistoryList;
    THistoryList m_List;
};

typedef std::map<uint32, int> TGroupIndexMap;


class CXmlHistoryManager
    : public IXmlHistoryManager
    , public IXmlUndoEventHandler
{
public:
    CXmlHistoryManager();
    ~CXmlHistoryManager();

    // Undo/Redo
    bool Undo();
    bool Redo();
    bool Goto(int historyNum);
    void RecordUndo(IXmlUndoEventHandler* pEventHandler, const char* desc);
    void UndoEventHandlerDestroyed(IXmlUndoEventHandler* pEventHandler, uint32 typeId = 0, bool destoryForever = false);
    void RestoreUndoEventHandler(IXmlUndoEventHandler* pEventHandler, uint32 typeId);

    void PrepareForNextVersion();
    void RecordNextVersion(SXmlHistory* pHistory, XmlNodeRef newData, const char* undoDesc = NULL);
    bool IsPreparedForNextVersion() const {return m_RecordNextVersion; }

    void RegisterEventListener(IXmlHistoryEventListener* pEventListener);
    void UnregisterEventListener(IXmlHistoryEventListener* pEventListener);
    void SetExclusiveListener(IXmlHistoryEventListener* pEventListener) { m_pExclusiveListener = pEventListener; }

    // History
    void ClearHistory(bool flagAsSaved = false);
    int GetVersionCount() const { return m_LatestVersion; }
    const string& GetVersionDesc(int number) const;
    int GetCurrentVersionNumber() const { return m_CurrentVersion; }

    // Views
    void RegisterView(IXmlHistoryView* pView);
    void UnregisterView(IXmlHistoryView* pView);

    // Xml History Groups
    SXmlHistoryGroup* CreateXmlGroup(uint32 typeId);
    void SetActiveGroup(const SXmlHistoryGroup* pGroup, const char* displayName = NULL, const TGroupIndexMap& groupIndex = TGroupIndexMap(), bool setExternal = false);
    const SXmlHistoryGroup* GetActiveGroup() const;
    const SXmlHistoryGroup* GetActiveGroup(TGroupIndexMap& currUserIndex /*out*/) const;
    void AddXmlGroup(const SXmlHistoryGroup* pGroup, const char* undoDesc = NULL);
    void RemoveXmlGroup(const SXmlHistoryGroup* pGroup, const char* undoDesc = NULL);

    void DeleteAll();

    void FlagHistoryAsSaved();

    virtual bool SaveToXml(XmlNodeRef& xmlNode);
    virtual bool LoadFromXml([[maybe_unused]] const XmlNodeRef& xmlNode) { return false; };
    virtual bool ReloadFromXml([[maybe_unused]] const XmlNodeRef& xmlNode) { return false; };

private:
    typedef std::list< SXmlHistory > TXmlHistoryList;
    TXmlHistoryList m_XmlHistoryList;
    int m_CurrentVersion;
    int m_LatestVersion;
    SXmlHistoryGroup* m_pNullGroup; // hack for unload view ...
    XmlNodeRef m_newHistoryData;
    bool m_RecordNextVersion;
    bool m_bIsActiveGroupEx;
    SXmlHistoryGroup* m_pExActiveGroup;
    TGroupIndexMap m_ExCurrentIndex;

    typedef std::list< SXmlHistoryGroup > TXmlHistotyGroupList;
    TXmlHistotyGroupList m_Groups;

    // event listener
    typedef std::list< IXmlHistoryEventListener* > TEventListener;
    TEventListener m_EventListener;
    IXmlHistoryEventListener* m_pExclusiveListener;

    // views
    typedef std::list< IXmlHistoryView* > TViews;
    TViews m_Views;

    typedef std::list< const SXmlHistoryGroup* > TXmlHistotyGroupPtrList;
    // history
    struct SHistoryInfo
    {
        SHistoryInfo()
            : IsNullUndo(false)
            , CurrGroup(NULL)
            , HistoryInvalidated(false) {}

        const SXmlHistoryGroup* CurrGroup;
        TGroupIndexMap CurrUserIndex;
        string HistoryDescription;
        bool IsNullUndo;
        bool HistoryInvalidated;
        TXmlHistotyGroupPtrList ActiveGroups;
    };
    typedef std::map< int, SHistoryInfo > THistoryInfoMap;
    THistoryInfoMap m_HistoryInfoMap;

    // undo event handler
    typedef std::map< int, SXmlHistory* > THistoryVersionMap;
    struct SUndoEventHandlerData
    {
        SUndoEventHandlerData()
            : CurrentData(NULL) {}

        SXmlHistory* CurrentData;
        THistoryVersionMap HistoryData;
    };
    typedef std::map< IXmlUndoEventHandler*, SUndoEventHandlerData > TUndoEventHandlerMap;
    TUndoEventHandlerMap m_UndoEventHandlerMap;

    typedef std::map< IXmlUndoEventHandler*, IXmlHistoryView* > TUndoHandlerViewMap;
    TUndoHandlerViewMap m_UndoHandlerViewMap;

    typedef std::map< uint32, IXmlUndoEventHandler* > TInvalidUndoEventListener;
    TInvalidUndoEventListener m_InvalidHandlerMap;

private:
    friend struct SXmlHistory;
    friend struct SXmlHistoryGroup;

    int IncrementVersion(SXmlHistory* pXmlHistry);
    int IncrementVersion();
    SXmlHistory* CreateXmlHistory(uint32 typeId, const XmlNodeRef& xmlBaseVersion);
    void DeleteXmlHistory(const SXmlHistory* pXmlHistry);

    void RegisterUndoEventHandler(IXmlUndoEventHandler* pEventHandler, SXmlHistory* pXmlData);
    void UnregisterUndoEventHandler(IXmlUndoEventHandler* pEventHandler);
    typedef std::list< IXmlUndoEventHandler* > TEventHandlerList;
    void RecordNullUndo(const TEventHandlerList& eventHandler, const char* desc, bool isNull = true);
    void ReloadCurrentVersion(const SXmlHistoryGroup* pPrevGroup, int prevVersion);
    SXmlHistory* GetLatestHistory(SUndoEventHandlerData& eventHandlerData);
    void NotifyUndoEventListener(IXmlHistoryEventListener::EHistoryEventType event, void* pData = NULL);

    void SetActiveGroupInt(const SXmlHistoryGroup* pGroup, const char* displayName = NULL, bool bRecordNullUndo = false, const TGroupIndexMap& groupIndex = TGroupIndexMap());
    void UnloadInt();
    void ClearRedo();

    void RecordUndoInternal(const char* desc);
    void RemoveListFromList(TXmlHistotyGroupPtrList& list, const TXmlHistotyGroupPtrList& removeList);

    bool IsEventHandlerValid(IXmlUndoEventHandler* pEventHandler);
};

#endif // CRYINCLUDE_EDITOR_UTIL_XMLHISTORYMANAGER_H
