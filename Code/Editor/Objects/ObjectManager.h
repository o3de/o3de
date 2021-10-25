/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : ObjectManager definition.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_OBJECTMANAGER_H
#define CRYINCLUDE_EDITOR_OBJECTS_OBJECTMANAGER_H
#pragma once

#include "IObjectManager.h"
#include "BaseObject.h"
#include "SelectionGroup.h"
#include "ObjectManagerEventBus.h"

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>
#include <Include/SandboxAPI.h>

// forward declarations.
class CGizmoManager;
class CEntityObject;
class CObjectArchive;
class CObjectClassDesc;
class CWaitProgress;

enum class ImageRotationDegrees;

//////////////////////////////////////////////////////////////////////////
// Helper class to signal when we are exporting a level to game
//////////////////////////////////////////////////////////////////////////
class CObjectManagerLevelIsExporting
{
public:
    CObjectManagerLevelIsExporting()
    {
        AZ::ObjectManagerEventBus::Broadcast(&AZ::ObjectManagerEventBus::Events::OnExportingStarting);
        GetIEditor()->GetObjectManager()->SetExportingLevel(true);
    }

    ~CObjectManagerLevelIsExporting()
    {
        GetIEditor()->GetObjectManager()->SetExportingLevel(false);
        AZ::ObjectManagerEventBus::Broadcast(&AZ::ObjectManagerEventBus::Events::OnExportingFinished);
    }
};

/*!
 *  CObjectManager is a singleton object that
 *  manages global set of objects in level.
 */
