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
#include "GizmoManager.h"
#include "AxisGizmo.h"
#include "GameEngine.h"
#include "WaitProgress.h"
#include "Util/Image.h"
#include "ObjectManagerLegacyUndo.h"
#include "Include/HitContext.h"
#include "EditMode/DeepSelection.h"
#include "Plugins/ComponentEntityEditorPlugin/Objects/ComponentEntityObject.h"

#include <AzCore/Console/Console.h>
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
    : m_lastHideMask(0)
    , m_maxObjectViewDistRatio(0.00001f)
    , m_currSelection(&m_defaultSelection)
    , m_nLastSelCount(0)
    , m_bSelectionChanged(false)
    , m_selectCallback(nullptr)
    , m_currEditObject(nullptr)
    , m_bSingleSelection(false)
    , m_createGameObjects(true)
    , m_bGenUniqObjectNames(true)
    , m_gizmoManager(new CGizmoManager())
    , m_pLoadProgress(nullptr)
    , m_loadedObjects(0)
    , m_totalObjectsToLoad(0)
    , m_bExiting(false)
    , m_isUpdateVisibilityList(false)
    , m_currentHideCount(CBaseObject::s_invalidHiddenID)
    , m_bInReloading(false)
    , m_bSkipObjectUpdate(false)
    , m_bLevelExporting(false)
{
    g_pObjectManager = this;

    RegisterObjectClasses();

    m_objectsByName.reserve(1024);
    LoadRegistry();
}

