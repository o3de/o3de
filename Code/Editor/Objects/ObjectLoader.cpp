/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "ObjectLoader.h"

// Editor
#include "Util/PakFile.h"
#include "WaitProgress.h"
#include "Include/IObjectManager.h"


//////////////////////////////////////////////////////////////////////////
// CObjectArchive Implementation.
//////////////////////////////////////////////////////////////////////////
CObjectArchive::CObjectArchive(IObjectManager* objMan, XmlNodeRef xmlRoot, bool loading)
{
    m_objectManager = objMan;
    bLoading = loading;
    bUndo = false;
    m_nFlags = 0;
    node = xmlRoot;
    m_pCurrentErrorReport = GetIEditor()->GetErrorReport();
    m_bNeedResolveObjects = false;
}

//////////////////////////////////////////////////////////////////////////
CObjectArchive::~CObjectArchive()
{
    // Always make sure objects are resolved when loading from archive.
    if (bLoading && m_bNeedResolveObjects)
    {
        ResolveObjects();
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectArchive::SetResolveCallback(CBaseObject* fromObject, REFGUID objectId, ResolveObjRefFunctor1 func)
{
    if (objectId == GUID_NULL)
    {
        func(0);
        return;
    }

    GUID guid(objectId);

    CBaseObject* pObject = m_objectManager->FindObject(guid);
    if (pObject && !(m_nFlags & eObjectLoader_MakeNewIDs))
    {
        // Object is already resolved. immediately call callback.
        func(pObject);
    }
    else
    {
        Callback cb;
        cb.fromObject = fromObject;
        cb.func1 = func;
        m_resolveCallbacks.insert(Callbacks::value_type(guid, cb));
    }
}

//////////////////////////////////////////////////////////////////////////
GUID CObjectArchive::ResolveID(REFGUID id)
{
    return stl::find_in_map(m_IdRemap, id, id);
}

//////////////////////////////////////////////////////////////////////////
void CObjectArchive::ResolveObjects()
{
    int i;

    if (!bLoading)
    {
        return;
    }

    {
        CWaitProgress wait("Loading Objects", false);
        wait.Start();

        GetIEditor()->SuspendUndo();
        //////////////////////////////////////////////////////////////////////////
        // Serialize All Objects from XML.
        //////////////////////////////////////////////////////////////////////////
        int numObj = static_cast<int>(m_loadedObjects.size());
        for (i = 0; i < numObj; i++)
        {
            wait.Step((i * 100) / numObj);

            SLoadedObjectInfo& obj = m_loadedObjects[i];
            m_pCurrentErrorReport->SetCurrentValidatorObject(obj.pObject);
            node = obj.xmlNode;

            obj.pObject->Serialize(*this);

            m_pCurrentErrorReport->SetCurrentValidatorObject(nullptr);

            // Objects can be added to the list here (from Groups).
            numObj = static_cast<int>(m_loadedObjects.size());
        }
        m_pCurrentErrorReport->SetCurrentValidatorObject(nullptr);
        //////////////////////////////////////////////////////////////////////////
        GetIEditor()->ResumeUndo();
    }

    // Sort objects by sort order.
    std::sort(m_loadedObjects.begin(), m_loadedObjects.end());

    // Then rearrange to parent-first, if same sort order.
    for (i = 0; i < m_loadedObjects.size(); i++)
    {
        if (m_loadedObjects[i].pObject->GetParent())
        {
            // Find later in array.
            for (int j = i + 1; j < m_loadedObjects.size() && m_loadedObjects[j].nSortOrder == m_loadedObjects[i].nSortOrder; j++)
            {
                if (m_loadedObjects[j].pObject == m_loadedObjects[i].pObject->GetParent())
                {
                    // Swap the objects.
                    std::swap(m_loadedObjects[i], m_loadedObjects[j]);
                    i--;
                    break;
                }
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Resolve objects GUIDs
    //////////////////////////////////////////////////////////////////////////
    for (Callbacks::iterator it = m_resolveCallbacks.begin(); it != m_resolveCallbacks.end(); it++)
    {
        Callback& cb = it->second;
        GUID objectId = ResolveID(it->first);
        CBaseObject* object = m_objectManager->FindObject(objectId);
        if (!object)
        {
            QString from;
            if (cb.fromObject)
            {
                from = cb.fromObject->GetName();
            }
            // Cannot resolve this object id.
            CErrorRecord err;
            err.error = QObject::tr("Unresolved ObjectID: %1, Referenced from Object %1").arg(GuidUtil::ToString(objectId)).arg(from);
            err.severity = CErrorRecord::ESEVERITY_ERROR;
            err.flags = CErrorRecord::FLAG_OBJECTID;
            err.pObject = cb.fromObject;
            GetIEditor()->GetErrorReport()->ReportError(err);

            //Warning( "Cannot resolve ObjectID: %s\r\nObject with this ID was not present in loaded file.\r\nFor instance Trigger referencing another object which is not loaded in Level.",GuidUtil::ToString(objectId) );
            continue;
        }
        m_pCurrentErrorReport->SetCurrentValidatorObject(object);
        // Call callback with this object.
        if (cb.func1)
        {
            (cb.func1)(object);
        }
    }
    m_resolveCallbacks.clear();
    //////////////////////////////////////////////////////////////////////////

    {
        CWaitProgress wait("Creating Objects", false);
        wait.Start();
        //////////////////////////////////////////////////////////////////////////
        // Serialize All Objects from XML.
        //////////////////////////////////////////////////////////////////////////
        int numObj = static_cast<int>(m_loadedObjects.size());
        for (i = 0; i < numObj; i++)
        {
            wait.Step((i * 100) / numObj);

            SLoadedObjectInfo& obj = m_loadedObjects[i];
            m_pCurrentErrorReport->SetCurrentValidatorObject(obj.pObject);

            obj.pObject->CreateGameObject();

            // unset the current validator object because the wait Step 
            // might generate unrelated errors
            m_pCurrentErrorReport->SetCurrentValidatorObject(nullptr);
        }
        m_pCurrentErrorReport->SetCurrentValidatorObject(nullptr);
        //////////////////////////////////////////////////////////////////////////
    }

    //////////////////////////////////////////////////////////////////////////
    // Call PostLoad on all these objects.
    //////////////////////////////////////////////////////////////////////////
    {
        int numObj = static_cast<int>(m_loadedObjects.size());
        for (i = 0; i < numObj; i++)
        {
            SLoadedObjectInfo& obj = m_loadedObjects[i];
            m_pCurrentErrorReport->SetCurrentValidatorObject(obj.pObject);
            node = obj.xmlNode;
            obj.pObject->PostLoad(*this);
        }
    }

    m_bNeedResolveObjects = false;
    m_pCurrentErrorReport->SetCurrentValidatorObject(nullptr);
}

//////////////////////////////////////////////////////////////////////////
void CObjectArchive::SaveObject(CBaseObject* pObject)
{
    if (pObject->CheckFlags(OBJFLAG_DONT_SAVE))
    {
        return;
    }

    if (m_savedObjects.find(pObject) == m_savedObjects.end())
    {
        m_savedObjects.insert(pObject);
        // If this object was not saved before.
        XmlNodeRef objNode = node->newChild("Object");
        XmlNodeRef prevRoot = node;
        node = objNode;

        pObject->Serialize(*this);
        node = prevRoot;
    }
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CObjectArchive::LoadObject(const XmlNodeRef& objNode, CBaseObject* pPrevObject)
{
    XmlNodeRef prevNode = node;
    node = objNode;
    CBaseObject* pObject;
    bool bMakeNewID = (m_nFlags & eObjectLoader_MakeNewIDs) ? true : false;

    pObject = m_objectManager->NewObject(*this, pPrevObject, bMakeNewID);
    if (pObject)
    {
        SLoadedObjectInfo obj;
        obj.nSortOrder = pObject->GetClassDesc()->GameCreationOrder();
        obj.pObject = pObject;
        obj.newGuid = pObject->GetId();
        obj.xmlNode = node;
        m_loadedObjects.push_back(obj);
        m_bNeedResolveObjects = true;
    }
    node = prevNode;
    return pObject;
}

//////////////////////////////////////////////////////////////////////////
void CObjectArchive::MakeNewIds(bool bEnable)
{
    if (bEnable)
    {
        m_nFlags |= eObjectLoader_MakeNewIDs;
    }
    else
    {
        m_nFlags &= ~(eObjectLoader_MakeNewIDs);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectArchive::RemapID(REFGUID oldId, REFGUID newId)
{
    m_IdRemap[oldId] = newId;
}
