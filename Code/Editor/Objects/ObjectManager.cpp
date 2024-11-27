/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "ObjectManager.h"

// Qt
#include <QMessageBox>

// Editor
#include "Settings.h"
#include "DisplaySettings.h"
#include "EntityObject.h"
#include "Viewport.h"
#include "GameEngine.h"
#include "WaitProgress.h"
#include "Util/Image.h"
#include "ObjectManagerLegacyUndo.h"
#include "Include/HitContext.h"

#include <AzCore/Console/Console.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>

AZ_CVAR_EXTERNED(bool, ed_visibility_logTiming);

/*!
 *  Class Description used for object templates.
 *  This description filled from Xml template files.
 */
class CXMLObjectClassDesc
    : public CObjectClassDesc
{
public:
    CObjectClassDesc*   superType;
    QString type;
    QString category;
    QString fileSpec;
    GUID guid;

public:
    virtual ~CXMLObjectClassDesc() = default;
    REFGUID ClassID() override
    {
        return guid;
    }
    ObjectType GetObjectType() override { return superType->GetObjectType(); };
    QString ClassName() override { return type; };
    QString Category() override { return category; };
    QObject* CreateQObject() const override { return superType->CreateQObject(); }
    QString GetTextureIcon() override { return superType->GetTextureIcon(); };
    QString GetFileSpec() override
    {
        if (!fileSpec.isEmpty())
        {
            return fileSpec;
        }
        else
        {
            return superType->GetFileSpec();
        }
    };
    int GameCreationOrder() override { return superType->GameCreationOrder(); };
};

//////////////////////////////////////////////////////////////////////////
// CObjectManager implementation.
//////////////////////////////////////////////////////////////////////////
CObjectManager* g_pObjectManager = nullptr;

//////////////////////////////////////////////////////////////////////////
CObjectManager::CObjectManager()
    : m_bExiting(false)
    , m_isUpdateVisibilityList(false)
    , m_currentHideCount(CBaseObject::s_invalidHiddenID)
{
    g_pObjectManager = this;

    m_objectsByName.reserve(1024);
}