//////////////////////////////////////////////////////////////////////////
CObjectManager::~CObjectManager()
{
    m_bExiting = true;
    SaveRegistry();
    DeleteAllObjects();

    delete m_gizmoManager;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::RegisterObjectClasses()
{
    LoadRegistry();
}

//////////////////////////////////////////////////////////////////////////
void    CObjectManager::SaveRegistry()
{
}

void    CObjectManager::LoadRegistry()
{
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

            // check for script entities
            const char* scriptClassName = "";
            CEntityObject* entityObj = qobject_cast<CEntityObject*>(obj);
            QByteArray entityClass; // Leave it outside of the if. Otherwise buffer is deleted.
            if (entityObj)
            {
                entityClass = entityObj->GetEntityClass().toUtf8();
                scriptClassName = entityClass.data();
            }

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

    m_loadedObjects++;
    if (m_pLoadProgress && m_totalObjectsToLoad > 0)
    {
        m_pLoadProgress->Step((m_loadedObjects * 100) / m_totalObjectsToLoad);
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
    if (m_currEditObject == obj)
    {
        EndEditParams();
    }

    if (!obj)
    {
        return;
    }

    // If object already deleted.
    if (obj->CheckFlags(OBJFLAG_DELETED))
    {
        return;
    }

    NotifyObjectListeners(obj, CBaseObject::ON_PREDELETE);
    obj->NotifyListeners(CBaseObject::ON_PREDELETE);

    // Must be after object DetachAll to support restoring Parent/Child relations.
    // AZ entity deletions are handled through the AZ undo system.
    if (CUndo::IsRecording() && obj->GetType() != OBJTYPE_AZENTITY)
    {
        // Store undo for all child objects.
        for (int i = 0; i < obj->GetChildCount(); i++)
        {
            obj->GetChild(i)->StoreUndo("DeleteParent");
        }
        CUndo::Record(new CUndoBaseObjectDelete(obj));
    }

    AABB objAAB;
    obj->GetBoundBox(objAAB);
    GetIEditor()->GetGameEngine()->OnAreaModified(objAAB);

    obj->Done();

    NotifyObjectListeners(obj, CBaseObject::ON_DELETE);

    RemoveObject(obj);
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::DeleteSelection(CSelectionGroup* pSelection)
{
    AZ_PROFILE_FUNCTION(Editor);
    if (pSelection == nullptr)
    {
        return;
    }

    // if the object contains an entity which has link, the link information should be recorded for undo separately. p

    if (CUndo::IsRecording())
    {
        for (int i = 0, iSize(pSelection->GetCount()); i < iSize; ++i)
        {
            CBaseObject* pObj = pSelection->GetObject(i);
            if (!qobject_cast<CEntityObject*>(pObj))
            {
                continue;
            }

            CEntityObject* pEntity = (CEntityObject*)pObj;
            if (pEntity->GetEntityLinkCount() <= 0)
            {
                continue;
            }

            CEntityObject::StoreUndoEntityLink(pSelection);
            break;
        }
    }

    AzToolsFramework::EntityIdList selectedComponentEntities;
    for (int i = 0, iObjSize(pSelection->GetCount()); i < iObjSize; i++)
    {
        CBaseObject* object = pSelection->GetObject(i);

        // AZ::Entity deletion is handled through AZ undo system (DeleteSelected bus call below).
        if (object->GetType() != OBJTYPE_AZENTITY)
        {
            DeleteObject(object);
        }
        else
        {
            AZ::EntityId id;
            EBUS_EVENT_ID_RESULT(id, object, AzToolsFramework::ComponentEntityObjectRequestBus, GetAssociatedEntityId);
            if (id.IsValid())
            {
                selectedComponentEntities.push_back(id);
            }
        }
    }

    // Delete AZ (component) entities.
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
    {
        EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, DeleteEntities, selectedComponentEntities);
    }
    else
    {
        EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, DeleteEntitiesAndAllDescendants, selectedComponentEntities);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::DeleteAllObjects()
{
    AZ_PROFILE_FUNCTION(Editor);

    EndEditParams();

    ClearSelection();
    int i;

    InvalidateVisibleList();

    // Delete all selection groups.
    std::vector<CSelectionGroup*> sel;
    stl::map_to_vector(m_selections, sel);
    for (i = 0; i < sel.size(); i++)
    {
        delete sel[i];
    }
    m_selections.clear();

    TBaseObjects objectsHolder;
    GetAllObjects(objectsHolder);

    // Clear map. Need to do this before deleting objects in case someone tries to get object list during shutdown.
    m_objects.clear();
    m_objectsByName.clear();

    for (i = 0; i < objectsHolder.size(); i++)
    {
        objectsHolder[i]->Done();
    }

    //! Delete object instances.
    objectsHolder.clear();

    // Clear name map.
    m_nameNumbersMap.clear();

    m_animatedAttachedEntities.clear();
}

CBaseObject* CObjectManager::CloneObject(CBaseObject* obj)
{
    AZ_PROFILE_FUNCTION(Editor);
    assert(obj);
    //CRuntimeClass *cls = obj->GetRuntimeClass();
    //CBaseObject *clone = (CBaseObject*)cls->CreateObject();
    //clone->CloneCopy( obj );
    CBaseObject* clone = NewObject(obj->GetClassDesc(), obj);
    return clone;
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
            if (attachType == CEntityObject::EAttachmentType::eAT_GeomCacheNode || attachType == CEntityObject::EAttachmentType::eAT_CharacterBone)
            {
                m_animatedAttachedEntities.insert(entityObj);
            }
        }
    }

    const AZ::Crc32 nameCrc(obj->GetName().toUtf8().data(), obj->GetName().toUtf8().count(), true);
    m_objectsByName[nameCrc] = obj;

    RegisterObjectName(obj->GetName());
    InvalidateVisibleList();
    NotifyObjectListeners(obj, CBaseObject::ON_ADD);
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

    // Remove this object from selection groups.
    m_currSelection->RemoveObject(obj);
    std::vector<CSelectionGroup*> sel;
    stl::map_to_vector(m_selections, sel);
    for (int i = 0; i < sel.size(); i++)
    {
        sel[i]->RemoveObject(obj);
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
        m_objects.insert(std::make_pair(newGuid, pRemappedObject));
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::ShowDuplicationMsgWarning(CBaseObject* obj, const QString& newName, bool bShowMsgBox) const
{
    CBaseObject* pExisting = FindObject(newName);
    if (pExisting)
    {
        QString sRenameWarning = QObject::tr("%1 \"%2\" was NOT renamed to \"%3\" because %4 with the same name already exists.")
            .arg(obj->GetClassDesc()->ClassName())
            .arg(obj->GetName())
            .arg(newName)
            .arg(pExisting->GetClassDesc()->ClassName()
        );

        // If id is taken.
        CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "%s", sRenameWarning.toUtf8().data());

        if (bShowMsgBox)
        {
            QMessageBox::critical(QApplication::activeWindow(), QString(), sRenameWarning);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::ChangeObjectName(CBaseObject* obj, const QString& newName)
{
    assert(obj);

    if (newName != obj->GetName())
    {
        if (IsDuplicateObjectName(newName))
        {
            return;
        }

        // Remove previous name from map
        const AZ::Crc32 oldNameCrc(obj->GetName().toUtf8().data(), obj->GetName().count(), true);
        m_objectsByName.erase(oldNameCrc);

        obj->SetName(newName);

        // Add new name to map
        const AZ::Crc32 nameCrc(newName.toUtf8().data(), newName.count(), true);
        m_objectsByName[nameCrc] = obj;
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

void CObjectManager::GetObjects(CBaseObjectsArray& objects, BaseObjectFilterFunctor const& filter) const
{
    objects.clear();
    objects.reserve(m_objects.size());
    for (Objects::const_iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        assert(it->second);
        if (filter.first(*it->second, filter.second))
        {
            objects.push_back(it->second);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::SendEvent(ObjectEvent event)
{
    if (event == EVENT_RELOAD_ENTITY)
    {
        m_bInReloading = true;
    }

    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;
        obj->OnEvent(event);
    }

    if (event == EVENT_RELOAD_ENTITY)
    {
        m_bInReloading = false;
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
void CObjectManager::Update()
{
    if (m_bSkipObjectUpdate)
    {
        return;
    }

    QWidget* prevActiveWindow = QApplication::activeWindow();

    // Restore focus if it changed.
    if (prevActiveWindow && QApplication::activeWindow() != prevActiveWindow)
    {
        prevActiveWindow->setFocus();
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::HideObject(CBaseObject* obj, bool hide)
{
    assert(obj != 0);
    if (hide)
    {
        obj->SetHidden(hide, ++m_currentHideCount);
    }
    else
    {
        obj->SetHidden(false);
    }
    InvalidateVisibleList();
}

void CObjectManager::ShowLastHiddenObject()
{
    uint64 mostRecentID = CBaseObject::s_invalidHiddenID;
    CBaseObject* mostRecentObject = nullptr;
    for (auto it : m_objects)
    {
        CBaseObject* obj = it.second;

        uint64 hiddenID = obj->GetHideOrder();

        if (hiddenID > mostRecentID)
        {
            mostRecentID = hiddenID;
            mostRecentObject = obj;
        }
    }

    if (mostRecentObject != nullptr)
    {
        mostRecentObject->SetHidden(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::UnhideAll()
{
    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;
        obj->SetHidden(false);
    }

    InvalidateVisibleList();
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::FreezeObject(CBaseObject* obj, bool freeze)
{
    assert(obj != 0);
    // Remove object from main object set and put it to hidden set.
    obj->SetFrozen(freeze);
    InvalidateVisibleList();
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::UnfreezeAll()
{
    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;
        obj->SetFrozen(false);
    }
    InvalidateVisibleList();
}

//////////////////////////////////////////////////////////////////////////
bool CObjectManager::SelectObject(CBaseObject* obj, bool bUseMask)
{
    assert(obj);
    if (obj == nullptr)
    {
        return false;
    }

    // Check if can be selected.
    if (bUseMask && (!(obj->GetType() & gSettings.objectSelectMask)))
    {
        return false;
    }

    if (m_selectCallback)
    {
        if (!m_selectCallback->OnSelectObject(obj))
        {
            return true;
        }
    }

    m_currSelection->AddObject(obj);

    // while in ComponentMode we never explicitly change selection (the entity will always be selected).
    // this check is to handle the case where an undo or redo action has occurred and
    // the entity has been destroyed and recreated as part of the deserialization step.
    // we want the internal state to stay consistent but do not want to notify other systems of the change.
    if (AzToolsFramework::ComponentModeFramework::InComponentMode())
    {
        obj->SetSelected(true);
    }
    else
    {
        SetObjectSelected(obj, true);
        GetIEditor()->Notify(eNotify_OnSelectionChange);
    }

    return true;
}

void CObjectManager::SelectEntities(std::set<CEntityObject*>& s)
{
    for (std::set<CEntityObject*>::iterator it = s.begin(), end = s.end(); it != end; ++it)
    {
        SelectObject(*it);
    }
}

void CObjectManager::UnselectObject(CBaseObject* obj)
{
    // while in ComponentMode we never explicitly change selection (the entity will always be selected).
    // this check is to handle the case where an undo or redo action has occurred and
    // the entity has been destroyed and recreated as part of the deserialization step.
    // we want the internal state to stay consistent but do not want to notify other systems of the change.
    if (AzToolsFramework::ComponentModeFramework::InComponentMode())
    {
        obj->SetSelected(false);
    }
    else
    {
        SetObjectSelected(obj, false);
    }

    m_currSelection->RemoveObject(obj);
}

CSelectionGroup* CObjectManager::GetSelection(const QString& name) const
{
    CSelectionGroup* selection = stl::find_in_map(m_selections, name, (CSelectionGroup*)nullptr);
    return selection;
}

void CObjectManager::GetNameSelectionStrings(QStringList& names)
{
    for (TNameSelectionMap::iterator it = m_selections.begin(); it != m_selections.end(); ++it)
    {
        names.push_back(it->first);
    }
}

void CObjectManager::NameSelection(const QString& name)
{
    if (m_currSelection->IsEmpty())
    {
        return;
    }

    CSelectionGroup* selection = stl::find_in_map(m_selections, name, (CSelectionGroup*)nullptr);
    if (selection)
    {
        assert(selection != 0);
        // Check if trying to rename itself to the same name.
        if (selection == m_currSelection)
        {
            return;
        }
        m_selections.erase(name);
        delete selection;
    }
    selection = new CSelectionGroup;
    selection->Copy(*m_currSelection);
    selection->SetName(name);
    m_selections[name] = selection;
    m_currSelection = selection;
    m_defaultSelection.RemoveAll();
}

void CObjectManager::SerializeNameSelection(XmlNodeRef& rootNode, bool bLoading)
{
    if (!rootNode)
    {
        return;
    }

    _smart_ptr<CSelectionGroup> tmpGroup(nullptr);

    QString selRootStr("NameSelection");
    QString selNodeStr("NameSelectionNode");
    QString selNodeNameStr("name");
    QString idStr("id");
    QString objAttrStr("obj");

    XmlNodeRef startNode = rootNode->findChild(selRootStr.toUtf8().data());

    if (bLoading)
    {
        m_selections.erase(m_selections.begin(), m_selections.end());

        if (startNode)
        {
            for (int selNodeNo = 0; selNodeNo < startNode->getChildCount(); ++selNodeNo)
            {
                XmlNodeRef selNode = startNode->getChild(selNodeNo);
                tmpGroup = new CSelectionGroup;

                for (int objIDNodeNo = 0; objIDNodeNo < selNode->getChildCount(); ++objIDNodeNo)
                {
                    GUID curID = GUID_NULL;
                    XmlNodeRef idNode = selNode->getChild(objIDNodeNo);
                    if (!idNode->getAttr(idStr.toUtf8().data(), curID))
                    {
                        continue;
                    }

                    if (curID != GUID_NULL)
                    {
                        if (GetIEditor()->GetObjectManager()->FindObject(curID))
                        {
                            tmpGroup->AddObject(GetIEditor()->GetObjectManager()->FindObject(curID));
                        }
                    }
                }

                if (tmpGroup->GetCount() > 0)
                {
                    QString nameStr;
                    if (!selNode->getAttr(selNodeNameStr.toUtf8().data(), nameStr))
                    {
                        continue;
                    }
                    tmpGroup->SetName(nameStr);
                    m_selections[nameStr] = tmpGroup;
                }
            }
        }
    }
    else
    {
        startNode = rootNode->newChild(selRootStr.toUtf8().data());
        CSelectionGroup* objSelection = nullptr;

        for (TNameSelectionMap::iterator it = m_selections.begin(); it != m_selections.end(); ++it)
        {
            XmlNodeRef selectionNameNode = startNode->newChild(selNodeStr.toUtf8().data());
            selectionNameNode->setAttr(selNodeNameStr.toUtf8().data(), it->first.toUtf8().data());
            objSelection = it->second;

            if (!objSelection)
            {
                continue;
            }

            if (objSelection->GetCount() == 0)
            {
                continue;
            }

            for (int i = 0; i < objSelection->GetCount(); ++i)
            {
                if (objSelection->GetObject(i))
                {
                    XmlNodeRef objNode = selectionNameNode->newChild(objAttrStr.toUtf8().data());
                    objNode->setAttr(idStr.toUtf8().data(), GuidUtil::ToString(objSelection->GetObject(i)->GetId()));
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
int CObjectManager::ClearSelection()
{
    AZ_PROFILE_FUNCTION(Editor);

    // Make sure to unlock selection.
    GetIEditor()->LockSelection(false);

    int numSel = m_currSelection->GetCount();

    // Handle Undo/Redo of Component Entities
    bool isUndoRecording = GetIEditor()->IsUndoRecording();
    if (isUndoRecording)
    {
        m_processingBulkSelect = true;
        GetIEditor()->RecordUndo(new CUndoBaseObjectClearSelection(*m_currSelection));
    }

    // Handle legacy entities separately so the selection group can be cleared safely.
    // This prevents every AzEntity from being removed one by one from a vector.
    m_currSelection->RemoveAllExceptLegacySet();

    // Kick off Deselect for Legacy Entities
    for (CBaseObjectPtr legacyObject : m_currSelection->GetLegacyObjects())
    {
        if (isUndoRecording && legacyObject->IsSelected())
        {
            GetIEditor()->RecordUndo(new CUndoBaseObjectSelect(legacyObject));
        }

        SetObjectSelected(legacyObject, false);
    }

    // Legacy set is cleared
    m_defaultSelection.RemoveAll();
    m_currSelection = &m_defaultSelection;
    m_bSelectionChanged = true;

    // Unselect all component entities as one bulk operation instead of individually
    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities,
        AzToolsFramework::EntityIdList());

    m_processingBulkSelect = false;

    if (!m_bExiting)
    {
        GetIEditor()->Notify(eNotify_OnSelectionChange);
    }

    return numSel;
}

//////////////////////////////////////////////////////////////////////////
int CObjectManager::InvertSelection()
{
    AZ_PROFILE_FUNCTION(Editor);

    int selCount = 0;
    // iterate all objects.
    for (Objects::const_iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* pObj = it->second;
        if (pObj->IsSelected())
        {
            UnselectObject(pObj);
        }
        else
        {
            if (SelectObject(pObj))
            {
                selCount++;
            }
        }
    }
    return selCount;
}

void CObjectManager::SetSelection(const QString& name)
{
    AZ_PROFILE_FUNCTION(Editor);
    CSelectionGroup* selection = stl::find_in_map(m_selections, name, (CSelectionGroup*)nullptr);
    if (selection)
    {
        UnselectCurrent();
        assert(selection != 0);
        m_currSelection = selection;
        SelectCurrent();
    }
}

void CObjectManager::RemoveSelection(const QString& name)
{
    AZ_PROFILE_FUNCTION(Editor);

    QString selName = name;
    CSelectionGroup* selection = stl::find_in_map(m_selections, name, (CSelectionGroup*)nullptr);
    if (selection)
    {
        if (selection == m_currSelection)
        {
            UnselectCurrent();
            m_currSelection = &m_defaultSelection;
            m_defaultSelection.RemoveAll();
        }
        delete selection;
        m_selections.erase(selName);
    }
}

void CObjectManager::SelectCurrent()
{
    AZ_PROFILE_FUNCTION(Editor);
    for (int i = 0; i < m_currSelection->GetCount(); i++)
    {
        CBaseObject* obj = m_currSelection->GetObject(i);
        if (GetIEditor()->IsUndoRecording() && !obj->IsSelected())
        {
            GetIEditor()->RecordUndo(new CUndoBaseObjectSelect(obj));
        }

        SetObjectSelected(obj, true);
    }
}

void CObjectManager::UnselectCurrent()
{
    AZ_PROFILE_FUNCTION(Editor);

    // Make sure to unlock selection.
    GetIEditor()->LockSelection(false);

    // Unselect all component entities as one bulk operation instead of individually
    AzToolsFramework::EntityIdList selectedEntities;
    EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, SetSelectedEntities, selectedEntities);

    for (int i = 0; i < m_currSelection->GetCount(); i++)
    {
        CBaseObject* obj = m_currSelection->GetObject(i);
        if (GetIEditor()->IsUndoRecording() && obj->IsSelected())
        {
            GetIEditor()->RecordUndo(new CUndoBaseObjectSelect(obj));
        }

        SetObjectSelected(obj, false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::Display(DisplayContext& dc)
{
    AZ_PROFILE_FUNCTION(Editor);

    int currentHideMask = GetIEditor()->GetDisplaySettings()->GetObjectHideMask();
    if (m_lastHideMask != currentHideMask)
    {
        // a setting has changed which may cause the set of currently visible objects to change, so invalidate the serial number
        // so that viewports and anyone else that needs to update settings knows it has to.
        m_lastHideMask = currentHideMask;
        ++m_visibilitySerialNumber;
    }

    // the object manager itself has a visibility list, so it also has to update its cache when the serial has changed
    if (m_visibilitySerialNumber != m_lastComputedVisibility)
    {
        m_lastComputedVisibility = m_visibilitySerialNumber;
        UpdateVisibilityList();
    }

    if (dc.settings->IsDisplayHelpers())
    {
        // Also broadcast for anyone else that needs to draw global debug to do so now
        AzFramework::DebugDisplayEventBus::Broadcast(&AzFramework::DebugDisplayEvents::DrawGlobalDebugInfo);
    }

    if (m_gizmoManager)
    {
        m_gizmoManager->Display(dc);
    }
}

void CObjectManager::ForceUpdateVisibleObjectCache([[maybe_unused]] DisplayContext& dc)
{
    AZ_Assert(false, "CObjectManager::ForceUpdateVisibleObjectCache is legacy/deprecated and should not be used.");
}

void CObjectManager::FindDisplayableObjects([[maybe_unused]] DisplayContext& dc, [[maybe_unused]] bool bDisplay)
{
    AZ_Assert(false, "CObjectManager::FindDisplayableObjects is legacy/deprecated and should not be used.");
}

void CObjectManager::BeginEditParams(CBaseObject* obj, int flags)
{
    assert(obj != 0);
    if (obj == m_currEditObject)
    {
        return;
    }

    if (GetSelection()->GetCount() > 1)
    {
        return;
    }

    QWidget* prevActiveWindow = QApplication::activeWindow();

    if (m_currEditObject)
    {
        //if (obj->GetClassDesc() != m_currEditObject->GetClassDesc())
        if (!obj->IsSameClass(m_currEditObject))
        {
            EndEditParams(flags);
        }
    }

    m_currEditObject = obj;

    if (flags & OBJECT_CREATE)
    {
        // Unselect all other objects.
        ClearSelection();
        // Select this object.
        SelectObject(obj, false);
    }

    m_bSingleSelection = true;

    // Restore focus if it changed.
    //  OBJECT_EDIT is used by the EntityOutliner when items are selected. Using it here to prevent shifting focus to the EntityInspector on select.
    if (!(flags & OBJECT_EDIT) && prevActiveWindow && QApplication::activeWindow() != prevActiveWindow)
    {
        prevActiveWindow->setFocus();
    }
}

void CObjectManager::EndEditParams([[maybe_unused]] int flags)
{
    m_bSingleSelection = false;
    m_currEditObject = nullptr;
    //m_bSelectionChanged = false; // don't need to clear for ungroup
}

//! Select objects within specified distance from given position.
int CObjectManager::SelectObjects(const AABB& box, bool bUnselect)
{
    AZ_PROFILE_FUNCTION(Editor);
    int numSel = 0;

    AABB objBounds;
    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;

        if (obj->IsHidden())
        {
            continue;
        }

        obj->GetBoundBox(objBounds);
        if (box.IsIntersectBox(objBounds))
        {
            numSel++;
            if (!bUnselect)
            {
                SelectObject(obj);
            }
            else
            {
                UnselectObject(obj);
            }
        }
    }
    return numSel;
}

//////////////////////////////////////////////////////////////////////////
int CObjectManager::MoveObjects(const AABB& box, const Vec3& offset, ImageRotationDegrees rotation, [[maybe_unused]] bool bIsCopy)
{
    AABB objBounds;

    Vec3 src = (box.min + box.max) / 2;
    Vec3 dst = src + offset;
    float alpha = 0.0f;
    switch (rotation)
    {
        case ImageRotationDegrees::Rotate90:
            alpha = gf_halfPI;
            break;
        case ImageRotationDegrees::Rotate180:
            alpha = gf_PI;
            break;
        case ImageRotationDegrees::Rotate270:
            alpha = gf_PI + gf_halfPI;
            break;
        default:
            break;
    }

    float cosa = cos(alpha);
    float sina = sin(alpha);

    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;

        if (obj->GetParent())
        {
            continue;
        }

        obj->GetBoundBox(objBounds);
        if (box.IsIntersectBox(objBounds))
        {
            if (rotation == ImageRotationDegrees::Rotate0)
            {
                obj->SetPos(obj->GetPos() - src + dst);
            }
            else
            {
                Vec3 pos = obj->GetPos() - src;
                Vec3 newPos(pos);
                newPos.x = cosa * pos.x - sina * pos.y;
                newPos.y = sina * pos.x + cosa * pos.y;
                obj->SetPos(newPos + dst);
                Quat q;
                obj->SetRotation(q.CreateRotationZ(alpha) * obj->GetRotation());
            }
        }
    }
    return 0;
}

bool CObjectManager::IsObjectDeletionAllowed(CBaseObject* pObject)
{
    if (!pObject)
    {
        return false;
    }

    return true;
};

//////////////////////////////////////////////////////////////////////////
void CObjectManager::DeleteSelection()
{
    AZ_PROFILE_FUNCTION(Editor);

    // Make sure to unlock selection.
    GetIEditor()->LockSelection(false);

    CSelectionGroup objects;
    for (int i = 0; i < m_currSelection->GetCount(); i++)
    {
        // Check condition(s) if object could be deleted
        if (!IsObjectDeletionAllowed(m_currSelection->GetObject(i)))
        {
            return;
        }

        objects.AddObject(m_currSelection->GetObject(i));
    }

    RemoveSelection(m_currSelection->GetName());
    m_currSelection = &m_defaultSelection;
    m_defaultSelection.RemoveAll();

    DeleteSelection(&objects);
}

//////////////////////////////////////////////////////////////////////////
bool CObjectManager::HitTestObject(CBaseObject* obj, HitContext& hc)
{
    AZ_PROFILE_FUNCTION(Editor);

    if (obj->IsFrozen())
    {
        return false;
    }

    if (obj->IsHidden())
    {
        return false;
    }

    // This object is rejected by deep selection.
    if (obj->CheckFlags(OBJFLAG_NO_HITTEST))
    {
        return false;
    }

    ObjectType objType = obj->GetType();

    // Check if this object type is masked for selection.
    if (!(objType & gSettings.objectSelectMask))
    {
        return false;
    }

    const bool bSelectionHelperHit = obj->HitHelperTest(hc);

    if (hc.bUseSelectionHelpers && !bSelectionHelperHit)
    {
        return false;
    }

    if (!bSelectionHelperHit)
    {
        // Fast checking.
        if (hc.camera && !obj->IsInCameraView(*hc.camera))
        {
            return false;
        }
        else if (hc.bounds && !obj->IntersectRectBounds(*hc.bounds))
        {
            return false;
        }

        // Do 2D space testing.
        if (hc.nSubObjFlags == 0)
        {
            Ray ray(hc.raySrc, hc.rayDir);
            if (!obj->IntersectRayBounds(ray))
            {
                return false;
            }
        }
        else if (!obj->HitTestRect(hc))
        {
            return false;
        }
    }

    return (bSelectionHelperHit || obj->HitTest(hc));
}

//////////////////////////////////////////////////////////////////////////
bool CObjectManager::HitTest([[maybe_unused]] HitContext& hitInfo)
{
    AZ_Assert(false, "CObjectManager::HitTest is legacy/deprecated and should not be used.");
    return false;
}

void CObjectManager::FindObjectsInRect(
    [[maybe_unused]] CViewport* view, [[maybe_unused]] const QRect& rect, [[maybe_unused]] std::vector<GUID>& guids)
{
    AZ_Assert(false, "CObjectManager::FindObjectsInRect is legacy/deprecated and should not be used.");
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::SelectObjectsInRect(
    [[maybe_unused]] CViewport* view, [[maybe_unused]] const QRect& rect, [[maybe_unused]] bool bSelect)
{
    AZ_Assert(false, "CObjectManager::SelectObjectsInRect is legacy/deprecated and should not be used.");
}

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
void CObjectManager::UpdateRegisterObjectName(const QString& name)
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

    NameNumbersMap::iterator it = m_nameNumbersMap.find(typeName);

    if (it != m_nameNumbersMap.end())
    {
        if (it->second.end() != it->second.find(num))
        {
            it->second.erase(num);
            if (it->second.empty())
            {
                m_nameNumbersMap.erase(it);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
QString CObjectManager::GenerateUniqueObjectName(const QString& theTypeName)
{
    if (!m_bGenUniqObjectNames)
    {
        return theTypeName;
    }

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
bool CObjectManager::EnableUniqObjectNames(bool bEnable)
{
    bool bPrev = m_bGenUniqObjectNames;
    m_bGenUniqObjectNames = bEnable;
    return bPrev;
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
void CObjectManager::GetClassCategories(QStringList& categories)
{
    std::vector<IClassDesc*> classes;
    CClassFactory::Instance()->GetClassesBySystemID(ESYSTEM_CLASS_OBJECT, classes);
    std::set<QString> cset;
    for (int i = 0; i < classes.size(); i++)
    {
        QString category = classes[i]->Category();
        if (!category.isEmpty())
        {
            cset.insert(category);
        }
    }
    categories.clear();
    categories.reserve(static_cast<int>(cset.size()));
    for (std::set<QString>::iterator cit = cset.begin(); cit != cset.end(); ++cit)
    {
        categories.push_back(*cit);
    }
}

void CObjectManager::GetClassCategoryToolClassNamePairs(std::vector< std::pair<QString, QString> >& categoryToolClassNamePairs)
{
    std::vector<IClassDesc*> classes;
    CClassFactory::Instance()->GetClassesBySystemID(ESYSTEM_CLASS_OBJECT, classes);
    std::set< std::pair<QString, QString> > cset;
    for (int i = 0; i < classes.size(); i++)
    {
        QString category = classes[i]->Category();
        QString toolClassName = ((CObjectClassDesc*)classes[i])->GetToolClassName();
        if (!category.isEmpty())
        {
            cset.insert(std::pair<QString, QString>(category, toolClassName));
        }
    }
    categoryToolClassNamePairs.clear();
    categoryToolClassNamePairs.reserve(cset.size());
    for (std::set< std::pair<QString, QString> >::iterator cit = cset.begin(); cit != cset.end(); ++cit)
    {
        categoryToolClassNamePairs.push_back(*cit);
    }
}

void CObjectManager::GetClassTypes(const QString& category, QStringList& types)
{
    std::vector<IClassDesc*> classes;
    CClassFactory::Instance()->GetClassesBySystemID(ESYSTEM_CLASS_OBJECT, classes);
    for (int i = 0; i < classes.size(); i++)
    {
        QString cat = classes[i]->Category();
        if (QString::compare(cat, category, Qt::CaseInsensitive) == 0 && classes[i]->IsEnabled())
        {
            types.push_back(classes[i]->ClassName());
        }
    }
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
void CObjectManager::RegisterCVars()
{
    REGISTER_CVAR2("AxisHelperHitRadius",
        &m_axisHelperHitRadius,
        20,
        VF_DEV_ONLY,
        "Adjust the hit radius used for axis helpers, like the transform gizmo.");
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::Serialize(XmlNodeRef& xmlNode, bool bLoading, int flags)
{
    if (!xmlNode)
    {
        return;
    }

    if (bLoading)
    {
        m_loadedObjects = 0;

        if (flags == SERIALIZE_ONLY_NOTSHARED)
        {
            DeleteNotSharedObjects();
        }
        else if (flags == SERIALIZE_ONLY_SHARED)
        {
            DeleteSharedObjects();
        }
        else
        {
            DeleteAllObjects();
        }


        XmlNodeRef root = xmlNode->findChild("Objects");

        int totalObjects = 0;

        if (root)
        {
            root->getAttr("NumObjects", totalObjects);
        }


        StartObjectsLoading(totalObjects);

        CObjectArchive ar(this, xmlNode, true);

        // Loading.
        if (root)
        {
            ar.node = root;
            LoadObjects(ar, false);
        }
        EndObjectsLoading();
    }
    else
    {
        // Saving.
        XmlNodeRef root = xmlNode->newChild("Objects");

        CObjectArchive ar(this, root, false);

        // Save all objects to XML.
        for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
        {
            CBaseObject* obj = it->second;

            if (obj->CheckFlags(OBJFLAG_DONT_SAVE))
            {
                continue;
            }

            if ((flags == SERIALIZE_ONLY_SHARED) && !obj->CheckFlags(OBJFLAG_SHARED))
            {
                continue;
            }
            else if ((flags == SERIALIZE_ONLY_NOTSHARED) && obj->CheckFlags(OBJFLAG_SHARED))
            {
                continue;
            }

            XmlNodeRef objNode = root->newChild("Object");
            ar.node = objNode;
            obj->Serialize(ar);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::LoadObjects(CObjectArchive& objectArchive, bool bSelect)
{
    m_bLoadingObjects = true;

    XmlNodeRef objectsNode = objectArchive.node;
    int numObjects = objectsNode->getChildCount();
    for (int i = 0; i < numObjects; i++)
    {
        objectArchive.node = objectsNode->getChild(i);
        CBaseObject* obj = objectArchive.LoadObject(objectsNode->getChild(i));
        if (obj && bSelect)
        {
            SelectObject(obj);
        }
    }
    EndObjectsLoading(); // End progress bar, here, Resolve objects have his own.
    objectArchive.ResolveObjects();

    InvalidateVisibleList();

    m_bLoadingObjects = false;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::Export(const QString& levelPath, XmlNodeRef& rootNode, bool onlyShared)
{
    // Clear export files.
    QFile::remove(QStringLiteral("%1TagPoints.ini").arg(levelPath));
    QFile::remove(QStringLiteral("%1Volumes.ini").arg(levelPath));

    // Save all objects to XML.
    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;
        // Export Only shared objects.
        if ((obj->CheckFlags(OBJFLAG_SHARED) && onlyShared) ||
            (!obj->CheckFlags(OBJFLAG_SHARED) && !onlyShared))
        {
            obj->Export(levelPath, rootNode);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::ExportEntities(XmlNodeRef& rootNode)
{
    // Save all objects to XML.
    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;
        if (qobject_cast<CEntityObject*>(obj))
        {
            obj->Export("", rootNode);
        }
    }
}

void CObjectManager::DeleteNotSharedObjects()
{
    TBaseObjects objects;
    GetAllObjects(objects);
    for (int i = 0; i < objects.size(); i++)
    {
        CBaseObject* obj = objects[i];
        if (!obj->CheckFlags(OBJFLAG_SHARED))
        {
            DeleteObject(obj);
        }
    }
}

void CObjectManager::DeleteSharedObjects()
{
    TBaseObjects objects;
    GetAllObjects(objects);
    for (int i = 0; i < objects.size(); i++)
    {
        CBaseObject* obj = objects[i];
        if (obj->CheckFlags(OBJFLAG_SHARED))
        {
            DeleteObject(obj);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
IObjectSelectCallback* CObjectManager::SetSelectCallback(IObjectSelectCallback* callback)
{
    IObjectSelectCallback* prev = m_selectCallback;
    m_selectCallback = callback;
    return prev;
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
bool CObjectManager::ConvertToType(CBaseObject* pObject, const QString& typeName)
{
    QString message = QString("Convert ") + pObject->GetName() + " to " + typeName;
    CUndo undo(message.toUtf8().data());

    CBaseObjectPtr pNewObject = GetIEditor()->NewObject(typeName.toUtf8().data());
    if (pNewObject)
    {
        if (pNewObject->ConvertFromObject(pObject))
        {
            DeleteObject(pObject);
            return true;
        }
        DeleteObject(pNewObject);
    }

    Log((message + " is failed.").toUtf8().data());
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::SetObjectSelected(CBaseObject* pObject, bool bSelect)
{
    AZ_PROFILE_FUNCTION(Editor);
    // Only select/unselect once.
    if ((pObject->IsSelected() && bSelect) || (!pObject->IsSelected() && !bSelect))
    {
        return;
    }

    // Store selection undo.
    if (CUndo::IsRecording() && !m_processingBulkSelect)
    {
        CUndo::Record(new CUndoBaseObjectSelect(pObject));
    }

    pObject->SetSelected(bSelect);
    m_bSelectionChanged = true;


    if (bSelect && !GetIEditor()->GetTransformManipulator())
    {
        if (CAxisGizmo::GetGlobalAxisGizmoCount() < 1 /*legacy axisGizmoMaxCount*/)
        {
            // Create axis gizmo for this object.
            m_gizmoManager->AddGizmo(new CAxisGizmo(pObject));
        }
    }

    if (bSelect)
    {
        NotifyObjectListeners(pObject, CBaseObject::ON_SELECT);
    }
    else
    {
        NotifyObjectListeners(pObject, CBaseObject::ON_UNSELECT);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::HideTransformManipulators()
{
    m_gizmoManager->DeleteAllTransformManipulators();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CObjectManager::AddObjectEventListener(EventListener* listener)
{
    stl::push_back_unique(m_objectEventListeners, listener);
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::RemoveObjectEventListener(EventListener* listener)
{
    stl::find_and_erase(m_objectEventListeners, listener);
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::NotifyObjectListeners(CBaseObject* pObject, CBaseObject::EObjectListenerEvent event)
{
    std::list<EventListener*>::iterator next;
    for (std::list<EventListener*>::iterator it = m_objectEventListeners.begin(); it != m_objectEventListeners.end(); it = next)
    {
        next = it;
        ++next;
        // Call listener callback.
        (*it)->OnObjectEvent(pObject, event);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::StartObjectsLoading(int numObjects)
{
    if (m_pLoadProgress)
    {
        return;
    }
    m_pLoadProgress = new CWaitProgress("Loading Objects");
    m_totalObjectsToLoad = numObjects;
    m_loadedObjects = 0;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::EndObjectsLoading()
{
    if (m_pLoadProgress)
    {
        delete m_pLoadProgress;
    }
    m_pLoadProgress = nullptr;
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

//////////////////////////////////////////////////////////////////////////
IGizmoManager* CObjectManager::GetGizmoManager()
{
    return m_gizmoManager;
}

//////////////////////////////////////////////////////////////////////////
bool CObjectManager::IsLightClass(CBaseObject* pObject)
{
    if (qobject_cast<CEntityObject*>(pObject))
    {
        CEntityObject* pEntity = (CEntityObject*)pObject;
        if (pEntity)
        {
            if (pEntity->GetEntityClass().compare(CLASS_LIGHT) == 0)
            {
                return true;
            }
            if (pEntity->GetEntityClass().compare(CLASS_RIGIDBODY_LIGHT) == 0)
            {
                return true;
            }
            if (pEntity->GetEntityClass().compare(CLASS_DESTROYABLE_LIGHT) == 0)
            {
                return true;
            }
        }
    }

    return false;
}

void CObjectManager::FindAndRenameProperty2(const char* property2Name, const QString& oldValue, const QString& newValue)
{
    CBaseObjectsArray objects;
    GetObjects(objects);

    for (size_t i = 0, n = objects.size(); i < n; ++i)
    {
        CBaseObject* pObject = objects[i];
        if (qobject_cast<CEntityObject*>(pObject))
        {
            CEntityObject* pEntity = static_cast<CEntityObject*>(pObject);
            CVarBlock* pProperties2 = pEntity->GetProperties2();
            if (pProperties2)
            {
                IVariable* pVariable = pProperties2->FindVariable(property2Name);
                if (pVariable)
                {
                    QString sValue;
                    pVariable->Get(sValue);
                    if (sValue == oldValue)
                    {
                        pEntity->StoreUndo("Rename Property2");

                        pVariable->Set(newValue);
                    }
                }
            }
        }
    }
}

void CObjectManager::FindAndRenameProperty2If(const char* property2Name, const QString& oldValue, const QString& newValue, const char* otherProperty2Name, const QString& otherValue)
{
    CBaseObjectsArray objects;
    GetObjects(objects);

    for (size_t i = 0, n = objects.size(); i < n; ++i)
    {
        CBaseObject* pObject = objects[i];
        if (qobject_cast<CEntityObject*>(pObject))
        {
            CEntityObject* pEntity = static_cast<CEntityObject*>(pObject);
            CVarBlock* pProperties2 = pEntity->GetProperties2();
            if (pProperties2)
            {
                IVariable* pVariable      = pProperties2->FindVariable(property2Name);
                IVariable* pOtherVariable = pProperties2->FindVariable(otherProperty2Name);
                if (pVariable && pOtherVariable)
                {
                    QString sValue;
                    pVariable->Get(sValue);

                    QString sOtherValue;
                    pOtherVariable->Get(sOtherValue);

                    if ((sValue == oldValue) && (sOtherValue == otherValue))
                    {
                        pEntity->StoreUndo("Rename Property2 If");

                        pVariable->Set(newValue);
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::HitTestObjectAgainstRect(CBaseObject* pObj, CViewport* view, HitContext hc, std::vector<GUID>& guids)
{
    if (!pObj->IsSelectable())
    {
        return;
    }

    AABB box;

    // Retrieve world space bound box.
    pObj->GetBoundBox(box);

    // Check if object visible in viewport.
    if (!view->IsBoundsVisible(box))
    {
        return;
    }

    if (pObj->HitTestRect(hc))
    {
        stl::push_back_unique(guids, pObj->GetId());
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::SelectObjectInRect(CBaseObject* pObj, CViewport* view, HitContext hc, bool bSelect)
{
    if (!pObj->IsSelectable())
    {
        return;
    }

    AABB box;

    // Retrieve world space bound box.
    pObj->GetBoundBox(box);

    // Check if object visible in viewport.
    if (!view->IsBoundsVisible(box))
    {
        return;
    }

    if (pObj->HitTestRect(hc))
    {
        if (bSelect)
        {
            SelectObject(pObj);
        }
        else
        {
            UnselectObject(pObj);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
namespace
{
    AZStd::vector<AZStd::string> PyGetAllObjects()
    {
        IObjectManager* pObjMgr = GetIEditor()->GetObjectManager();
        CBaseObjectsArray objects;
        pObjMgr->GetObjects(objects);
        int count = pObjMgr->GetObjectCount();
        AZStd::vector<AZStd::string> result;
        for (int i = 0; i < count; ++i)
        {
            result.push_back(objects[i]->GetName().toUtf8().data());
        }

        return result;
    }

    AZStd::vector<AZStd::string> PyGetNamesOfSelectedObjects()
    {
        CSelectionGroup* pSel = GetIEditor()->GetSelection();
        AZStd::vector<AZStd::string> result;
        const int selectionCount = pSel->GetCount();
        result.reserve(selectionCount);

        for (int i = 0; i < selectionCount; i++)
        {
            result.push_back(pSel->GetObject(i)->GetName().toUtf8().data());
        }

        return result;
    }

    void PySelectObject(const char* objName)
    {
        CUndo undo("Select Object");

        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (pObject)
        {
            GetIEditor()->GetObjectManager()->SelectObject(pObject);
        }
    }

    void PyUnselectObjects(const AZStd::vector<AZStd::string>& names)
    {
        CUndo undo("Unselect Objects");

        std::vector<CBaseObject*> pBaseObjects;
        for (int i = 0; i < names.size(); i++)
        {
            if (!GetIEditor()->GetObjectManager()->FindObject(names[i].c_str()))
            {
                throw std::logic_error((QString("\"") + names[i].c_str() + "\" is an invalid entity.").toUtf8().data());
            }
            pBaseObjects.push_back(GetIEditor()->GetObjectManager()->FindObject(names[i].c_str()));
        }

        for (int i = 0; i < pBaseObjects.size(); i++)
        {
            GetIEditor()->GetObjectManager()->UnselectObject(pBaseObjects[i]);
        }
    }

    void PySelectObjects(const AZStd::vector<AZStd::string>& names)
    {
        CUndo undo("Select Objects");
        CBaseObject* pObject;
        for (size_t i = 0; i < names.size(); ++i)
        {
            pObject = GetIEditor()->GetObjectManager()->FindObject(names[i].c_str());
            if (!pObject)
            {
                throw std::logic_error((QString("\"") + names[i].c_str() + "\" is an invalid entity.").toUtf8().data());
            }
            GetIEditor()->GetObjectManager()->SelectObject(pObject);
        }
    }

    bool PyIsObjectHidden(const char* objName)
    {
        CBaseObject* pObject =  GetIEditor()->GetObjectManager()->FindObject(objName);
        if (!pObject)
        {
            throw std::logic_error((QString("\"") + objName + "\" is an invalid object name.").toUtf8().data());
        }
        return pObject->IsHidden();
    }

    void PyHideAllObjects()
    {
        CBaseObjectsArray baseObjects;
        GetIEditor()->GetObjectManager()->GetObjects(baseObjects);

        if (baseObjects.size() <= 0)
        {
            throw std::logic_error("Objects not found.");
        }

        CUndo undo("Hide All Objects");
        for (int i = 0; i < baseObjects.size(); i++)
        {
            GetIEditor()->GetObjectManager()->HideObject(baseObjects[i], true);
        }
    }

    void PyUnHideAllObjects()
    {
        CUndo undo("Unhide All Objects");
        GetIEditor()->GetObjectManager()->UnhideAll();
    }

    void PyHideObject(const char* objName)
    {
        CUndo undo("Hide Object");

        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (pObject)
        {
            GetIEditor()->GetObjectManager()->HideObject(pObject, true);
        }
    }

    void PyUnhideObject(const char* objName)
    {
        CUndo undo("Unhide Object");

        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (pObject)
        {
            GetIEditor()->GetObjectManager()->HideObject(pObject, false);
        }
    }

    void PyFreezeObject(const char* objName)
    {
        CUndo undo("Freeze Object");

        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (pObject)
        {
            GetIEditor()->GetObjectManager()->FreezeObject(pObject, true);
        }
    }

    void PyUnfreezeObject(const char* objName)
    {
        CUndo undo("Unfreeze Object");

        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (pObject)
        {
            GetIEditor()->GetObjectManager()->FreezeObject(pObject, false);
        }
    }

    bool PyIsObjectFrozen(const char* objName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (!pObject)
        {
            throw std::logic_error((QString("\"") + objName + "\" is an invalid object name.").toUtf8().data());
        }
        return pObject->IsFrozen();
    }

    void PyDeleteObject(const char* objName)
    {
        CUndo undo("Delete Object");

        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (pObject)
        {
            GetIEditor()->GetObjectManager()->DeleteObject(pObject);
        }
    }

    int PyClearSelection()
    {
        CUndo undo("Clear Selection");
        return GetIEditor()->GetObjectManager()->ClearSelection();
    }

    void PyDeleteSelected()
    {
        CUndo undo("Delete Selected Object");
        GetIEditor()->GetObjectManager()->DeleteSelection();
    }

    int PyGetNumSelectedObjects()
    {
        if (CSelectionGroup* pGroup = GetIEditor()->GetObjectManager()->GetSelection())
        {
            return pGroup->GetCount();
        }

        return 0;
    }

    AZ::Vector3 PyGetSelectionCenter()
    {
        if (CSelectionGroup* pGroup = GetIEditor()->GetObjectManager()->GetSelection())
        {
            if (pGroup->GetCount() == 0)
            {
                throw std::runtime_error("Nothing selected");
            }

            const Vec3 center = pGroup->GetCenter();
            return AZ::Vector3(center.x, center.y, center.z);
        }

        throw std::runtime_error("Nothing selected");
    }

    AZ::Aabb PyGetSelectionAABB()
    {
        if (CSelectionGroup* pGroup = GetIEditor()->GetObjectManager()->GetSelection())
        {
            if (pGroup->GetCount() == 0)
            {
                throw std::runtime_error("Nothing selected");
            }

            const AABB aabb = pGroup->GetBounds();
            AZ::Aabb result;
            result.Set(
                AZ::Vector3(
                    aabb.min.x,
                    aabb.min.y,
                    aabb.min.z
                ),
                AZ::Vector3(
                    aabb.max.x,
                    aabb.max.y,
                    aabb.max.z
                )
            );
            return result;
        }

        throw std::runtime_error("Nothing selected");
    }

    AZ::Vector3 PyGetObjectPosition(const char* pName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pName);
        if (!pObject)
        {
            throw std::logic_error((QString("\"") + pName + "\" is an invalid object.").toUtf8().data());
        }
        Vec3 position = pObject->GetPos();
        return AZ::Vector3(position.x, position.y, position.z);
    }

    void PySetObjectPosition(const char* pName, float fValueX, float fValueY, float fValueZ)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pName);
        if (!pObject)
        {
            throw std::logic_error((QString("\"") + pName + "\" is an invalid object.").toUtf8().data());
        }
        CUndo undo("Set Object Base Position");
        pObject->SetPos(Vec3(fValueX, fValueY, fValueZ));
    }

    AZ::Vector3 PyGetObjectRotation(const char* pName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pName);
        if (!pObject)
        {
            throw std::logic_error((QString("\"") + pName + "\" is an invalid object.").toUtf8().data());
        }
        Ang3 ang = RAD2DEG(Ang3(pObject->GetRotation()));
        return AZ::Vector3(ang.x, ang.y, ang.z);
    }

    void PySetObjectRotation(const char* pName, float fValueX, float fValueY, float fValueZ)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pName);
        if (!pObject)
        {
            throw std::logic_error((QString("\"") + pName + "\" is an invalid object.").toUtf8().data());
        }
        CUndo undo("Set Object Rotation");
        pObject->SetRotation(Quat(DEG2RAD(Ang3(fValueX, fValueY, fValueZ))));
    }

    AZ::Vector3 PyGetObjectScale(const char* pName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pName);
        if (!pObject)
        {
            throw std::logic_error((QString("\"") + pName + "\" is an invalid object.").toUtf8().data());
        }
        Vec3 scaleVec3 = pObject->GetScale();
        return AZ::Vector3(scaleVec3.x, scaleVec3.y, scaleVec3.z);
    }

    void PySetObjectScale(const char* pName, float fValueX, float fValueY, float fValueZ)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pName);
        if (!pObject)
        {
            throw std::logic_error((QString("\"") + pName + "\" is an invalid object.").toUtf8().data());
        }
        CUndo undo("Set Object Scale");
        pObject->SetScale(Vec3(fValueX, fValueY, fValueZ));
    }

    void PyRenameObject(const char* pOldName, const char* pNewName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pOldName);
        if (!pObject)
        {
            throw std::runtime_error("Could not find object");
        }

        if (strcmp(pNewName, "") == 0 || GetIEditor()->GetObjectManager()->FindObject(pNewName))
        {
            throw std::runtime_error("Invalid object name.");
        }

        CUndo undo("Rename object");
        pObject->SetName(pNewName);
    }
}

namespace AzToolsFramework
{
    void ObjectManagerFuncsHandler::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // this will put these methods into the 'azlmbr.legacy.general' module
            auto addLegacyGeneral = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Legacy/Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "legacy.general");
            };
            addLegacyGeneral(behaviorContext->Method("get_all_objects", PyGetAllObjects, nullptr, "Gets the list of names of all objects in the whole level."));
            addLegacyGeneral(behaviorContext->Method("get_names_of_selected_objects", PyGetNamesOfSelectedObjects, nullptr, "Get the name from selected object/objects."));

            addLegacyGeneral(behaviorContext->Method("select_object", PySelectObject, nullptr, "Selects a specified object."));
            addLegacyGeneral(behaviorContext->Method("unselect_objects", PyUnselectObjects, nullptr, "Unselects a list of objects."));
            addLegacyGeneral(behaviorContext->Method("select_objects", PySelectObjects, nullptr, "Selects a list of objects."));
            addLegacyGeneral(behaviorContext->Method("get_num_selected", PyGetNumSelectedObjects, nullptr, "Returns the number of selected objects."));
            addLegacyGeneral(behaviorContext->Method("clear_selection", PyClearSelection, nullptr, "Clears selection."));

            addLegacyGeneral(behaviorContext->Method("get_selection_center", PyGetSelectionCenter, nullptr, "Returns the center point of the selection group."));
            addLegacyGeneral(behaviorContext->Method("get_selection_aabb", PyGetSelectionAABB, nullptr, "Returns the aabb of the selection group."));

            addLegacyGeneral(behaviorContext->Method("hide_object", PyHideObject, nullptr, "Hides a specified object."));
            addLegacyGeneral(behaviorContext->Method("is_object_hidden", PyIsObjectHidden, nullptr, "Checks if object is hidden and returns a bool value."));
            addLegacyGeneral(behaviorContext->Method("unhide_object", PyUnhideObject, nullptr, "Unhides a specified object."));
            addLegacyGeneral(behaviorContext->Method("hide_all_objects", PyHideAllObjects, nullptr, "Hides all objects."));
            addLegacyGeneral(behaviorContext->Method("unhide_all_objects", PyUnHideAllObjects, nullptr, "Unhides all objects."));

            addLegacyGeneral(behaviorContext->Method("freeze_object", PyFreezeObject, nullptr, "Freezes a specified object."));
            addLegacyGeneral(behaviorContext->Method("is_object_frozen", PyIsObjectFrozen, nullptr, "Checks if object is frozen and returns a bool value."));
            addLegacyGeneral(behaviorContext->Method("unfreeze_object", PyUnfreezeObject, nullptr, "Unfreezes a specified object."));

            addLegacyGeneral(behaviorContext->Method("delete_object", PyDeleteObject, nullptr, "Deletes a specified object."));
            addLegacyGeneral(behaviorContext->Method("delete_selected", PyDeleteSelected, nullptr, "Deletes selected object(s)."));

            addLegacyGeneral(behaviorContext->Method("get_position", PyGetObjectPosition, nullptr, "Gets the position of an object."));
            addLegacyGeneral(behaviorContext->Method("set_position", PySetObjectPosition, nullptr, "Sets the position of an object."));

            addLegacyGeneral(behaviorContext->Method("get_rotation", PyGetObjectRotation, nullptr, "Gets the rotation of an object."));
            addLegacyGeneral(behaviorContext->Method("set_rotation", PySetObjectRotation, nullptr, "Sets the rotation of an object."));

            addLegacyGeneral(behaviorContext->Method("get_scale", PyGetObjectScale, nullptr, "Gets the scale of an object."));
            addLegacyGeneral(behaviorContext->Method("set_scale", PySetObjectScale, nullptr, "Sets the scale of an object."));

            addLegacyGeneral(behaviorContext->Method("rename_object", PyRenameObject, nullptr, "Renames object with oldObjectName to newObjectName."));


        }
    }
} // namespace AzToolsFramework
