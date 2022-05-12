/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : CSelectionGroup implementation.


#include "EditorDefs.h"

#include "SelectionGroup.h"

// Editor
#include "ViewManager.h"
#include "Include/IObjectManager.h"

//////////////////////////////////////////////////////////////////////////
CSelectionGroup::CSelectionGroup()
    : m_ref(1)
    , m_bVertexSnapped(false)
{
    m_LastestMoveSelectionFlag = eMS_None;
    m_LastestMovedObjectRot.SetIdentity();
}

//////////////////////////////////////////////////////////////////////////
void CSelectionGroup::AddObject(CBaseObject* obj)
{
    if (!IsContainObject(obj))
    {
        m_objects.push_back(obj);
        m_objectsSet.insert(obj);
        m_filtered.clear();

        if (obj->GetType() != OBJTYPE_AZENTITY)
        {
            m_legacyObjectsSet.insert(obj);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSelectionGroup::RemoveObject(CBaseObject* obj)
{
    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        if (*it == obj)
        {
            m_objects.erase(it);
            m_objectsSet.erase(obj);
            m_legacyObjectsSet.erase(obj);
            m_filtered.clear();
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSelectionGroup::RemoveAll()
{
    m_objects.clear();
    m_objectsSet.clear();
    m_filtered.clear();
    m_legacyObjectsSet.clear();
}

void CSelectionGroup::RemoveAllExceptLegacySet()
{
    m_objects.clear();
    m_objectsSet.clear();
    m_filtered.clear();
}

bool CSelectionGroup::IsContainObject(CBaseObject* obj)
{
    return (m_objectsSet.find(obj) != m_objectsSet.end());
}

//////////////////////////////////////////////////////////////////////////
bool CSelectionGroup::IsEmpty() const
{
    return m_objects.empty();
}

//////////////////////////////////////////////////////////////////////////
bool CSelectionGroup::SameObjectType()
{
    if (IsEmpty())
    {
        return false;
    }
    CBaseObjectPtr pFirst = (*(m_objects.begin()));
    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        if ((*it)->metaObject() != pFirst->metaObject())
        {
            return false;
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
int CSelectionGroup::GetCount() const
{
    return static_cast<int>(m_objects.size());
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CSelectionGroup::GetObject(int index) const
{
    assert(index >= 0 && index < m_objects.size());
    return m_objects[index];
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CSelectionGroup::GetObjectByGuid(REFGUID guid) const
{
    for (size_t i = 0, count(m_objects.size()); i < count; ++i)
    {
        if (m_objects[i]->GetId() == guid)
        {
            return m_objects[i];
        }
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
std::set<CBaseObjectPtr>& CSelectionGroup::GetLegacyObjects()
{
    return m_legacyObjectsSet;
}

//////////////////////////////////////////////////////////////////////////
void CSelectionGroup::Copy(const CSelectionGroup& from)
{
    m_name = from.m_name;
    m_objects = from.m_objects;
    m_objectsSet = from.m_objectsSet;
    m_filtered = from.m_filtered;
}

//////////////////////////////////////////////////////////////////////////
Vec3    CSelectionGroup::GetCenter() const
{
    Vec3 c(0, 0, 0);
    for (int i = 0; i < GetCount(); i++)
    {
        c += GetObject(i)->GetWorldPos();
    }
    if (GetCount() > 0)
    {
        c /= static_cast<f32>(GetCount());
    }
    return c;
}

//////////////////////////////////////////////////////////////////////////
AABB CSelectionGroup::GetBounds() const
{
    AABB b;
    AABB box;
    box.Reset();
    for (int i = 0; i < GetCount(); i++)
    {
        GetObject(i)->GetBoundBox(b);
        box.Add(b.min);
        box.Add(b.max);
    }
    return box;
}

//////////////////////////////////////////////////////////////////////////
void CSelectionGroup::FilterParents()
{
    if (!m_filtered.empty())
    {
        return;
    }

    m_filtered.reserve(m_objects.size());
    for (int i = 0; i < m_objects.size(); i++)
    {
        CBaseObject* obj = m_objects[i];
        CBaseObject* parent = obj->GetParent();
        bool bParentInSet = false;
        while (parent)
        {
            if (m_objectsSet.find(parent) != m_objectsSet.end())
            {
                bParentInSet = true;
                break;
            }
            parent = parent->GetParent();
        }
        if (!bParentInSet)
        {
            m_filtered.push_back(obj);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CSelectionGroup::Move(const Vec3& offset, EMoveSelectionFlag moveFlag, [[maybe_unused]] int referenceCoordSys, const QPoint& point)
{
    // [MichaelS - 17/3/2005] Removed this code from the three edit functions (move,
    // rotate and scale). This was causing a bug where the render node of objects
    // was not being updated when objects were dragged away from their position
    // and then back again, since movement is re-calculated from the initial position
    // each mouse message (ie first the previous movement is undone and then the
    // movement is applied). This meant that when moving back to the start position
    // it appeared like no movement was applied, although it was still necessary to
    // update the graphics resources. The object transform is explicitly reset
    // below.

    //if (offset.x == 0 && offset.y == 0 && offset.z == 0)
    //  return;

    m_bVertexSnapped = false;
    FilterParents();

    bool bValidFollowGeometryMode(true);
    if (point.x() == -1 || point.y() == -1)
    {
        bValidFollowGeometryMode = false;
    }

    if (moveFlag == eMS_FollowGeometryPosNorm)
    {
        if (m_LastestMoveSelectionFlag != eMS_FollowGeometryPosNorm)
        {
            if (GetFilteredCount() > 0)
            {
                CBaseObject* pObj = GetFilteredObject(0);
                m_LastestMovedObjectRot = pObj->GetRotation();
            }
        }
    }

    m_LastestMoveSelectionFlag = moveFlag;
    Vec3 zeroPickedInfo_vHitNormal;
    Vec3 zeroPickedInfo_vHitPos;

    for (int i = 0; i < GetFilteredCount(); i++)
    {
        CBaseObject* obj = GetFilteredObject(i);

        if(obj->IsFrozen())
        {
            continue;
        }

        if (i == 0 && moveFlag == eMS_FollowGeometryPosNorm && bValidFollowGeometryMode)
        {
            Vec3 zaxis = m_LastestMovedObjectRot * Vec3(0, 0, 1);
            zaxis.Normalize();
            Quat nq;
            nq.SetRotationV0V1(zaxis, zeroPickedInfo_vHitNormal);
            obj->SetPos(zeroPickedInfo_vHitPos);
            obj->SetRotation(nq * m_LastestMovedObjectRot);
            continue;
        }

        const Matrix34 &wtm = obj->GetWorldTM();
        Vec3 wp = wtm.GetTranslation();
        Vec3 newPos = wp + offset;
        if (moveFlag == eMS_FollowTerrain)
        {
            // Make sure object keeps it height.
            float height = wp.z - GetIEditor()->GetTerrainElevation(wp.x, wp.y);
            newPos.z = GetIEditor()->GetTerrainElevation(newPos.x, newPos.y) + height;
        }

        obj->SetWorldPos(newPos, eObjectUpdateFlags_UserInput | eObjectUpdateFlags_PositionChanged | eObjectUpdateFlags_MoveTool);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSelectionGroup::MoveTo(const Vec3& pos, EMoveSelectionFlag moveFlag, int referenceCoordSys, const QPoint& point)
{
    FilterParents();
    if (GetFilteredCount() < 1)
    {
        return;
    }

    CBaseObject* refObj = GetFilteredObject(0);
    CSelectionGroup::Move(pos - refObj->GetWorldTM().GetTranslation(), moveFlag, referenceCoordSys, point);
}


//////////////////////////////////////////////////////////////////////////
void CSelectionGroup::Rotate(const Quat& qRot, int referenceCoordSys)
{
    Matrix34 rotateTM;
    rotateTM.SetIdentity();
    rotateTM = Matrix33(qRot) * rotateTM;

    Rotate(rotateTM, referenceCoordSys);
}

//////////////////////////////////////////////////////////////////////////
void CSelectionGroup::Rotate(const Ang3& angles, int referenceCoordSys)
{
    //if (angles.x == 0 && angles.y == 0 && angles.z == 0)
    //  return;

    // Rotate selection about selection center.
    Matrix34 rotateTM = Matrix34::CreateRotationXYZ(DEG2RAD(angles));
    Rotate(rotateTM, referenceCoordSys);
}

//////////////////////////////////////////////////////////////////////////
void CSelectionGroup::Rotate(const Matrix34& rotateTM, int referenceCoordSys)
{
    // Rotate selection about selection center.
    Vec3 center = GetCenter();

    Matrix34 ToOrigin = Matrix34::CreateIdentity();
    Matrix34 FromOrigin = Matrix34::CreateIdentity();

    if (referenceCoordSys != COORDS_LOCAL)
    {
        ToOrigin.SetTranslation(-center);
        FromOrigin.SetTranslation(center);

        if (referenceCoordSys == COORDS_USERDEFINED)
        {
            Matrix34 userTM;
            userTM.SetIdentity();
            Matrix34 invUserTM = userTM.GetInvertedFast();

            ToOrigin = invUserTM * ToOrigin;
            FromOrigin = FromOrigin * userTM;
        }
    }

    FilterParents();

    for (int i = 0; i < GetFilteredCount(); i++)
    {
        CBaseObject* obj = GetFilteredObject(i);

        Matrix34 objectTransform = obj->GetWorldTM();
        if (referenceCoordSys != COORDS_LOCAL)
        {
            if (referenceCoordSys == COORDS_PARENT && obj->GetParent())
            {
                Matrix34 parentTM = obj->GetParent()->GetWorldTM();
                parentTM.OrthonormalizeFast();
                parentTM.SetTranslation(Vec3(0, 0, 0));
                Matrix34 invParentTM = parentTM.GetInvertedFast();

                objectTransform = FromOrigin * parentTM * rotateTM * invParentTM * ToOrigin * objectTransform;
            }
            else
            {
                objectTransform = FromOrigin * rotateTM * ToOrigin * objectTransform;
            }
        }
        else
        {
            // Decompose the matrix and reconstruct it to ensure no scaling artifacts are introduced
            AffineParts affineParts;
            affineParts.SpectralDecompose(objectTransform);

            Matrix33 rotationMatrix(affineParts.rot);
            Matrix34 translationMatrix = Matrix34::CreateTranslationMat(affineParts.pos);
            Matrix33 scaleMatrix = Matrix33::CreateScale(affineParts.scale);

            objectTransform = translationMatrix * rotationMatrix * rotateTM * scaleMatrix;
        }

        obj->SetWorldTM(objectTransform, eObjectUpdateFlags_UserInput);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSelectionGroup::Scale(const Vec3& scale, int referenceCoordSys)
{
    //if (scale.x == 1 && scale.y == 1 && scale.z == 1)
    //  return;

    Vec3 scl = scale;
    if (scl.x == 0)
    {
        scl.x = 0.01f;
    }
    if (scl.y == 0)
    {
        scl.y = 0.01f;
    }
    if (scl.z == 0)
    {
        scl.z = 0.01f;
    }

    // Scale selection relative to selection center.
    Vec3 center = GetCenter();

    Matrix34 scaleTM;
    scaleTM.SetIdentity();
    scaleTM = Matrix33::CreateScale(Vec3(scl.x, scl.y, scl.z)) * scaleTM;

    Matrix34 ToOrigin;
    Matrix34 FromOrigin;

    ToOrigin.SetIdentity();
    FromOrigin.SetIdentity();

    if (referenceCoordSys != COORDS_LOCAL)
    {
        ToOrigin.SetTranslation(-center);
        FromOrigin.SetTranslation(center);
    }

    FilterParents();

    for (int i = 0; i < GetFilteredCount(); i++)
    {
        CBaseObject* obj = GetFilteredObject(i);
        Matrix34 m = obj->GetWorldTM();

        if (referenceCoordSys != COORDS_LOCAL)
        {
            // Apply new scale
            m = FromOrigin * scaleTM * ToOrigin * m;
        }
        else
        {
            // Apply new scale
            m = m * scaleTM;
        }

        obj->SetWorldTM(m, eObjectUpdateFlags_UserInput | eObjectUpdateFlags_ScaleTool);
        obj->InvalidateTM(eObjectUpdateFlags_UserInput | eObjectUpdateFlags_ScaleTool);
    }
}


void CSelectionGroup::SetScale(const Vec3& scale, int referenceCoordSys)
{
    Vec3 relScale = scale;

    if (GetCount() > 0 && GetObject(0))
    {
        Vec3 objScale = GetObject(0)->GetScale();
        if (relScale == objScale && (objScale.x == 0.0f || objScale.y == 0.0f || objScale.z == 0.0f))
        {
            return;
        }
        relScale = relScale / objScale;
    }

    Scale(relScale, referenceCoordSys);
}

//////////////////////////////////////////////////////////////////////////
void CSelectionGroup::Align()
{
    for (int i = 0; i < GetFilteredCount(); ++i)
    {
        bool terrain = false;
        CBaseObject* obj = GetFilteredObject(i);
        Vec3 pos = obj->GetPos();
        Quat rot = obj->GetRotation();
        QPoint point = GetIEditor()->GetActiveView()->WorldToView(pos);
        Vec3 normal = GetIEditor()->GetActiveView()->ViewToWorldNormal(point, false, true);
        pos =   GetIEditor()->GetActiveView()->ViewToWorld(point, &terrain, false, false, true);
        Vec3 zaxis = rot * Vec3(0, 0, 1);
        normal.Normalize();
        zaxis.Normalize();
        Quat nq;
        nq.SetRotationV0V1(zaxis, normal);
        obj->SetRotation(nq * rot);
        obj->SetPos(pos);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSelectionGroup::Transform(const Vec3& offset, EMoveSelectionFlag moveFlag, const Ang3& angles, const Vec3& scale, int referenceCoordSys)
{
    if (offset != Vec3(0))
    {
        Move(offset, moveFlag, referenceCoordSys);
    }

    if (!(angles == Ang3(ZERO)))
    {
        Rotate(angles, referenceCoordSys);
    }

    if (scale != Vec3(0))
    {
        Scale(scale, referenceCoordSys);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSelectionGroup::ResetTransformation()
{
    FilterParents();
    Quat qIdentity;
    qIdentity.SetIdentity();
    Vec3 vScale(1.0f, 1.0f, 1.0f);

    for (int i = 0, n = GetFilteredCount(); i < n; ++i)
    {
        CBaseObject* pObj = GetFilteredObject(i);
        pObj->SetRotation(qIdentity);
        pObj->SetScale(vScale);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSelectionGroup::SendEvent(ObjectEvent event)
{
    for (int i = 0; i < m_objects.size(); i++)
    {
        CBaseObject* obj = m_objects[i];
        obj->OnEvent(event);
    }
}

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
ULONG STDMETHODCALLTYPE CSelectionGroup::AddRef()
{
    return ++m_ref;
};
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
ULONG STDMETHODCALLTYPE CSelectionGroup::Release()
{
    if ((--m_ref) == 0)
    {
        delete this;
        return 0;
    }
    else
    {
        return m_ref;
    }
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CSelectionGroup::IndicateSnappingVertex(DisplayContext& dc) const
{
    if (m_bVertexSnapped == false)
    {
        return;
    }

    dc.DepthTestOff();

    ColorB green(0, 255, 0, 255);

    dc.SetColor(green);
    float fScale = dc.view->GetScreenScaleFactor(m_snapVertex) * 0.005f;
    Vec3 sz(fScale, fScale, fScale);
    dc.DrawWireBox(m_snapVertex - sz, m_snapVertex + sz);

    dc.DepthTestOn();
}

//////////////////////////////////////////////////////////////////////////
void CSelectionGroup::FinishChanges()
{
    Objects selectedObjects(m_objects);
    int iObjectSize = static_cast<int>(selectedObjects.size());
    for (int i = 0; i < iObjectSize; ++i)
    {
        CBaseObject* pObject = selectedObjects[i];
        if (pObject == nullptr)
        {
            continue;
        }
    }
}
