/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "XmlHistoryManager.h"

////////////////////////////////////////////////////////////////////////////
SXmlHistory::SXmlHistory(CXmlHistoryManager* pManager, const XmlNodeRef& xmlBaseVersion, uint32 typeId)
    : m_pManager(pManager)
    , m_typeId(typeId)
    , m_DeletedVersion(-1)
    , m_SavedVersion(-1)
{
    int newVersionNumber = m_pManager->GetCurrentVersionNumber() + (m_pManager->IsPreparedForNextVersion() ? 1 : 0);
    m_xmlVersionList[ newVersionNumber ] = xmlBaseVersion;
}

////////////////////////////////////////////////////////////////////////////
void SXmlHistory::AddToHistory(const XmlNodeRef& newXmlVersion)
{
    int newVersionNumber = m_pManager->IncrementVersion(this);
    m_xmlVersionList[ newVersionNumber ] = newXmlVersion;
}

////////////////////////////////////////////////////////////////////////////
const XmlNodeRef& SXmlHistory::Undo(bool* bVersionExist)
{
    m_pManager->Undo();
    return GetCurrentVersion(bVersionExist);
}

////////////////////////////////////////////////////////////////////////////
const XmlNodeRef& SXmlHistory::Redo()
{
    m_pManager->Redo();
    return GetCurrentVersion();
}

////////////////////////////////////////////////////////////////////////////
const XmlNodeRef& SXmlHistory::GetCurrentVersion(bool* bVersionExist, int* iVersionNumber) const
{
    int currentVersion = m_pManager->GetCurrentVersionNumber();

    TXmlVersionMap::const_iterator it = m_xmlVersionList.begin();
    TXmlVersionMap::const_iterator previt = m_xmlVersionList.begin();
    while (it != m_xmlVersionList.end())
    {
        if (it->first > currentVersion)
        {
            break;
        }
        previt = it;
        ++it;
    }

    if (bVersionExist)
    {
        *bVersionExist = currentVersion >= m_xmlVersionList.begin()->first;
    }
    if (iVersionNumber)
    {
        *iVersionNumber = previt->first;
    }

    return previt->second;
}

////////////////////////////////////////////////////////////////////////////
bool SXmlHistory::IsModified() const
{
    int currVersion;
    GetCurrentVersion(NULL, &currVersion);
    return m_SavedVersion != currVersion;
}

////////////////////////////////////////////////////////////////////////////
void SXmlHistory::FlagAsDeleted()
{
    assert(m_pManager->IsPreparedForNextVersion());
    m_DeletedVersion = m_pManager->GetCurrentVersionNumber() + 1;
    m_SavedVersion = -1;
}

////////////////////////////////////////////////////////////////////////////
void SXmlHistory::FlagAsSaved()
{
    if (Exist())
    {
        int currVersion;
        GetCurrentVersion(NULL, &currVersion);
        m_SavedVersion = currVersion;
    }
}

////////////////////////////////////////////////////////////////////////////
bool SXmlHistory::Exist() const
{
    //  int currentVersion = m_pManager->GetCurrentVersionNumber();
    //  bool deleted = m_DeletedVersion != -1 && currentVersion >= m_DeletedVersion;
    //  bool exist = currentVersion >= m_xmlVersionList.begin()->first;
    //  return !deleted && exist;
    int currentVersion = m_pManager->GetCurrentVersionNumber();
    return currentVersion >= m_xmlVersionList.begin()->first && (m_DeletedVersion == -1 || currentVersion < m_DeletedVersion);
}

////////////////////////////////////////////////////////////////////////////
void SXmlHistory::ClearRedo()
{
    int currentVersion = m_pManager->GetCurrentVersionNumber();
    if (m_DeletedVersion > currentVersion + (m_pManager->IsPreparedForNextVersion() ? 1 : 0))
    {
        m_DeletedVersion = -1;
    }
    TXmlVersionMap::iterator it = m_xmlVersionList.begin();
    for (; it != m_xmlVersionList.end(); ++it)
    {
        if (it->first > currentVersion)
        {
            break;
        }
    }
    if (it != m_xmlVersionList.end())
    {
        ++it;
        while (it != m_xmlVersionList.end())
        {
            m_xmlVersionList.erase(it++);
        }
    }
}

