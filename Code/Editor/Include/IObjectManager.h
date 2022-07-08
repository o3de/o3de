/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
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
class CBaseObject;
class XmlNodeRef;

#include "ObjectEvent.h"

//////////////////////////////////////////////////////////////////////////
typedef std::vector<CBaseObject*> CBaseObjectsArray;
typedef std::pair< bool(CALLBACK*)(CBaseObject const&, void*), void* > BaseObjectFilterFunctor;

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

    //! Get number of objects manager by ObjectManager (not contain sub objects of groups).
    virtual int GetObjectCount() const = 0;

    //! Get array of objects, managed by manager (not contain sub objects of groups).
    //! @param layer if 0 get objects for all layers, or layer to get objects from.
    virtual void GetObjects(CBaseObjectsArray& objects) const = 0;

    //! Display objects on specified display context.
    virtual void Display(DisplayContext& dc) = 0;

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
    // Object Selection.
    //////////////////////////////////////////////////////////////////////////
    virtual bool    SelectObject(CBaseObject* obj, bool bUseMask = true) = 0;
    virtual void    UnselectObject(CBaseObject* obj) = 0;

    //! Clear default selection set.
    //! @Return number of objects removed from selection.
    virtual int ClearSelection() = 0;

    //! Get current selection.
    virtual CSelectionGroup*    GetSelection() const = 0;

    //! Delete all objects in current selection group.
    virtual void DeleteSelection() = 0;

    //! Generates uniq name base on type name of object.
    virtual QString GenerateUniqueObjectName(const QString& typeName) = 0;
    //! Register object name in object manager, needed for generating uniq names.
    virtual void RegisterObjectName(const QString& name) = 0;

    //! Find object class by name.
    virtual CObjectClassDesc* FindClass(const QString& className) = 0;

    virtual void ChangeObjectId(REFGUID oldId, REFGUID newId) = 0;

    virtual IGizmoManager* GetGizmoManager() = 0;

    //////////////////////////////////////////////////////////////////////////
    //! Invalidate visibily settings of objects.
    virtual void InvalidateVisibleList() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Gathers all resources used by all objects.
    virtual void GatherUsedResources(CUsedResources& resources) = 0;

    virtual bool IsLightClass(CBaseObject* pObject) = 0;
};
