/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : CSelection group definition.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_SELECTIONGROUP_H
#define CRYINCLUDE_EDITOR_OBJECTS_SELECTIONGROUP_H
#pragma once


class CBaseObject;

#include "ObjectEvent.h"
#include "Objects/BaseObject.h"

#include <Editor/EditorDefs.h>

/*!
 *  CSelectionGroup is a named selection group of objects.
 */
class SANDBOX_API CSelectionGroup
{
public:
    CSelectionGroup();

    //! Set name of selection.
    void SetName(const QString& name) { m_name = name; };
    //! Get name of selection.
    const QString& GetName() const { return m_name; };

    //! Adds object into selection list.
    void AddObject(CBaseObject* obj);
    //! Remove object from selection list.
    void RemoveObject(CBaseObject* obj);
    //! Remove all objects from selection.
    void RemoveAll();
    //! Remove all objects from selection except for the LegacyObjects list
    //! This is used in a performance improvement for deselecting legacy objects
    void RemoveAllExceptLegacySet();
    //! Check if object contained in selection list.
    bool IsContainObject(CBaseObject* obj);
    //! Return true if selection doesnt contain any object.
    bool IsEmpty() const;
    //! Check if all selected objects are of same type
    bool SameObjectType();
    //! Number of selected object.
    int  GetCount() const;
    //! Get object at given index.
    CBaseObject* GetObject(int index) const;
    //! Get object from a GUID
    CBaseObject* GetObjectByGuid(REFGUID guid) const;
    //! Get set of legacy objects
    std::set<CBaseObjectPtr>& GetLegacyObjects();

    //! Get mass center of selected objects.
    Vec3    GetCenter() const;

    //! Get Bounding box of selection.
    AABB GetBounds() const;

    void    Copy(const CSelectionGroup& from);

    //! Remove from selection group all objects which have parent also in selection group.
    //! And save resulting objects to saveTo selection.
    void    FilterParents();
    //! Get number of child filtered objects.
    int GetFilteredCount() const { return static_cast<int>(m_filtered.size()); }
    CBaseObject* GetFilteredObject(int i) const { return m_filtered[i]; }

    //////////////////////////////////////////////////////////////////////////
    // Operations on selection group.
    //////////////////////////////////////////////////////////////////////////
    enum EMoveSelectionFlag
    {
        eMS_None = 0x00,
        eMS_FollowTerrain = 0x01,
        eMS_FollowGeometryPosNorm = 0x02
    };
    //! Move objects in selection by offset.
    void Move(const Vec3& offset, EMoveSelectionFlag moveFlag, int referenceCoordSys, const QPoint& point = QPoint(-1, -1));
    //! Move objects in selection to specific position.
    void MoveTo(const Vec3& pos, EMoveSelectionFlag moveFlag, int referenceCoordSys, const QPoint& point = QPoint(-1, -1));
    //! Rotate objects in selection by given quaternion.
    void Rotate(const Quat& qRot, int referenceCoordSys);
    //! Rotate objects in selection by given angle.
    void Rotate(const Ang3& angles, int referenceCoordSys);
    //! Rotate objects in selection by given rotation matrix.
    void Rotate(const Matrix34& matRot, int referenceCoordSys);
    //! Transforms objects
    void Transform(const Vec3& offset, EMoveSelectionFlag moveFlag, const Ang3& angles, const Vec3& scale, int referenceCoordSys);
    //! Resets rotation and scale to identity and (1.0f, 1.0f, 1.0f)
    void ResetTransformation();
    //! Scale objects in selection by given scale.
    void Scale(const Vec3& scale, int referenceCoordSys);
    void SetScale(const Vec3& scale, int referenceCoordSys);
    //! Align objects in selection to surface normal
    void Align();
    //! Very special method to move contents of a voxel.
    void MoveContent(const Vec3& offset);

    // Send event to all objects in selection group.
    void SendEvent(ObjectEvent event);

    ULONG STDMETHODCALLTYPE     AddRef();
    ULONG STDMETHODCALLTYPE     Release();

    void IndicateSnappingVertex(DisplayContext& dc) const;
    void FinishChanges();
private:
    QString m_name;
    typedef std::vector<TSmartPtr<CBaseObject> > Objects;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    Objects m_objects;
    // Objects set, for fast searches.
    std::set<CBaseObject*> m_objectsSet;

    // Legacy objects aren't deselected through Ebuses, so keeping a 
    // separate set for them helps improve performance of deselection
    std::set<CBaseObjectPtr> m_legacyObjectsSet;

    //! Selection list with child objecs filtered out.
    std::vector<CBaseObject*> m_filtered;

    bool m_bVertexSnapped;
    Vec3 m_snapVertex;

    const static int SnappingVertexNumThreshold = 700;

    EMoveSelectionFlag m_LastestMoveSelectionFlag;
    Quat m_LastestMovedObjectRot;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

protected:
    ULONG   m_ref;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_SELECTIONGROUP_H
