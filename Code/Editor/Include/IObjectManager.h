/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_INCLUDE_IOBJECTMANAGER_H
#define CRYINCLUDE_EDITOR_INCLUDE_IOBJECTMANAGER_H
#pragma once

#include <AzCore/PlatformIncl.h>
#include <AzCore/Math/Guid.h>
#include <CryCommon/platform.h>
#include <CryCommon/Cry_Geo.h>
#include <set>

// forward declarations.
class CEntityObject;
struct DisplayContext;
struct IGizmoManager;
class CTrackViewAnimNode;
class CUsedResources;
class CSelectionGroup;
class CObjectClassDesc;
class CObjectArchive;
class CViewport;
struct HitContext;
enum class ImageRotationDegrees;
struct IStatObj;
class CBaseObject;
class XmlNodeRef;

#include "ObjectEvent.h"

enum SerializeFlags
{
    SERIALIZE_ALL = 0,
    SERIALIZE_ONLY_SHARED = 1,
    SERIALIZE_ONLY_NOTSHARED = 2,
};

//////////////////////////////////////////////////////////////////////////
typedef std::vector<CBaseObject*> CBaseObjectsArray;
typedef std::pair< bool(CALLBACK*)(CBaseObject const&, void*), void* > BaseObjectFilterFunctor;

struct IObjectSelectCallback
{
    //! Called when object is selected.
    //! Return true if selection should proceed, or false to abort object selection.
    virtual bool OnSelectObject(CBaseObject* obj) = 0;
    //! Return true if object can be selected.
    virtual bool CanSelectObject(CBaseObject* obj) = 0;
};

//////////////////////////////////////////////////////////////////////////
//
// Interface to access editor objects scene graph.
//
//////////////////////////////////////////////////////////////////////////
struct IObjectManager
{
public:
    virtual ~IObjectManager() = default;

    //! This callback will be called on response to object event.
    struct EventListener
    {
        virtual void OnObjectEvent(CBaseObject*, int) = 0;
    };

    virtual CBaseObject* NewObject(CObjectClassDesc* cls, CBaseObject* prev = 0, const QString& file = "", const char* newObjectName = nullptr) = 0;
    virtual CBaseObject* NewObject(const QString& typeName, CBaseObject* prev = 0, const QString& file = "", const char* newObjectName = nullptr) = 0;
    virtual CBaseObject* NewObject(CObjectArchive& archive, CBaseObject* pUndoObject = 0, bool bMakeNewId = false) = 0;

    virtual void DeleteObject(CBaseObject* obj) = 0;
    virtual void DeleteSelection(CSelectionGroup* pSelection) = 0;
    virtual void DeleteAllObjects() = 0;
    virtual CBaseObject* CloneObject(CBaseObject* obj) = 0;

    virtual void BeginEditParams(CBaseObject* obj, int flags) = 0;
    virtual void EndEditParams(int flags = 0) = 0;

    //! Get number of objects manager by ObjectManager (not contain sub objects of groups).
    virtual int GetObjectCount() const = 0;

    //! Get array of objects, managed by manager (not contain sub objects of groups).
    //! @param layer if 0 get objects for all layers, or layer to get objects from.
    virtual void GetObjects(CBaseObjectsArray& objects) const = 0;

    //! Get array of objects that pass the filter.
    //! @param filter The filter functor, return true if you want to get the certain obj, return false if want to skip it.
    virtual void GetObjects(CBaseObjectsArray& objects, BaseObjectFilterFunctor const& filter) const = 0;

    //! Display objects on specified display context.
    virtual void Display(DisplayContext& dc) = 0;

    //! Called when selecting without selection helpers - this is needed since
    //! the visible object cache is normally not updated when not displaying helpers.
    virtual void ForceUpdateVisibleObjectCache(DisplayContext& dc) = 0;

    //! Check intersection with objects.
    //! Find intersection with nearest to ray origin object hit by ray.
    //! If distance tollerance is specified certain relaxation applied on collision test.
    //! @return true if hit any object, and fills hitInfo structure.
    virtual bool HitTest(HitContext& hitInfo) = 0;

    //! Check intersection with an object.
    //! @return true if hit, and fills hitInfo structure.
    virtual bool HitTestObject(CBaseObject* obj, HitContext& hc) = 0;