////////////////////////////////////////////////////////////////////////////
void SXmlHistory::ClearHistory(bool flagAsSaved)
{
    bool wasModified = !flagAsSaved && IsModified();
    m_DeletedVersion = Exist() ? -1 : 0;
    ClearRedo();
    XmlNodeRef latest = m_xmlVersionList.rbegin()->second;
    m_xmlVersionList.clear();
    m_xmlVersionList[ 0 ] = latest;
    m_SavedVersion = wasModified ? -1 : 0;
}

////////////////////////////////////////////////////////////////////////////
SXmlHistoryGroup::SXmlHistoryGroup(CXmlHistoryManager* pManager, uint32 typeId)
    : m_pManager(pManager)
    , m_typeId(typeId)
{
}

////////////////////////////////////////////////////////////////////////////
SXmlHistory* SXmlHistoryGroup::GetHistory(int index) const
{
    THistoryList::const_iterator it = m_List.begin();
    while (index > 0 && it != m_List.end())
    {
        ++it;
        if ((*it)->Exist())
        {
            --index;
        }
    }
    return it != m_List.end() ? *it : NULL;
}

////////////////////////////////////////////////////////////////////////////
int SXmlHistoryGroup::GetHistoryCount() const
{
    int count = 0;
    for (THistoryList::const_iterator it = m_List.begin(); it != m_List.end(); ++it)
    {
        if ((*it)->Exist())
        {
            count++;
        }
    }
    return count;
}

