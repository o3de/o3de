/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : ObjectManager definition.
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
    }

    ~CObjectManagerLevelIsExporting()
    {
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

    CBaseObject* NewObject(CObjectClassDesc* cls, CBaseObject* prev = 0, const QString& file = "", const char* newObjectName = nullptr) override;
    CBaseObject* NewObject(const QString& typeName, CBaseObject* prev = 0, const QString& file = "", const char* newEntityName = nullptr) override;

    void    DeleteObject(CBaseObject* obj) override;
    void    DeleteSelection(CSelectionGroup* pSelection) override;
    void    DeleteAllObjects() override;

    //! Get number of objects manager by ObjectManager (not contain sub objects of groups).
    int     GetObjectCount() const override;

    //! Get array of objects, managed by manager (not contain sub objects of groups).
    //! @param layer if 0 get objects for all layers, or layer to get objects from.
    void GetObjects(CBaseObjectsArray& objects) const override;

    //! Display objects on display context.
    void    Display(DisplayContext& dc) override;

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
    // Object Selection.
    //////////////////////////////////////////////////////////////////////////
    bool    SelectObject(CBaseObject* obj, bool bUseMask = true) override;
    void    UnselectObject(CBaseObject* obj) override;

    //! Clear default selection set.
    //! @Return number of objects removed from selection.
    int ClearSelection() override;

    //! Get current selection.
    CSelectionGroup*    GetSelection() const override { return m_currSelection; };

    bool IsObjectDeletionAllowed(CBaseObject* pObject);

    //! Delete all objects in selection group.
    void DeleteSelection() override;

    //! Generates uniq name base on type name of object.
    QString GenerateUniqueObjectName(const QString& typeName) override;
    //! Register object name in object manager, needed for generating uniq names.
    void RegisterObjectName(const QString& name) override;

    //! Register XML template of runtime class.
    void    RegisterClassTemplate(const XmlNodeRef& templ);
    //! Load class templates for specified directory,
    void    LoadClassTemplates(const QString& path);

    //! Registers the ObjectManager's console variables.
    void RegisterCVars();

    //! Find object class by name.
    CObjectClassDesc* FindClass(const QString& className) override;

    bool AddObject(CBaseObject* obj);
    void RemoveObject(CBaseObject* obj);
    void ChangeObjectId(REFGUID oldId, REFGUID newId) override;

    //////////////////////////////////////////////////////////////////////////
    //! Get access to gizmo manager.
    IGizmoManager* GetGizmoManager() override;

    //////////////////////////////////////////////////////////////////////////
    //! Invalidate visibily settings of objects.
    void InvalidateVisibleList() override;

    //////////////////////////////////////////////////////////////////////////
    // Gathers all resources used by all objects.
    void GatherUsedResources(CUsedResources& resources) override;

    bool IsLightClass(CBaseObject* pObject) override;

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

private:
    typedef AZStd::map<GUID, CBaseObjectPtr, guid_less_predicate> Objects;
    Objects m_objects;
    typedef AZStd::unordered_map<AZ::u32, CBaseObjectPtr> ObjectsByNameCrc;
    ObjectsByNameCrc m_objectsByName;

    //! Array of currently visible objects.
    TBaseObjects m_visibleObjects;

    // this number changes whenever visibility is invalidated.  Viewports can use it to keep track of whether they need to recompute object
    // visibility.
    unsigned int m_visibilitySerialNumber = 1;
    unsigned int m_lastComputedVisibility = 0; // when the object manager itself last updated visibility (since it also has a cache)
    int m_lastHideMask = 0;

    //////////////////////////////////////////////////////////////////////////
    // Selection.
    //! Current selection group.
    CSelectionGroup* m_currSelection;
    bool m_bSelectionChanged;

    // True while performing a select or deselect operation on more than one object.
    // Prevents individual undo/redo commands for every object, allowing bulk undo/redo
    bool m_processingBulkSelect = false;

    //! Default selection.
    CSelectionGroup m_defaultSelection;

    // Object manager also handles Gizmo manager.
    CGizmoManager* m_gizmoManager;

    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Numbering for names.
    //////////////////////////////////////////////////////////////////////////
    typedef std::map<QString, std::set<uint16>, stl::less_stricmp<QString> > NameNumbersMap;
    NameNumbersMap m_nameNumbersMap;

    bool m_bExiting;

    std::unordered_set<CEntityObject*> m_animatedAttachedEntities;

    bool m_isUpdateVisibilityList;

    uint64 m_currentHideCount;

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