class CObjectManager
    : public IObjectManager
{
public:
    //! Selection functor callback.
    //! Callback function must return a boolean value.
    //! Return true if selection should proceed, or false to abort object selection.
    CObjectManager();
    ~CObjectManager();

    void RegisterObjectClasses();

    CBaseObject* NewObject(CObjectClassDesc* cls, CBaseObject* prev = 0, const QString& file = "", const char* newObjectName = nullptr) override;
    CBaseObject* NewObject(const QString& typeName, CBaseObject* prev = 0, const QString& file = "", const char* newEntityName = nullptr) override;

    void    DeleteObject(CBaseObject* obj) override;
    void    DeleteSelection(CSelectionGroup* pSelection) override;
    void    DeleteAllObjects() override;
    CBaseObject* CloneObject(CBaseObject* obj) override;

    void BeginEditParams(CBaseObject* obj, int flags) override;
    void EndEditParams(int flags = 0) override;
    // Hides all transform manipulators.
    void HideTransformManipulators();

    //! Get number of objects manager by ObjectManager (not contain sub objects of groups).
    int     GetObjectCount() const override;

    //! Get array of objects, managed by manager (not contain sub objects of groups).
    //! @param layer if 0 get objects for all layers, or layer to get objects from.
    void GetObjects(CBaseObjectsArray& objects) const override;

    //! Get array of objects that pass the filter.
    //! @param filter The filter functor, return true if you want to get the certain obj, return false if want to skip it.
    void    GetObjects(CBaseObjectsArray& objects, BaseObjectFilterFunctor const& filter) const override;

    //! Update objects.
    void    Update();

    //! Display objects on display context.
    void    Display(DisplayContext& dc) override;

    //! Called when selecting without selection helpers - this is needed since
    //! the visible object cache is normally not updated when not displaying helpers.
    void ForceUpdateVisibleObjectCache(DisplayContext& dc) override;

    //! Check intersection with objects.
    //! Find intersection with nearest to ray origin object hit by ray.
    //! If distance tollerance is specified certain relaxation applied on collision test.
    //! @return true if hit any object, and fills hitInfo structure.
    bool HitTest(HitContext& hitInfo) override;

    //! Check intersection with an object.
    //! @return true if hit, and fills hitInfo structure.
    bool HitTestObject(CBaseObject* obj, HitContext& hc) override;

    //! Send event to all objects.
    //! Will cause OnEvent handler to be called on all objects.
    void    SendEvent(ObjectEvent event) override;

    //! Send event to all objects within given bounding box.
    //! Will cause OnEvent handler to be called on objects within bounding box.
    void    SendEvent(ObjectEvent event, const AABB& bounds) override;

    //////////////////////////////////////////////////////////////////////////
    //! Find object by ID.
    CBaseObject* FindObject(REFGUID guid) const override;
    //////////////////////////////////////////////////////////////////////////
    //! Find object by name.
    CBaseObject* FindObject(const QString& sName) const override;
    //////////////////////////////////////////////////////////////////////////
    //! Find objects of given type.
    void FindObjectsOfType(const QMetaObject* pClass, std::vector<CBaseObject*>& result) override;
    void FindObjectsOfType(ObjectType type, std::vector<CBaseObject*>& result) override;
    //////////////////////////////////////////////////////////////////////////
    //! Find objects which intersect with a given AABB.
    void FindObjectsInAABB(const AABB& aabb, std::vector<CBaseObject*>& result) const override;

    //////////////////////////////////////////////////////////////////////////
    // Operations on objects.
    //////////////////////////////////////////////////////////////////////////
    //! Makes object visible or invisible.
    void HideObject(CBaseObject* obj, bool hide) override;
    //! Shows the last hidden object based on hidden ID
    void ShowLastHiddenObject() override;
    //! Freeze object, making it unselectable.
    void FreezeObject(CBaseObject* obj, bool freeze) override;
    //! Unhide all hidden objects.
    void UnhideAll() override;
    //! Unfreeze all frozen objects.
    void UnfreezeAll() override;

    //////////////////////////////////////////////////////////////////////////
    // Object Selection.
    //////////////////////////////////////////////////////////////////////////
    bool    SelectObject(CBaseObject* obj, bool bUseMask = true) override;
    void    UnselectObject(CBaseObject* obj) override;

    //! Select objects within specified distance from given position.
    //! Return number of selected objects.
    int SelectObjects(const AABB& box, bool bUnselect = false) override;

    void SelectEntities(std::set<CEntityObject*>& s) override;

    int MoveObjects(const AABB& box, const Vec3& offset, ImageRotationDegrees rotation, bool bIsCopy = false) override;

    //! Selects/Unselects all objects within 2d rectangle in given viewport.
    void SelectObjectsInRect(CViewport* view, const QRect& rect, bool bSelect) override;
    void FindObjectsInRect(CViewport* view, const QRect& rect, std::vector<GUID>& guids) override;

    //! Clear default selection set.
    //! @Return number of objects removed from selection.
    int ClearSelection() override;

    //! Deselect all current selected objects and selects object that were unselected.
    //! @Return number of selected objects.
    int InvertSelection() override;

    //! Get current selection.
    CSelectionGroup*    GetSelection() const override { return m_currSelection; };
    //! Get named selection.
    CSelectionGroup*    GetSelection(const QString& name) const override;
    // Get selection group names
    void GetNameSelectionStrings(QStringList& names) override;
    //! Change name of current selection group.
    //! And store it in list.
    void    NameSelection(const QString& name) override;
    //! Set one of name selections as current selection.
    void    SetSelection(const QString& name) override;
    void    RemoveSelection(const QString& name) override;

    bool IsObjectDeletionAllowed(CBaseObject* pObject);

    //! Delete all objects in selection group.
    void DeleteSelection() override;

    uint32  ForceID() const override{return m_ForceID; }
    void    ForceID(uint32 FID) override{m_ForceID = FID; }

    //! Generates uniq name base on type name of object.
    QString GenerateUniqueObjectName(const QString& typeName) override;
    //! Register object name in object manager, needed for generating uniq names.
    void RegisterObjectName(const QString& name) override;
    //! Decrease name number and remove if it was last in object manager, needed for generating uniq names.
    void UpdateRegisterObjectName(const QString& name);
    //! Enable/Disable generating of unique object names (Enabled by default).
    //! Return previous value.
    bool EnableUniqObjectNames(bool bEnable) override;

    //! Register XML template of runtime class.
    void    RegisterClassTemplate(const XmlNodeRef& templ);
    //! Load class templates for specified directory,
    void    LoadClassTemplates(const QString& path);

    //! Registers the ObjectManager's console variables.
    void RegisterCVars();

    //! Find object class by name.
    CObjectClassDesc* FindClass(const QString& className) override;
    void    GetClassCategories(QStringList& categories) override;
    void    GetClassCategoryToolClassNamePairs(std::vector< std::pair<QString, QString> >& categoryToolClassNamePairs) override;
    void    GetClassTypes(const QString& category, QStringList& types) override;

    //! Export objects to xml.
    //! When onlyShared is true ony objects with shared flags exported, overwise only not shared object exported.
    void    Export(const QString& levelPath, XmlNodeRef& rootNode, bool onlyShared) override;
    void    ExportEntities(XmlNodeRef& rootNode) override;

    //! Serialize Objects in manager to specified XML Node.
    //! @param flags Can be one of SerializeFlags.
    void    Serialize(XmlNodeRef& rootNode, bool bLoading, int flags = SERIALIZE_ALL) override;

    void SerializeNameSelection(XmlNodeRef& rootNode, bool bLoading) override;

    //! Load objects from object archive.
    //! @param bSelect if set newly loaded object will be selected.
    void LoadObjects(CObjectArchive& ar, bool bSelect) override;

    //! Delete from Object manager all objects without SHARED flag.
    void    DeleteNotSharedObjects();
    //! Delete from Object manager all objects with SHARED flag.
    void    DeleteSharedObjects();

    bool AddObject(CBaseObject* obj);
    void RemoveObject(CBaseObject* obj);
    void ChangeObjectId(REFGUID oldId, REFGUID newId) override;
    bool IsDuplicateObjectName(const QString& newName) const override
    {
        return FindObject(newName) ? true : false;
    }
    void ShowDuplicationMsgWarning(CBaseObject* obj, const QString& newName, bool bShowMsgBox) const override;
    void ChangeObjectName(CBaseObject* obj, const QString& newName) override;

    //! Convert object of one type to object of another type.
    //! Original object is deleted.
    bool ConvertToType(CBaseObject* pObject, const QString& typeName) override;

    //! Set new selection callback.
    //! @return previous selection callback.
    IObjectSelectCallback* SetSelectCallback(IObjectSelectCallback* callback) override;

    // Enables/Disables creating of game objects.
    void SetCreateGameObject(bool enable) override { m_createGameObjects = enable; };
    //! Return true if objects loaded from xml should immidiatly create game objects associated with them.
    bool IsCreateGameObjects() const override { return m_createGameObjects; };

    //////////////////////////////////////////////////////////////////////////
    //! Get access to gizmo manager.
    IGizmoManager* GetGizmoManager() override;

    //////////////////////////////////////////////////////////////////////////
    //! Invalidate visibily settings of objects.
    void InvalidateVisibleList() override;

    //////////////////////////////////////////////////////////////////////////
    // ObjectManager notification Callbacks.
    //////////////////////////////////////////////////////////////////////////
    void AddObjectEventListener(EventListener* listener) override;
    void RemoveObjectEventListener(EventListener* listener) override;

    //////////////////////////////////////////////////////////////////////////
    // Used to indicate starting and ending of objects loading.
    //////////////////////////////////////////////////////////////////////////
    void StartObjectsLoading(int numObjects) override;
    void EndObjectsLoading() override;

    //////////////////////////////////////////////////////////////////////////
    // Gathers all resources used by all objects.
    void GatherUsedResources(CUsedResources& resources) override;

    bool IsLightClass(CBaseObject* pObject) override;

    virtual void FindAndRenameProperty2(const char* property2Name, const QString& oldValue, const QString& newValue) override;
    virtual void FindAndRenameProperty2If(const char* property2Name, const QString& oldValue, const QString& newValue, const char* otherProperty2Name, const QString& otherValue) override;

    bool IsReloading() const override { return m_bInReloading; }
    void SetSkipUpdate(bool bSkipUpdate) override { m_bSkipObjectUpdate = bSkipUpdate; }

    void SetExportingLevel(bool bExporting) override { m_bLevelExporting = bExporting; }
    bool IsExportingLevelInprogress() const override { return m_bLevelExporting; }

    int GetAxisHelperHitRadius() const override { return m_axisHelperHitRadius; }

private:
    friend CObjectArchive;
    friend class CBaseObject;
    /** Creates and serialize object from xml node.
    @param objectNode Xml node to serialize object info from.
    @param pUndoObject Pointer to deleted object for undo.
    */
    CBaseObject* NewObject(CObjectArchive& archive, CBaseObject* pUndoObject, bool bMakeNewId) override;

    //! Update visibility of all objects.
    void UpdateVisibilityList();
    //! Get array of all objects in manager.
    void GetAllObjects(TBaseObjects& objects) const;

    void UnselectCurrent();
    void SelectCurrent();
    void SetObjectSelected(CBaseObject* pObject, bool bSelect);

    // Recursive functions potentially taking into child objects into account
    void SelectObjectInRect(CBaseObject* pObj, CViewport* view, HitContext hc, bool bSelect);
    void HitTestObjectAgainstRect(CBaseObject* pObj, CViewport* view, HitContext hc, std::vector<GUID>& guids);

    void SaveRegistry();
    void LoadRegistry();

    void NotifyObjectListeners(CBaseObject* pObject, CBaseObject::EObjectListenerEvent event);

    void FindDisplayableObjects(DisplayContext& dc, bool bDisplay);

private:
    typedef std::map<GUID, CBaseObjectPtr, guid_less_predicate> Objects;
    Objects m_objects;
    typedef std::unordered_map<AZ::u32, CBaseObjectPtr> ObjectsByNameCrc;
    ObjectsByNameCrc m_objectsByName;

    typedef std::map<QString, CSelectionGroup*> TNameSelectionMap;
    TNameSelectionMap m_selections;

    //! Used for forcing IDs of "GetEditorObjectID" of PreFabs, as they used to have random IDs on each load
    uint32  m_ForceID;

    //! Array of currently visible objects.
    TBaseObjects m_visibleObjects;

    // this number changes whenever visibility is invalidated.  Viewports can use it to keep track of whether they need to recompute object
    // visibility.
    unsigned int m_visibilitySerialNumber = 1;
    unsigned int m_lastComputedVisibility = 0; // when the object manager itself last updated visibility (since it also has a cache)
    int m_lastHideMask = 0;

    float m_maxObjectViewDistRatio;

    //////////////////////////////////////////////////////////////////////////
    // Selection.
    //! Current selection group.
    CSelectionGroup* m_currSelection;
    int m_nLastSelCount;
    bool m_bSelectionChanged;
    IObjectSelectCallback* m_selectCallback;
    bool m_bLoadingObjects;

    // True while performing a select or deselect operation on more than one object.
    // Prevents individual undo/redo commands for every object, allowing bulk undo/redo
    bool m_processingBulkSelect = false;

    //! Default selection.
    CSelectionGroup m_defaultSelection;

    CBaseObjectPtr m_currEditObject;
    bool m_bSingleSelection;

    bool m_createGameObjects;
    bool m_bGenUniqObjectNames;

    // Object manager also handles Gizmo manager.
    CGizmoManager* m_gizmoManager;

    //////////////////////////////////////////////////////////////////////////
    // Loading progress.
    CWaitProgress* m_pLoadProgress;
    int m_loadedObjects;
    int m_totalObjectsToLoad;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Numbering for names.
    //////////////////////////////////////////////////////////////////////////
    typedef std::map<QString, std::set<uint16>, stl::less_stricmp<QString> > NameNumbersMap;
    NameNumbersMap m_nameNumbersMap;

    //////////////////////////////////////////////////////////////////////////
    // Listeners.
    std::list<EventListener*> m_objectEventListeners;

    bool m_bExiting;

    std::unordered_set<CEntityObject*> m_animatedAttachedEntities;

    bool m_isUpdateVisibilityList;

    uint64 m_currentHideCount;

    bool m_bInReloading;
    bool m_bSkipObjectUpdate;
    bool m_bLevelExporting;

    int m_axisHelperHitRadius = 20;
};

namespace AzToolsFramework
{
    //! A component to reflect scriptable commands for the Editor
    class ObjectManagerFuncsHandler
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(ObjectManagerFuncsHandler, "{D79B69EE-A2CC-43C0-AA5C-47DCFCCBC955}")

        SANDBOX_API static void Reflect(AZ::ReflectContext* context);

        // AZ::Component ...
        void Activate() override {}
        void Deactivate() override {}
    };

} // namespace AzToolsFramework

#endif // CRYINCLUDE_EDITOR_OBJECTS_OBJECTMANAGER_H
