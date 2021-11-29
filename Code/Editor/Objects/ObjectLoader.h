/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include "Util/GuidUtil.h"
#include "ErrorReport.h"
#include <AzCore/std/containers/set.h>

class CErrorRecord;
struct IObjectManager;

typedef std::map<GUID, GUID, guid_less_predicate> TGUIDRemap;

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
/** CObjectLoader used to load Bas Object and resolve ObjectId references while loading.
*/
class SANDBOX_API CObjectArchive
{
public:
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    XmlNodeRef node; //!< Current archive node.
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    bool bLoading;
    bool bUndo;

    CObjectArchive(IObjectManager* objMan, XmlNodeRef xmlRoot, bool loading);
    ~CObjectArchive();

    //! Resolve callback with only one parameter of CBaseObject.
    typedef AZStd::function<void(CBaseObject*)> ResolveObjRefFunctor1;

    // Return object ID remapped after loading.
    GUID ResolveID(REFGUID id);

    //! Set object resolve callback, it will be called once object with specified Id is loaded.
    void SetResolveCallback(CBaseObject* fromObject, REFGUID objectId, ResolveObjRefFunctor1 func);
    //! Resolve all object ids and call callbacks on resolved objects.
    void ResolveObjects();

    // Save object to archive.
    void SaveObject(CBaseObject* pObject);

    //! Load one object from archive.
    CBaseObject* LoadObject(const XmlNodeRef& objNode, CBaseObject* pPrevObject = nullptr);

    //! If true new loaded objects will be assigned new GUIDs.
    void MakeNewIds(bool bEnable);

    //! Remap object ids.
    void RemapID(REFGUID oldId, REFGUID newId);

private:
    struct SLoadedObjectInfo
    {
        int nSortOrder;
        _smart_ptr<CBaseObject> pObject;
        XmlNodeRef xmlNode;
        GUID newGuid;
        bool operator <(const SLoadedObjectInfo& oi) const { return nSortOrder < oi.nSortOrder;   }
    };

    IObjectManager* m_objectManager;
    struct Callback
    {
        ResolveObjRefFunctor1 func1;
        _smart_ptr<CBaseObject> fromObject;
        Callback() { func1 = 0; }
    };
    typedef std::multimap<GUID, Callback, guid_less_predicate> Callbacks;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    Callbacks m_resolveCallbacks;

    // Set of all saved objects to this archive.
    typedef AZStd::set<_smart_ptr<CBaseObject> > ObjectsSet;
    ObjectsSet m_savedObjects;

    //typedef std::multimap<int,_smart_ptr<CBaseObject> > OrderedObjects;
    //OrderedObjects m_orderedObjects;
    std::vector<SLoadedObjectInfo> m_loadedObjects;

    // Loaded objects IDs, used for remapping of GUIDs.
    TGUIDRemap m_IdRemap;

    enum EObjectLoaderFlags
    {
        eObjectLoader_MakeNewIDs = 0x0001,      // If true new loaded objects will be assigned new GUIDs.
    };
    int m_nFlags;
    IErrorReport* m_pCurrentErrorReport;

    bool m_bNeedResolveObjects;

    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};