    //! Gets a radius to be used for hit tests on the axis helpers, like the transform gizmo.
    //! @return the axis helper hit radius.
    virtual int GetAxisHelperHitRadius() const = 0;

    //! Send event to all objects.
    //! Will cause OnEvent handler to be called on all objects.
    virtual void SendEvent(ObjectEvent event) = 0;

    //! Send event to all objects within given bounding box.
    //! Will cause OnEvent handler to be called on objects within bounding box.
    virtual void SendEvent(ObjectEvent event, const AABB& bounds) = 0;

    //////////////////////////////////////////////////////////////////////////
    //! Find object by ID.
    virtual CBaseObject* FindObject(REFGUID guid) const = 0;
    //////////////////////////////////////////////////////////////////////////
    //! Find object by name.
    virtual CBaseObject* FindObject(const QString& sName) const = 0;
    //////////////////////////////////////////////////////////////////////////
    //! Find objects of given type.
    virtual void FindObjectsOfType(const QMetaObject* pClass, std::vector<CBaseObject*>& result) = 0;
    virtual void FindObjectsOfType(ObjectType type, std::vector<CBaseObject*>& result) = 0;
    //////////////////////////////////////////////////////////////////////////
    //! Find objects which intersect with a given AABB.
    virtual void FindObjectsInAABB(const AABB& aabb, std::vector<CBaseObject*>& result) const = 0;

    //////////////////////////////////////////////////////////////////////////
    // Operations on objects.
    //////////////////////////////////////////////////////////////////////////
    //! Makes object visible or invisible.
    virtual void HideObject(CBaseObject* obj, bool hide) = 0;
    //! Shows the last hidden object based on hidden ID
    virtual void ShowLastHiddenObject() = 0;
    //! Freeze object, making it unselectable.
    virtual void FreezeObject(CBaseObject* obj, bool freeze) = 0;
    //! Unhide all hidden objects.
    virtual void UnhideAll() = 0;
    //! Unfreeze all frozen objects.
    virtual void UnfreezeAll() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Object Selection.
    //////////////////////////////////////////////////////////////////////////
    virtual bool    SelectObject(CBaseObject* obj, bool bUseMask = true) = 0;
    virtual void    UnselectObject(CBaseObject* obj) = 0;

    //! Select objects within specified distance from given position.
    //! Return number of selected objects.
    virtual int SelectObjects(const AABB& box, bool bUnselect = false) = 0;

    virtual void SelectEntities(std::set<CEntityObject*>& s) = 0;

    virtual int MoveObjects(const AABB& box, const Vec3& offset, ImageRotationDegrees rotation, bool bIsCopy = false) = 0;

    //! Selects/Unselects all objects within 2d rectangle in given viewport.
    virtual void SelectObjectsInRect(CViewport* view, const QRect& rect, bool bSelect) = 0;
    virtual void FindObjectsInRect(CViewport* view, const QRect& rect, std::vector<GUID>& guids) = 0;

    //! Clear default selection set.
    //! @Return number of objects removed from selection.
    virtual int ClearSelection() = 0;

    //! Deselect all current selected objects and selects object that were unselected.
    //! @Return number of selected objects.
    virtual int InvertSelection() = 0;

    //! Get current selection.
    virtual CSelectionGroup*    GetSelection() const = 0;
    //! Get named selection.
    virtual CSelectionGroup*    GetSelection(const QString& name) const = 0;
    // Get selection group names
    virtual void GetNameSelectionStrings(QStringList& names) = 0;
    //! Change name of current selection group.
    //! And store it in list.
    virtual void    NameSelection(const QString& name) = 0;
    //! Set one of name selections as current selection.
    virtual void    SetSelection(const QString& name) = 0;
    //! Removes one of named selections.
    virtual void    RemoveSelection(const QString& name) = 0;

    //! Delete all objects in current selection group.
    virtual void DeleteSelection() = 0;

    //! Generates uniq name base on type name of object.
    virtual QString GenerateUniqueObjectName(const QString& typeName) = 0;
    //! Register object name in object manager, needed for generating uniq names.
    virtual void RegisterObjectName(const QString& name) = 0;
    //! Enable/Disable generating of unique object names (Enabled by default).
    //! Return previous value.
    virtual bool EnableUniqObjectNames(bool bEnable) = 0;