//////////////////////////////////////////////////////////////////////////
CObjectManager::~CObjectManager()
{
    m_bExiting = true;
    DeleteAllObjects();
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CObjectManager::NewObject(CObjectClassDesc* cls, CBaseObject* prev, const QString& file, const char* newObjectName)
{
    // Suspend undo operations when initializing object.
    GetIEditor()->SuspendUndo();

    CBaseObjectPtr obj;
    {
        obj = qobject_cast<CBaseObject*>(cls->CreateQObject());
        obj->SetClassDesc(cls);
        obj->InitVariables();
        obj->m_guid = AZ::Uuid::CreateRandom();    // generate uniq GUID for this object.

        GetIEditor()->GetErrorReport()->SetCurrentValidatorObject(obj);
        if (obj->Init(GetIEditor(), prev, file))
        {
            if ((newObjectName)&&(newObjectName[0]))
            {
                obj->SetName(newObjectName);
            }
            else
            {
                if (obj->GetName().isEmpty())
                {
                    obj->GenerateUniqueName();
                }
            }

            // Create game object itself.
            obj->CreateGameObject();

            if (!AddObject(obj))
            {
                obj = nullptr;
            }
        }
        else
        {
            obj = nullptr;
        }
        GetIEditor()->GetErrorReport()->SetCurrentValidatorObject(nullptr);
    }

    GetIEditor()->ResumeUndo();

    if (obj != nullptr && GetIEditor()->IsUndoRecording())
    {
        // AZ entity creations are handled through the AZ undo system.
        if (obj->GetType() != OBJTYPE_AZENTITY)
        {
            GetIEditor()->RecordUndo(new CUndoBaseObjectNew(obj));
        }
    }

    return obj;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CObjectManager::NewObject(CObjectArchive& ar, CBaseObject* pUndoObject, bool bMakeNewId)
{
    XmlNodeRef objNode = ar.node;

    // Load all objects from XML.
    QString typeName;
    GUID id = GUID_NULL;

    if (!objNode->getAttr("Type", typeName))
    {
        return nullptr;
    }

    if (!objNode->getAttr("Id", id))
    {
        // Make new ID for object that doesn't have if.
        id = AZ::Uuid::CreateRandom();
    }

    if (bMakeNewId)
    {
        // Make new guid for this object.
        GUID newId = AZ::Uuid::CreateRandom();
        ar.RemapID(id, newId);  // Mark this id remapped.
        id = newId;
    }

    CBaseObjectPtr pObject;
    if (pUndoObject)
    {
        // if undoing restore object pointer.
        pObject = pUndoObject;
    }
    else
    {
        // New object creation.

        // Suspend undo operations when initializing object.
        CUndoSuspend undoSuspender;

        QString entityClass;
        if (objNode->getAttr("EntityClass", entityClass))
        {
            typeName = typeName + "::" + entityClass;
        }

        CObjectClassDesc* cls = FindClass(typeName);
        if (!cls)
        {
            CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "RuntimeClass %s not registered", typeName.toUtf8().data());
            return nullptr;
        }

        pObject = qobject_cast<CBaseObject*>(cls->CreateQObject());
        assert(pObject);
        pObject->SetClassDesc(cls);
        pObject->m_guid = id;

        pObject->InitVariables();

        QString objName;
        objNode->getAttr("Name", objName);
        pObject->m_name = objName;

        CBaseObject* obj = FindObject(pObject->GetId());
        if (obj)
        {
            // If id is taken.
            QString error;
            error = QObject::tr("[Error] Object %1 already exists in the Object Manager and has been deleted as it is a duplicate of object %2").arg(pObject->m_name, obj->GetName());
            CLogFile::WriteLine(error.toUtf8().data());

            if (!GetIEditor()->IsInTestMode() && !GetIEditor()->IsInLevelLoadTestMode())
            {
                CErrorRecord errorRecord;
                errorRecord.pObject = obj;
                errorRecord.count = 1;
                errorRecord.severity = CErrorRecord::ESEVERITY_ERROR;
                errorRecord.error = error;
                errorRecord.description = "Possible duplicate objects being loaded, potential fix is to remove duplicate objects from level files.";
                GetIEditor()->GetErrorReport()->ReportError(errorRecord);
            }

            return nullptr;
            //CoCreateGuid( &pObject->m_guid ); // generate uniq GUID for this object.
        }
    }

    GetIEditor()->GetErrorReport()->SetCurrentValidatorObject(pObject);
    if (!pObject->Init(GetIEditor(), nullptr, ""))
    {
        GetIEditor()->GetErrorReport()->SetCurrentValidatorObject(nullptr);
        return nullptr;
    }

    if (!AddObject(pObject))
    {
        GetIEditor()->GetErrorReport()->SetCurrentValidatorObject(nullptr);
        return nullptr;
    }

    //pObject->Serialize( ar );

    GetIEditor()->GetErrorReport()->SetCurrentValidatorObject(nullptr);

    if (pObject != nullptr && pUndoObject == nullptr)
    {
        // If new object with no undo, record it.
        if (CUndo::IsRecording())
        {
            GetIEditor()->RecordUndo(new CUndoBaseObjectNew(pObject));
        }
    }

    return pObject;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CObjectManager::NewObject(const QString& typeName, CBaseObject* prev, const QString& file, const char* newObjectName)
{
    // [9/22/2009 evgeny] If it is "Entity", figure out if a CEntity subclass is actually needed
    QString fullName = typeName + "::" + file;
    CObjectClassDesc* cls = FindClass(fullName);
    if (!cls)
    {
        cls = FindClass(typeName);
    }

    if (!cls)
    {
        GetIEditor()->GetSystem()->GetILog()->Log("Warning: RuntimeClass %s (as well as %s) not registered", typeName.toUtf8().data(), fullName.toUtf8().data());
        return nullptr;
    }
    CBaseObject* pObject = NewObject(cls, prev, file, newObjectName);
    return pObject;
}

//////////////////////////////////////////////////////////////////////////
void    CObjectManager::DeleteObject(CBaseObject* obj)
{
    AZ_PROFILE_FUNCTION(Editor);

    if (!obj)
    {
        return;
    }

    // If object already deleted.
    if (obj->CheckFlags(OBJFLAG_DELETED))
    {
        return;
    }

    obj->NotifyListeners(CBaseObject::ON_PREDELETE);

    // Must be after object DetachAll to support restoring Parent/Child relations.
    // AZ entity deletions are handled through the AZ undo system.
    if (CUndo::IsRecording() && obj->GetType() != OBJTYPE_AZENTITY)
    {
        // Store undo for all child objects.
        for (int i = 0; i < obj->GetChildCount(); i++)
        {
            obj->GetChild(i)->StoreUndo();
        }
        CUndo::Record(new CUndoBaseObjectDelete(obj));
    }

    AABB objAAB;
    obj->GetBoundBox(objAAB);
    GetIEditor()->GetGameEngine()->OnAreaModified(objAAB);

    obj->Done();

    RemoveObject(obj);
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::DeleteAllObjects()
{
    AZ_PROFILE_FUNCTION(Editor);

    InvalidateVisibleList();

    TBaseObjects objectsHolder;
    GetAllObjects(objectsHolder);

    // Clear map. Need to do this before deleting objects in case someone tries to get object list during shutdown.
    m_objects.clear();
    m_objectsByName.clear();

    for (int i = 0; i < objectsHolder.size(); i++)
    {
        objectsHolder[i]->Done();
    }

    //! Delete object instances.
    objectsHolder.clear();

    // Clear name map.
    m_nameNumbersMap.clear();

    m_animatedAttachedEntities.clear();
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CObjectManager::FindObject(REFGUID guid) const
{
    CBaseObject* result = stl::find_in_map(m_objects, guid, (CBaseObject*)nullptr);
    return result;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CObjectManager::FindObject(const QString& sName) const
{
    const AZ::Crc32 crc(sName.toUtf8().data(), sName.toUtf8().count(), true);

    auto iter = m_objectsByName.find(crc);
    if (iter != m_objectsByName.end())
    {
        return iter->second;
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::FindObjectsOfType(ObjectType type, std::vector<CBaseObject*>& result)
{
    result.clear();

    CBaseObjectsArray objects;
    GetObjects(objects);

    for (size_t i = 0, n = objects.size(); i < n; ++i)
    {
        if (objects[i]->GetType() == type)
        {
            result.push_back(objects[i]);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::FindObjectsOfType(const QMetaObject* pClass, std::vector<CBaseObject*>& result)
{
    result.clear();

    CBaseObjectsArray objects;
    GetObjects(objects);

    for (size_t i = 0, n = objects.size(); i < n; ++i)
    {
        CBaseObject* pObject = objects[i];
        if (pObject->metaObject() == pClass)
        {
            result.push_back(pObject);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::FindObjectsInAABB(const AABB& aabb, std::vector<CBaseObject*>& result) const
{
    result.clear();

    CBaseObjectsArray objects;
    GetObjects(objects);

    for (size_t i = 0, n = objects.size(); i < n; ++i)
    {
        CBaseObject* pObject = objects[i];
        AABB aabbObj;
        pObject->GetBoundBox(aabbObj);
        if (aabb.IsIntersectBox(aabbObj))
        {
            result.push_back(pObject);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CObjectManager::AddObject(CBaseObject* obj)
{
    CBaseObjectPtr p = stl::find_in_map(m_objects, obj->GetId(), nullptr);
    if (p)
    {
        CErrorRecord err;
        err.error = QObject::tr("New Object %1 has Duplicate GUID %2, New Object Ignored").arg(obj->GetName()).arg(GuidUtil::ToString(obj->GetId()));
        err.severity = CErrorRecord::ESEVERITY_ERROR;
        err.pObject = obj;
        err.flags = CErrorRecord::FLAG_OBJECTID;
        GetIEditor()->GetErrorReport()->ReportError(err);

        return false;
    }
    m_objects[obj->GetId()] = obj;

    // Handle adding object to type-specific containers if needed
    {
        if (CEntityObject* entityObj = qobject_cast<CEntityObject*>(obj))
        {
            CEntityObject::EAttachmentType attachType = entityObj->GetAttachType();
            if (attachType == CEntityObject::EAttachmentType::eAT_CharacterBone)
            {
                m_animatedAttachedEntities.insert(entityObj);
            }
        }
    }

    const AZ::Crc32 nameCrc(obj->GetName().toUtf8().data(), obj->GetName().toUtf8().count(), true);
    m_objectsByName[nameCrc] = obj;

    RegisterObjectName(obj->GetName());
    InvalidateVisibleList();
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::RemoveObject(CBaseObject* obj)
{
    assert(obj != 0);

    InvalidateVisibleList();

    // Handle removing object from type-specific containers if needed
    {
        if (CEntityObject* entityObj = qobject_cast<CEntityObject*>(obj))
        {
            m_animatedAttachedEntities.erase(entityObj);
        }
    }

    m_objectsByName.erase(AZ::Crc32(obj->GetName().toUtf8().data(), obj->GetName().toUtf8().count(), true));

    // Need to erase this last since it is a smart pointer and can end up deleting the object if it is the last reference to it being kept
    m_objects.erase(obj->GetId());
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::GetAllObjects(TBaseObjects& objects) const
{
    objects.clear();
    objects.reserve(m_objects.size());
    for (Objects::const_iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        objects.push_back(it->second);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::ChangeObjectId(REFGUID oldGuid, REFGUID newGuid)
{
    Objects::iterator it = m_objects.find(oldGuid);
    if (it != m_objects.end())
    {
        CBaseObjectPtr pRemappedObject = (*it).second;
        pRemappedObject->SetId(newGuid);
        m_objects.erase(it);
        m_objects.insert(AZStd::make_pair(newGuid, pRemappedObject));
    }
}

//////////////////////////////////////////////////////////////////////////
int CObjectManager::GetObjectCount() const
{
    return static_cast<int>(m_objects.size());
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::GetObjects(CBaseObjectsArray& objects) const
{
    objects.clear();
    objects.reserve(m_objects.size());
    for (Objects::const_iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        objects.push_back(it->second);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::SendEvent(ObjectEvent event)
{
    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;
        obj->OnEvent(event);
    }

    if (event == EVENT_RELOAD_ENTITY)
    {
        GetIEditor()->Notify(eNotify_OnReloadTrackView);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::SendEvent(ObjectEvent event, const AABB& bounds)
{
    AABB box;
    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;
        obj->GetBoundBox(box);
        if (bounds.IsIntersectBox(box))
        {
            obj->OnEvent(event);
        }
    }
}

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
bool CObjectManager::IsObjectDeletionAllowed(CBaseObject* pObject)
{
    if (!pObject)
    {
        return false;
    }

    return true;
};

//////////////////////////////////////////////////////////////////////////
uint16 FindPossibleObjectNameNumber(std::set<uint16>& numberSet)
{
    const int LIMIT = 65535;
    size_t nSetSize = numberSet.size();
    for (uint16 i = 1; i < LIMIT; ++i)
    {
        uint16 candidateNumber = (i + nSetSize) % LIMIT;
        if (numberSet.find(candidateNumber) == numberSet.end())
        {
            numberSet.insert(candidateNumber);
            return candidateNumber;
        }
    }
    return 0;
}

void CObjectManager::RegisterObjectName(const QString& name)
{
    // Remove all numbers from the end of typename.
    QString typeName = name;
    int nameLen = typeName.length();
    int len = nameLen;
    while (len > 0 && typeName[len - 1].isDigit())
    {
        len--;
    }

    typeName = typeName.left(len);

    uint16 num = 1;
    if (len < nameLen)
    {
        num = (uint16)atoi((const char*)name.toUtf8().data() + len) + 0;
    }

    NameNumbersMap::iterator iNameNumber = m_nameNumbersMap.find(typeName);
    if (iNameNumber == m_nameNumbersMap.end())
    {
        std::set<uint16> numberSet;
        numberSet.insert(num);
        m_nameNumbersMap[typeName] = numberSet;
    }
    else
    {
        std::set<uint16>& numberSet = iNameNumber->second;
        numberSet.insert(num);
    }
}

//////////////////////////////////////////////////////////////////////////
QString CObjectManager::GenerateUniqueObjectName(const QString& theTypeName)
{
    QString typeName = theTypeName;
    const int subIndex = theTypeName.indexOf("::");
    if (subIndex != -1 && subIndex > typeName.length() - 2)
    {
        typeName.remove(0, subIndex + 2);
    }

    // Remove all numbers from the end of typename.
    int len = typeName.length();
    while (len > 0 && typeName[len - 1].isDigit())
    {
        len--;
    }

    typeName = typeName.left(len);

    NameNumbersMap::iterator ii = m_nameNumbersMap.find(typeName);
    uint16 lastNumber = 1;
    if (ii != m_nameNumbersMap.end())
    {
        lastNumber = FindPossibleObjectNameNumber(ii->second);
    }
    else
    {
        std::set<uint16> numberSet;
        numberSet.insert(lastNumber);
        m_nameNumbersMap[typeName] = numberSet;
    }

    QString str = QStringLiteral("%1%2").arg(typeName).arg(lastNumber);

    return str;
}

//////////////////////////////////////////////////////////////////////////
CObjectClassDesc* CObjectManager::FindClass(const QString& className)
{
    IClassDesc* cls = CClassFactory::Instance()->FindClass(className.toUtf8().data());
    if (cls != nullptr && cls->SystemClassID() == ESYSTEM_CLASS_OBJECT)
    {
        return (CObjectClassDesc*)cls;
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::RegisterClassTemplate(const XmlNodeRef& templ)
{
    QString typeName = templ->getTag();
    QString superTypeName;
    if (!templ->getAttr("SuperType", superTypeName))
    {
        return;
    }

    CObjectClassDesc* superType = FindClass(superTypeName);
    if (!superType)
    {
        return;
    }

    QString category, fileSpec, initialName;
    templ->getAttr("Category", category);
    templ->getAttr("File", fileSpec);
    templ->getAttr("Name", initialName);

    CXMLObjectClassDesc* classDesc = new CXMLObjectClassDesc;
    classDesc->superType = superType;
    classDesc->type = typeName;
    classDesc->category = category;
    classDesc->fileSpec = fileSpec;
    classDesc->guid = AZ::Uuid::CreateRandom();

    CClassFactory::Instance()->RegisterClass(classDesc);
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::LoadClassTemplates(const QString& path)
{
    QString dir = Path::AddPathSlash(path);

    IFileUtil::FileArray files;
    CFileUtil::ScanDirectory(dir, "*.xml", files, false);

    for (int k = 0; k < files.size(); k++)
    {
        // Construct the full filepath of the current file
        XmlNodeRef node = XmlHelpers::LoadXmlFromFile((dir + files[k].filename).toUtf8().data());
        if (node != nullptr && node->isTag("ObjectTemplates"))
        {
            QString name;
            for (int i = 0; i < node->getChildCount(); i++)
            {
                RegisterClassTemplate(node->getChild(i));
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::InvalidateVisibleList()
{
    if (m_isUpdateVisibilityList)
    {
        return;
    }
    ++m_visibilitySerialNumber;
    m_visibleObjects.clear();
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::UpdateVisibilityList()
{
    m_isUpdateVisibilityList = true;
    m_visibleObjects.clear();

    bool isInIsolationMode = false;
    AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
        isInIsolationMode, &AzToolsFramework::ToolsApplicationRequestBus::Events::IsEditorInIsolationMode);

    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;
        bool visible = obj->IsPotentiallyVisible();

        // entities not isolated in Isolation Mode will be invisible
        bool isObjectIsolated = obj->IsIsolated();
        visible = visible && (!isInIsolationMode || isObjectIsolated);
        obj->UpdateVisibility(visible);

        // when the new viewport interaction model is enabled we always want to add objects
        // in the view (frustum) to the visible objects list so we can draw feedback for
        // entities being hidden in the viewport when selected in the  entity outliner
        // (EditorVisibleEntityDataCache must be populated even if entities are 'hidden')
        m_visibleObjects.push_back(obj);
    }

    m_isUpdateVisibilityList = false;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::GatherUsedResources(CUsedResources& resources)
{
    CBaseObjectsArray objects;
    GetIEditor()->GetObjectManager()->GetObjects(objects);

    for (int i = 0; i < objects.size(); i++)
    {
        CBaseObject* pObject = objects[i];
        pObject->GatherUsedResources(resources);
    }
}