////////////////////////////////////////////////////////////////////////////
SXmlHistory* SXmlHistoryGroup::GetHistoryByTypeId(uint32 typeId, int index /*= 0*/) const
{
    for (int i = 0; i < GetHistoryCount(); ++i)
    {
        SXmlHistory* pHistory = GetHistory(i);
        if (pHistory->GetTypeId() == typeId)
        {
            index--;
        }
        if (index == -1)
        {
            return pHistory;
        }
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////////
int SXmlHistoryGroup::GetHistoryCountByTypeId(uint32 typeId) const
{
    int res = 0;
    for (int i = 0; i < GetHistoryCount(); ++i)
    {
        if (GetHistory(i)->GetTypeId() == typeId)
        {
            res++;
        }
    }
    return res;
}

////////////////////////////////////////////////////////////////////////////
int SXmlHistoryGroup::CreateXmlHistory(uint32 typeId, const XmlNodeRef& xmlBaseVersion)
{
    if (m_pManager)
    {
        m_List.push_back(m_pManager->CreateXmlHistory(typeId, xmlBaseVersion));
        return m_List.size() - 1;
    }
    return -1;
}

////////////////////////////////////////////////////////////////////////////
int SXmlHistoryGroup::GetHistoryIndex(const SXmlHistory* pHistory) const
{
    int index = 0;
    for (THistoryList::const_iterator it = m_List.begin(); it != m_List.end(); ++it)
    {
        if (*it == pHistory)
        {
            return index;
        }
        index++;
    }
    return -1;
}

////////////////////////////////////////////////////////////////////////////
CXmlHistoryManager::CXmlHistoryManager()
    : m_CurrentVersion(0)
    , m_LatestVersion(0)
    , m_pExclusiveListener(NULL)
    , m_RecordNextVersion(false)
    , m_pExActiveGroup(NULL)
    , m_bIsActiveGroupEx(false)
{
    m_pNullGroup = new SXmlHistoryGroup(this, (uint32) - 1);
}

CXmlHistoryManager::~CXmlHistoryManager()
{
    delete m_pNullGroup;
}

/////////////////////////////////////////////////////////////////////////////
bool CXmlHistoryManager::Undo()
{
    if (m_CurrentVersion > 0)
    {
        int prevVersion = m_CurrentVersion;
        const SXmlHistoryGroup* pPrevCurrGroup = GetActiveGroup();
        m_CurrentVersion--;
        ReloadCurrentVersion(pPrevCurrGroup, prevVersion);
        return true;
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool CXmlHistoryManager::Redo()
{
    if (m_CurrentVersion < m_LatestVersion)
    {
        int prevVersion = m_CurrentVersion;
        const SXmlHistoryGroup* pPrevCurrGroup = GetActiveGroup();
        m_CurrentVersion++;
        ReloadCurrentVersion(pPrevCurrGroup, prevVersion);
        return true;
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool CXmlHistoryManager::Goto(int historyNum)
{
    if (historyNum >= 0 && historyNum <= m_LatestVersion)
    {
        int prevVersion = m_CurrentVersion;
        const SXmlHistoryGroup* pPrevCurrGroup = GetActiveGroup();
        m_CurrentVersion = historyNum;
        ReloadCurrentVersion(pPrevCurrGroup, prevVersion);
        return true;
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::RecordUndo(IXmlUndoEventHandler* pEventHandler, const char* desc)
{
    ClearRedo();
    SXmlHistory* pChangedXml = m_UndoEventHandlerMap[ pEventHandler ].CurrentData;
    assert(pChangedXml);

    XmlNodeRef newData = gEnv->pSystem->CreateXmlNode();
#if !defined(NDEBUG)
    bool bRes =
#endif
        pEventHandler->SaveToXml(newData);
    assert(bRes);

    TXmlHistotyGroupPtrList activeGroups = m_HistoryInfoMap[ m_CurrentVersion ].ActiveGroups;

    pChangedXml->AddToHistory(newData);
    m_UndoEventHandlerMap[ pEventHandler ].HistoryData[ m_CurrentVersion ] = pChangedXml;
    m_HistoryInfoMap[ m_CurrentVersion ].HistoryDescription = desc;
    m_HistoryInfoMap[ m_CurrentVersion ].IsNullUndo = false;
    m_HistoryInfoMap[ m_CurrentVersion ].ActiveGroups = activeGroups;
    m_HistoryInfoMap[ m_CurrentVersion ].HistoryInvalidated = false;
    NotifyUndoEventListener(IXmlHistoryEventListener::eHET_VersionAdded);
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::UndoEventHandlerDestroyed(IXmlUndoEventHandler* pEventHandler, uint32 typeId /*= 0*/, bool destoryForever /*= false*/)
{
    if (destoryForever)
    {
        UnregisterUndoEventHandler(pEventHandler);
    }
    else
    {
        m_InvalidHandlerMap[typeId] = pEventHandler;
    }
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::RestoreUndoEventHandler(IXmlUndoEventHandler* pEventHandler, uint32 typeId)
{
    TInvalidUndoEventListener::iterator it = m_InvalidHandlerMap.find(typeId);
    if (it != m_InvalidHandlerMap.end())
    {
        IXmlUndoEventHandler* pLastHandler = it->second;
        TUndoEventHandlerMap::iterator lastit = m_UndoEventHandlerMap.find(pLastHandler);
        if (lastit != m_UndoEventHandlerMap.end())
        {
            m_UndoEventHandlerMap[pEventHandler] = lastit->second;
            m_UndoEventHandlerMap.erase(lastit);
        }
        m_InvalidHandlerMap.erase(it);
    }
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::RegisterEventListener(IXmlHistoryEventListener* pEventListener)
{
    stl::push_back_unique(m_EventListener, pEventListener);
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::UnregisterEventListener(IXmlHistoryEventListener* pEventListener)
{
    m_EventListener.remove(pEventListener);
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::PrepareForNextVersion()
{
    assert(!m_RecordNextVersion);
    m_RecordNextVersion = true;
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::RecordNextVersion(SXmlHistory* pHistory, XmlNodeRef newData, const char* undoDesc /*= NULL*/)
{
    assert(m_RecordNextVersion);
    RegisterUndoEventHandler(this, pHistory);
    m_newHistoryData = newData;
    RecordUndo(this, undoDesc ? undoDesc : "<UNDEFINED>");
    m_RecordNextVersion = false;
    m_HistoryInfoMap[ m_CurrentVersion ].HistoryInvalidated = true;
    UnregisterUndoEventHandler(this);
    SetActiveGroupInt(GetActiveGroup());
    NotifyUndoEventListener(IXmlHistoryEventListener::eHET_HistoryInvalidate);
}

/////////////////////////////////////////////////////////////////////////////
bool CXmlHistoryManager::SaveToXml(XmlNodeRef& xmlNode)
{
    assert(m_newHistoryData);
    xmlNode = m_newHistoryData;
    return true;
}
/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::ClearHistory(bool flagAsSaved)
{
    TXmlHistotyGroupPtrList activeGropus = m_HistoryInfoMap[ m_CurrentVersion ].ActiveGroups;
    for (TXmlHistoryList::iterator it = m_XmlHistoryList.begin(); it != m_XmlHistoryList.end(); ++it)
    {
        it->ClearHistory(flagAsSaved);
    }

    SetActiveGroup(NULL);

    m_CurrentVersion = 0;
    m_LatestVersion = 0;

    m_UndoEventHandlerMap.clear();
    m_HistoryInfoMap.clear();

    m_HistoryInfoMap[ 0 ].HistoryDescription = "New History";
    m_HistoryInfoMap[ 0 ].ActiveGroups = activeGropus;
    NotifyUndoEventListener(IXmlHistoryEventListener::eHET_HistoryCleared);
}

/////////////////////////////////////////////////////////////////////////////
const string& CXmlHistoryManager::GetVersionDesc(int number) const
{
    THistoryInfoMap::const_iterator it = m_HistoryInfoMap.find(number);
    if (it != m_HistoryInfoMap.end())
    {
        return it->second.HistoryDescription;
    }

    static string undef("UNDEFINED");
    return undef;
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::RegisterView(IXmlHistoryView* pView)
{
    stl::push_back_unique(m_Views, pView);
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::UnregisterView(IXmlHistoryView* pView)
{
    m_Views.remove(pView);
}

/////////////////////////////////////////////////////////////////////////////
SXmlHistoryGroup* CXmlHistoryManager::CreateXmlGroup(uint32 typeId)
{
    m_Groups.push_back(SXmlHistoryGroup(this, typeId));
    return &(*m_Groups.rbegin());
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::AddXmlGroup(const SXmlHistoryGroup* pGroup, const char* undoDesc /*= NULL*/)
{
    RecordUndoInternal(undoDesc ? undoDesc : "New XML Group added");
    m_HistoryInfoMap[ m_CurrentVersion ].ActiveGroups.push_back(pGroup);
    NotifyUndoEventListener(IXmlHistoryEventListener::eHET_HistoryGroupAdded, (void*)pGroup);
}
/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::RemoveXmlGroup(const SXmlHistoryGroup* pGroup, const char* undoDesc /*= NULL*/)
{
    bool unload = m_HistoryInfoMap[ m_CurrentVersion ].CurrGroup == pGroup;
    RecordUndoInternal(undoDesc ? undoDesc : "XML Group deleted");
    TXmlHistotyGroupPtrList& list = m_HistoryInfoMap[ m_CurrentVersion ].ActiveGroups;
    stl::find_and_erase(list, pGroup);
    if (unload)
    {
        SetActiveGroupInt(NULL);
    }
    m_HistoryInfoMap[ m_CurrentVersion ].CurrGroup = m_pNullGroup;
    NotifyUndoEventListener(IXmlHistoryEventListener::eHET_HistoryGroupRemoved, (void*)pGroup);
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::SetActiveGroup(const SXmlHistoryGroup* pGroup, const char* displayName /*= NULL*/, const TGroupIndexMap& groupIndex /*= TGroupIndexMap()*/, bool setExternal /*= false*/)
{
    TGroupIndexMap userIndex;
    const SXmlHistoryGroup* pActiveGroup = GetActiveGroup(userIndex);
    if (pGroup != pActiveGroup || userIndex != groupIndex || setExternal)
    {
        const bool isEx = m_CurrentVersion != m_LatestVersion || setExternal;
        m_pExActiveGroup = const_cast<SXmlHistoryGroup*>(pGroup);
        m_ExCurrentIndex = groupIndex;
        m_bIsActiveGroupEx = true;
        SetActiveGroupInt(pGroup, displayName, !isEx, groupIndex);
        m_bIsActiveGroupEx = isEx;
    }
}

void CXmlHistoryManager::SetActiveGroupInt(const SXmlHistoryGroup* pGroup, const char* displayName /*= NULL*/, bool bRecordNullUndo /*= false*/, const TGroupIndexMap& groupIndex /*= TGroupIndexMap()*/)
{
    UnloadInt();

    TEventHandlerList eventHandler;
    string undoDesc;

    if (pGroup)
    {
        for (TViews::iterator it = m_Views.begin(); it != m_Views.end(); ++it)
        {
            IXmlHistoryView* pView = *it;

            std::map<uint32, int> userIndexCount;

            for (SXmlHistoryGroup::THistoryList::const_iterator history = pGroup->m_List.begin(); history != pGroup->m_List.end(); ++history)
            {
                std::map<uint32, int>::iterator it2 = userIndexCount.find((*history)->GetTypeId());
                if (it2 == userIndexCount.end())
                {
                    userIndexCount[ (*history)->GetTypeId() ] = 0;
                }
                uint32 userindex = userIndexCount[ (*history)->GetTypeId() ];
                IXmlUndoEventHandler* pEventHandler = NULL;
                TGroupIndexMap::const_iterator indexIter = groupIndex.find((*history)->GetTypeId());
                if (indexIter == groupIndex.end() || indexIter->second == userindex)
                {
                    if ((*history)->Exist())
                    {
                        if (pView->LoadXml((*history)->GetTypeId(), (*history)->GetCurrentVersion(), pEventHandler, userindex))
                        {
                            if (pEventHandler)
                            {
                                m_UndoHandlerViewMap[ pEventHandler ] = pView;
                                RegisterUndoEventHandler(pEventHandler, *history);
                                eventHandler.push_back(pEventHandler);
                            }
                        }
                    }
                }
                if ((*history)->Exist())
                {
                    userIndexCount[ (*history)->GetTypeId() ]++;
                }
            }
        }
        undoDesc.Format("Changed View to \"%s\"", displayName ? displayName : "UNDEFINED");
    }
    else
    {
        undoDesc = "Unloaded Views";
        for (TUndoEventHandlerMap::iterator it = m_UndoEventHandlerMap.begin(); it != m_UndoEventHandlerMap.end(); ++it)
        {
            eventHandler.push_back(it->first);
        }
        pGroup = m_pNullGroup;
    }

    if (bRecordNullUndo)
    {
        RecordNullUndo(eventHandler, undoDesc.c_str());
        m_HistoryInfoMap[ m_CurrentVersion ].CurrGroup = pGroup;
        m_HistoryInfoMap[ m_CurrentVersion ].CurrUserIndex = groupIndex;
    }
    NotifyUndoEventListener(IXmlHistoryEventListener::eHET_HistoryGroupChanged);
}

/////////////////////////////////////////////////////////////////////////////
const SXmlHistoryGroup* CXmlHistoryManager::GetActiveGroup() const
{
    TGroupIndexMap userIndex;
    return GetActiveGroup(userIndex);
}

/////////////////////////////////////////////////////////////////////////////
const SXmlHistoryGroup* CXmlHistoryManager::GetActiveGroup(TGroupIndexMap& currUserIndex /*out*/) const
{
    if (m_bIsActiveGroupEx)
    {
        currUserIndex = m_ExCurrentIndex;
        return m_pExActiveGroup;
    }

    int currVersion = m_CurrentVersion;
    do
    {
        THistoryInfoMap::const_iterator it = m_HistoryInfoMap.find(currVersion);
        if (it != m_HistoryInfoMap.end())
        {
            const SXmlHistoryGroup* pGroup = it->second.CurrGroup;
            if (it != m_HistoryInfoMap.end() && pGroup)
            {
                currUserIndex = it->second.CurrUserIndex;
                return pGroup == m_pNullGroup ? NULL : pGroup;
            }
        }
        currVersion--;
    } while (currVersion >= 0);
    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
SXmlHistory* CXmlHistoryManager::CreateXmlHistory(uint32 typeId, const XmlNodeRef& xmlBaseVersion)
{
    m_XmlHistoryList.push_back(SXmlHistory(this, xmlBaseVersion, typeId));
    return &(*m_XmlHistoryList.rbegin());
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::DeleteXmlHistory(const SXmlHistory* pXmlHistry)
{
    for (TXmlHistoryList::iterator it = m_XmlHistoryList.begin(); it != m_XmlHistoryList.end(); ++it)
    {
        if (&(*it) == pXmlHistry)
        {
            m_XmlHistoryList.erase(it);
            return;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::UnloadInt()
{
    for (TViews::iterator it = m_Views.begin(); it != m_Views.end(); ++it)
    {
        IXmlHistoryView* pView = *it;
        pView->UnloadXml(-1);
    }
}

/////////////////////////////////////////////////////////////////////////////
namespace
{
    template< class T >
    void ClearAfterVersion(T& container, int version)
    {
        auto foundit = container.end();
        do
        {
            auto it = container.find(version);
            if (it != container.end())
            {
                foundit = it;
                break;
            }
            version--;
        } while (version >= 0);
        if (foundit != container.end())
        {
            for (auto it = ++foundit; it != container.end(); )
            {
                container.erase(it++);
            }
        }
    }
}

void CXmlHistoryManager::ClearRedo()
{
    ClearAfterVersion(m_HistoryInfoMap, m_CurrentVersion);
    for (TUndoEventHandlerMap::iterator it = m_UndoEventHandlerMap.begin(); it != m_UndoEventHandlerMap.end(); ++it)
    {
        ClearAfterVersion(it->second.HistoryData, m_CurrentVersion);
    }
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::DeleteAll()
{
    ClearHistory();
    m_Groups.clear();
    m_UndoEventHandlerMap.clear();
    NotifyUndoEventListener(IXmlHistoryEventListener::eHET_HistoryDeleted);
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::FlagHistoryAsSaved()
{
    for (TXmlHistoryList::iterator it = m_XmlHistoryList.begin(); it != m_XmlHistoryList.end(); ++it)
    {
        it->FlagAsSaved();
    }
    NotifyUndoEventListener(IXmlHistoryEventListener::eHET_HistorySaved);
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::RecordNullUndo(const TEventHandlerList& eventHandler, const char* desc, bool isNull /*= true*/)
{
    TXmlHistotyGroupPtrList activeGroups = m_HistoryInfoMap[ m_CurrentVersion ].ActiveGroups;

    // if current version is already null undo, overwrite it instead of create a new version
    if (m_HistoryInfoMap[ m_CurrentVersion ].IsNullUndo && isNull)
    {
        assert(m_CurrentVersion);
        m_CurrentVersion--;
    }

    ClearRedo();
    IncrementVersion();

    for (TEventHandlerList::const_iterator it = eventHandler.begin(); it != eventHandler.end(); ++it)
    {
        SXmlHistory* pChangedXml = m_UndoEventHandlerMap[ *it ].CurrentData;
        m_UndoEventHandlerMap[ *it ].HistoryData[ m_CurrentVersion ] = pChangedXml;
    }
    m_HistoryInfoMap[ m_CurrentVersion ].HistoryDescription = desc;
    m_HistoryInfoMap[ m_CurrentVersion ].IsNullUndo = isNull;
    m_HistoryInfoMap[ m_CurrentVersion ].ActiveGroups = activeGroups;
    m_HistoryInfoMap[ m_CurrentVersion ].HistoryInvalidated = false;
    NotifyUndoEventListener(IXmlHistoryEventListener::eHET_VersionAdded);
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::ReloadCurrentVersion(const SXmlHistoryGroup* pPrevGroup, int prevVersion)
{
    m_bIsActiveGroupEx = false;
    const SXmlHistoryGroup* pActiveGroup = GetActiveGroup();

    bool bInvalidated = false;
    int start = min(prevVersion, m_CurrentVersion);
    int end = max(prevVersion, m_CurrentVersion);
    for (int i = start; i <= end; ++i)
    {
        if (m_HistoryInfoMap[i].HistoryInvalidated)
        {
            bInvalidated = true;
            break;
        }
    }

    if (!bInvalidated && pPrevGroup == pActiveGroup)
    {
        for (TUndoEventHandlerMap::iterator it = m_UndoEventHandlerMap.begin(); it != m_UndoEventHandlerMap.end(); ++it)
        {
            IXmlUndoEventHandler* pEventHandler = it->first;
            if (!IsEventHandlerValid(pEventHandler))
            {
                assert(false);
                continue;
            }
            SXmlHistory* pXmlHistory = GetLatestHistory(m_UndoEventHandlerMap[ pEventHandler ]);  /*m_UndoEventHandlerMap[ pEventHandler ].HistoryData[ m_CurrentVersion ];*/
            if (pXmlHistory)
            {
                if (pXmlHistory->Exist())
                {
                    pEventHandler->ReloadFromXml(pXmlHistory->GetCurrentVersion());
                }
            }
        }
        NotifyUndoEventListener(IXmlHistoryEventListener::eHET_VersionChanged);
    }
    else
    {
        SetActiveGroupInt(pActiveGroup);
    }

    if (bInvalidated)
    {
        NotifyUndoEventListener(IXmlHistoryEventListener::eHET_HistoryInvalidate);
    }

    TXmlHistotyGroupPtrList oldActiveGroups = m_HistoryInfoMap[ prevVersion ].ActiveGroups;
    TXmlHistotyGroupPtrList newActiveGroups = m_HistoryInfoMap[ m_CurrentVersion ].ActiveGroups;

    RemoveListFromList(newActiveGroups, oldActiveGroups);
    RemoveListFromList(oldActiveGroups, m_HistoryInfoMap[ m_CurrentVersion ].ActiveGroups);

    for (TXmlHistotyGroupPtrList::iterator it = newActiveGroups.begin(); it != newActiveGroups.end(); ++it)
    {
        NotifyUndoEventListener(IXmlHistoryEventListener::eHET_HistoryGroupAdded, (void*) *it);
    }

    for (TXmlHistotyGroupPtrList::iterator it = oldActiveGroups.begin(); it != oldActiveGroups.end(); ++it)
    {
        NotifyUndoEventListener(IXmlHistoryEventListener::eHET_HistoryGroupRemoved, (void*) *it);
    }
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::RemoveListFromList(TXmlHistotyGroupPtrList& list, const TXmlHistotyGroupPtrList& removeList)
{
    for (TXmlHistotyGroupPtrList::const_iterator rit = removeList.begin(); rit != removeList.end(); ++rit)
    {
        for (TXmlHistotyGroupPtrList::iterator it = list.begin(); it != list.end(); ++it)
        {
            if (*it == *rit)
            {
                list.erase(it);
                break;
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
SXmlHistory* CXmlHistoryManager::GetLatestHistory(SUndoEventHandlerData& eventHandlerData)
{
    int currVersion = m_CurrentVersion;
    do
    {
        THistoryVersionMap::iterator it = eventHandlerData.HistoryData.find(currVersion);
        if (it != eventHandlerData.HistoryData.end())
        {
            return it->second;
        }
        currVersion--;
    } while (currVersion >= 0);
    return NULL;
}


/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::NotifyUndoEventListener(IXmlHistoryEventListener::EHistoryEventType event, void* pData)
{
    if (m_pExclusiveListener)
    {
        m_pExclusiveListener->OnEvent(event, pData);
    }
    else
    {
        for (TEventListener::iterator it = m_EventListener.begin(); it != m_EventListener.end(); ++it)
        {
            (*it)->OnEvent(event, pData);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
int CXmlHistoryManager::IncrementVersion([[maybe_unused]] SXmlHistory* pXmlHistry)
{
    for (TXmlHistoryList::iterator it = m_XmlHistoryList.begin(); it != m_XmlHistoryList.end(); ++it)
    {
        it->ClearRedo();
    }
    return IncrementVersion();
}

/////////////////////////////////////////////////////////////////////////////
int CXmlHistoryManager::IncrementVersion()
{
    m_CurrentVersion++;
    m_LatestVersion = m_CurrentVersion;
    return m_CurrentVersion;
}

////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::RegisterUndoEventHandler(IXmlUndoEventHandler* pEventHandler, SXmlHistory* pXmlData)
{
    m_UndoEventHandlerMap[ pEventHandler ].CurrentData = pXmlData;
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::UnregisterUndoEventHandler(IXmlUndoEventHandler* pEventHandler)
{
    TUndoEventHandlerMap::iterator it = m_UndoEventHandlerMap.find(pEventHandler);
    if (it != m_UndoEventHandlerMap.end())
    {
        m_UndoEventHandlerMap.erase(it);
    }
}

/////////////////////////////////////////////////////////////////////////////
void CXmlHistoryManager::RecordUndoInternal(const char* desc)
{
    TEventHandlerList eventHandler;
    for (TUndoEventHandlerMap::iterator it = m_UndoEventHandlerMap.begin(); it != m_UndoEventHandlerMap.end(); ++it)
    {
        eventHandler.push_back(it->first);
    }
    RecordNullUndo(eventHandler, desc, false);
}

/////////////////////////////////////////////////////////////////////////////
bool CXmlHistoryManager::IsEventHandlerValid(IXmlUndoEventHandler* pEventHandler)
{
    for (TInvalidUndoEventListener::iterator it = m_InvalidHandlerMap.begin(); it != m_InvalidHandlerMap.end(); ++it)
    {
        if (it->second == pEventHandler)
        {
            return false;
        }
    }
    return true;
}