    //! Find object class by name.
    virtual CObjectClassDesc* FindClass(const QString& className) = 0;
    virtual void GetClassCategories(QStringList& categories) = 0;
    virtual void GetClassCategoryToolClassNamePairs(std::vector< std::pair<QString, QString> >& categoryToolClassNamePairs) = 0;
    virtual void GetClassTypes(const QString& category, QStringList& types) = 0;

    //! Export objects to xml.
    //! When onlyShared is true ony objects with shared flags exported, overwise only not shared object exported.
    virtual void Export(const QString& levelPath, XmlNodeRef& rootNode, bool onlyShared) = 0;
    //! Export only entities to xml.
    virtual void    ExportEntities(XmlNodeRef& rootNode) = 0;

    //! Serialize Objects in manager to specified XML Node.
    //! @param flags Can be one of SerializeFlags.
    virtual void Serialize(XmlNodeRef& rootNode, bool bLoading, int flags = SERIALIZE_ALL) = 0;
    virtual void SerializeNameSelection(XmlNodeRef& rootNode, bool bLoading) = 0;

    //! Load objects from object archive.
    //! @param bSelect if set newly loaded object will be selected.
    virtual void LoadObjects(CObjectArchive& ar, bool bSelect) = 0;

    virtual void ChangeObjectId(REFGUID oldId, REFGUID newId) = 0;
    virtual bool IsDuplicateObjectName(const QString& newName) const = 0;
    virtual void ShowDuplicationMsgWarning(CBaseObject* obj, const QString& newName, bool bShowMsgBox) const = 0;
    virtual void ChangeObjectName(CBaseObject* obj, const QString& newName) = 0;

    //! while loading PreFabs we need to force this IDs
    //! to force always the same IDs, on each load.
    //! needed for RAM-maps assignments
    virtual uint32  ForceID() const  =   0;
    virtual void    ForceID(uint32 FID) =   0;

    //! Convert object of one type to object of another type.
    //! Original object is deleted.
    virtual bool ConvertToType(CBaseObject* pObject, const QString& typeName) = 0;

    //! Set new selection callback.
    //! @return previous selection callback.
    virtual IObjectSelectCallback* SetSelectCallback(IObjectSelectCallback* callback) = 0;

    // Enables/Disables creating of game objects.
    virtual void SetCreateGameObject(bool enable) = 0;
    //! Return true if objects loaded from xml should immidiatly create game objects associated with them.
    virtual bool IsCreateGameObjects() const = 0;

    virtual IGizmoManager* GetGizmoManager() = 0;

    //////////////////////////////////////////////////////////////////////////
    //! Invalidate visibily settings of objects.
    virtual void InvalidateVisibleList() = 0;

    //////////////////////////////////////////////////////////////////////////
    // ObjectManager notification Callbacks.
    //////////////////////////////////////////////////////////////////////////
    virtual void AddObjectEventListener(EventListener* listener) = 0;
    virtual void RemoveObjectEventListener(EventListener* listener) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Used to indicate starting and ending of objects loading.
    //////////////////////////////////////////////////////////////////////////
    virtual void StartObjectsLoading(int numObjects) = 0;
    virtual void EndObjectsLoading() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Gathers all resources used by all objects.
    virtual void GatherUsedResources(CUsedResources& resources) = 0;

    virtual bool IsLightClass(CBaseObject* pObject) = 0;

    virtual void FindAndRenameProperty2(const char* property2Name, const QString& oldValue, const QString& newValue) = 0;
    virtual void FindAndRenameProperty2If(const char* property2Name, const QString& oldValue, const QString& newValue, const char* otherProperty2Name, const QString& otherValue) = 0;

    virtual bool IsReloading() const = 0;

    // Set bSkipUpdate to true if you want to skip update objects on the idle loop.
    virtual void SetSkipUpdate(bool bSkipUpdate) = 0;

    virtual void SetExportingLevel(bool bExporting) = 0;
    virtual bool IsExportingLevelInprogress() const = 0;
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_IOBJECTMANAGER_H
