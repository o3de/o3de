/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : CBaseObject implementation.


#include "EditorDefs.h"

#include "BaseObject.h"

// Azframework
#include <AzFramework/Terrain/TerrainDataRequestBus.h>      // for AzFramework::Terrain::TerrainDataRequests

// AzToolsFramework
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>  // for ComponentEntityObjectRequestBus

// Editor
#include "Settings.h"
#include "Viewport.h"
#include "DisplaySettings.h"
#include "Undo/Undo.h"
#include "UsedResources.h"
#include "Objects/SelectionGroup.h"
#include "Objects/ObjectManager.h"
#include "ViewManager.h"
#include "IEditorImpl.h"
#include "GameEngine.h"
// To use the Andrew's algorithm in order to make convex hull from the points, this header is needed.
#include "Util/GeometryUtil.h"

extern CObjectManager* g_pObjectManager;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//! Undo object for CBaseObject.
class CUndoBaseObject
    : public IUndoObject
{
public:
    CUndoBaseObject(CBaseObject* pObj);

protected:
    int GetSize() override { return sizeof(*this);   }
    QString GetObjectName() override;

    void Undo(bool bUndo) override;
    void Redo() override;

protected:
    GUID m_guid;
    XmlNodeRef m_undo;
    XmlNodeRef m_redo;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//! Undo object for CBaseObject that only stores its transform, color, area
class CUndoBaseObjectMinimal
    : public IUndoObject
{
public:
    CUndoBaseObjectMinimal(CBaseObject* obj, int flags);

protected:
    int GetSize() override { return sizeof(*this); }
    QString GetObjectName() override;

    void Undo(bool bUndo) override;
    void Redo() override;

private:
    struct StateStruct
    {
        Vec3 pos;
        Quat rotate;
        Vec3 scale;
        QColor color;
        float area;
    };

    void SetTransformsFromState(CBaseObject* pObject, const StateStruct& state, bool bUndo);

    GUID m_guid;
    StateStruct m_undoState;
    StateStruct m_redoState;
};

//////////////////////////////////////////////////////////////////////////
//! Undo object for attach/detach changes
class CUndoAttachBaseObject
    : public IUndoObject
{
public:
    CUndoAttachBaseObject(CBaseObject* pAttachedObject, bool bKeepPos, bool bAttach)
        : m_attachedObjectGUID(pAttachedObject->GetId())
        , m_parentObjectGUID(pAttachedObject->GetParent()->GetId())
        , m_bKeepPos(bKeepPos)
        , m_bAttach(bAttach) {}

    void Undo([[maybe_unused]] bool bUndo) override
    {
        if (m_bAttach)
        {
            Detach();
        }
        else
        {
            Attach();
        }
    }

    void Redo() override
    {
        if (m_bAttach)
        {
            Attach();
        }
        else
        {
            Detach();
        }
    }

private:
    void Attach()
    {
        CObjectManager* pObjectManager = static_cast<CObjectManager*>(GetIEditor()->GetObjectManager());
        CBaseObject* pObject = pObjectManager->FindObject(m_attachedObjectGUID);
        CBaseObject* pParentObject = pObjectManager->FindObject(m_parentObjectGUID);

        if (pObject && pParentObject)
        {
            pParentObject->AttachChild(pObject, m_bKeepPos);
        }
    }

    void Detach()
    {
        CObjectManager* pObjectManager = static_cast<CObjectManager*>(GetIEditor()->GetObjectManager());
        CBaseObject* pObject = pObjectManager->FindObject(m_attachedObjectGUID);

        if (pObject)
        {
            pObject->DetachThis(m_bKeepPos);
        }
    }

    int GetSize() override { return sizeof(CUndoAttachBaseObject); }

    GUID m_attachedObjectGUID;
    GUID m_parentObjectGUID;
    bool m_bKeepPos;
    bool m_bAttach;
};

//////////////////////////////////////////////////////////////////////////
CUndoBaseObject::CUndoBaseObject(CBaseObject* obj)
{
    // Stores the current state of this object.
    assert(obj != 0);
    m_guid = obj->GetId();

    m_redo = nullptr;
    m_undo = XmlHelpers::CreateXmlNode("Undo");
    CObjectArchive ar(GetIEditor()->GetObjectManager(), m_undo, false);
    ar.bUndo = true;
    obj->Serialize(ar);
}

//////////////////////////////////////////////////////////////////////////
QString CUndoBaseObject::GetObjectName()
{
    if (CBaseObject* obj = GetIEditor()->GetObjectManager()->FindObject(m_guid))
    {
        return obj->GetName();
    }
    return QString();
}

//////////////////////////////////////////////////////////////////////////
void CUndoBaseObject::Undo(bool bUndo)
{
    CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(m_guid);
    if (!pObject)
    {
        return;
    }

    GetIEditor()->SuspendUndo();

    if (bUndo)
    {
        m_redo = XmlHelpers::CreateXmlNode("Redo");
        // Save current object state.
        CObjectArchive ar(GetIEditor()->GetObjectManager(), m_redo, false);
        ar.bUndo = true;
        pObject->Serialize(ar);
    }

    // Undo object state.
    CObjectArchive ar(GetIEditor()->GetObjectManager(), m_undo, true);
    ar.bUndo = true;
    pObject->Serialize(ar);

    GetIEditor()->ResumeUndo();

    using namespace AzToolsFramework;
    ComponentEntityObjectRequestBus::Event(pObject, &ComponentEntityObjectRequestBus::Events::UpdatePreemptiveUndoCache);
}

//////////////////////////////////////////////////////////////////////////
void CUndoBaseObject::Redo()
{
    CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(m_guid);
    if (!pObject)
    {
        return;
    }

    GetIEditor()->SuspendUndo();

    CObjectArchive ar(GetIEditor()->GetObjectManager(), m_redo, true);
    ar.bUndo = true;

    pObject->Serialize(ar);

    GetIEditor()->ResumeUndo();

    using namespace AzToolsFramework;
    ComponentEntityObjectRequestBus::Event(pObject, &ComponentEntityObjectRequestBus::Events::UpdatePreemptiveUndoCache);
}

//////////////////////////////////////////////////////////////////////////
CUndoBaseObjectMinimal::CUndoBaseObjectMinimal(CBaseObject* pObj, [[maybe_unused]] int flags)
{
    // Stores the current state of this object.
    assert(pObj != nullptr);
    m_guid = pObj->GetId();

    ZeroStruct(m_redoState);

    m_undoState.pos = pObj->GetPos();
    m_undoState.rotate = pObj->GetRotation();
    m_undoState.scale = pObj->GetScale();
    m_undoState.color = pObj->GetColor();
    m_undoState.area = pObj->GetArea();
}

//////////////////////////////////////////////////////////////////////////
QString CUndoBaseObjectMinimal::GetObjectName()
{
    CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(m_guid);
    if (!pObject)
    {
        return QString();
    }

    return pObject->GetName();
}

//////////////////////////////////////////////////////////////////////////
void CUndoBaseObjectMinimal::Undo(bool bUndo)
{
    CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(m_guid);
    if (!pObject)
    {
        return;
    }

    if (bUndo)
    {
        m_redoState.pos = pObject->GetPos();
        m_redoState.scale = pObject->GetScale();
        m_redoState.rotate = pObject->GetRotation();
        m_redoState.color = pObject->GetColor();
        m_redoState.area = pObject->GetArea();
    }

    SetTransformsFromState(pObject, m_undoState, bUndo);

    pObject->ChangeColor(m_undoState.color);
    pObject->SetArea(m_undoState.area);

    using namespace AzToolsFramework;
    ComponentEntityObjectRequestBus::Event(pObject, &ComponentEntityObjectRequestBus::Events::UpdatePreemptiveUndoCache);
}

//////////////////////////////////////////////////////////////////////////
void CUndoBaseObjectMinimal::Redo()
{
    CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(m_guid);
    if (!pObject)
    {
        return;
    }

    SetTransformsFromState(pObject, m_redoState, true);

    pObject->ChangeColor(m_redoState.color);
    pObject->SetArea(m_redoState.area);

    using namespace AzToolsFramework;
    ComponentEntityObjectRequestBus::Event(pObject, &ComponentEntityObjectRequestBus::Events::UpdatePreemptiveUndoCache);
}

//////////////////////////////////////////////////////////////////////////
void CUndoBaseObjectMinimal::SetTransformsFromState(CBaseObject* pObject, const StateStruct& state, bool bUndo)
{
    uint32 flags = eObjectUpdateFlags_Undo;
    if (!bUndo)
    {
        flags |= eObjectUpdateFlags_UserInputUndo;
    }

    pObject->SetPos(state.pos, flags);
    pObject->SetScale(state.scale, flags);
    pObject->SetRotation(state.rotate, flags);
}

//////////////////////////////////////////////////////////////////////////
void CObjectCloneContext::AddClone(CBaseObject* pFromObject, CBaseObject* pToObject)
{
    m_objectsMap[pFromObject] = pToObject;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CObjectCloneContext::FindClone(CBaseObject* pFromObject)
{
    CBaseObject* pTarget = stl::find_in_map(m_objectsMap, pFromObject, (CBaseObject*) nullptr);
    return pTarget;
}

//////////////////////////////////////////////////////////////////////////
GUID CObjectCloneContext::ResolveClonedID(REFGUID guid)
{
    CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(guid);
    CBaseObject* pClonedTarget = FindClone(pObject);
    if (!pClonedTarget)
    {
        pClonedTarget = pObject; // If target not cloned, link to original target.
    }
    if (pClonedTarget)
    {
        return pClonedTarget->GetId();
    }
    return GUID_NULL;
}

//////////////////////////////////////////////////////////////////////////
// CBaseObject implementation.
//////////////////////////////////////////////////////////////////////////
CBaseObject::CBaseObject()
    : m_pos(0, 0, 0)
    , m_rotate(IDENTITY)
    , m_scale(1, 1, 1)
    , m_guid(GUID_NULL)
    , m_flags(0)
    , m_nTextureIcon(0)
    , m_color(QColor(255, 255, 255))
    , m_worldTM(IDENTITY)
    , m_lookat(nullptr)
    , m_lookatSource(nullptr)
    , m_flattenArea(0.f)
    , m_classDesc(nullptr)
    , m_numRefs(0)
    , m_parent(nullptr)
    , m_bInSelectionBox(false)
    , m_pTransformDelegate(nullptr)
    , m_bMatrixInWorldSpace(false)
    , m_bMatrixValid(false)
    , m_bWorldBoxValid(false)
    , m_vDrawIconPos(0, 0, 0)
    , m_nIconFlags(0)
{
    m_worldBounds.min.Set(0, 0, 0);
    m_worldBounds.max.Set(0, 0, 0);
}

//////////////////////////////////////////////////////////////////////////
IObjectManager* CBaseObject::GetObjectManager() const
{
    return g_pObjectManager;
};


void CBaseObject::SetClassDesc(CObjectClassDesc* classDesc)
{
    m_classDesc = classDesc;
}

//! Initialize Object.
bool CBaseObject::Init([[maybe_unused]] IEditor* ie, CBaseObject* prev, [[maybe_unused]] const QString& file)
{
    SetFlags(m_flags & (~OBJFLAG_DELETED));

    if (prev != nullptr)
    {
        SetUniqueName(prev->GetName());
        SetLocalTM(prev->GetPos(), prev->GetRotation(), prev->GetScale());
        SetArea(prev->GetArea());
        SetColor(prev->GetColor());

        // Copy all basic variables.
        EnableUpdateCallbacks(false);
        CopyVariableValues(prev);
        EnableUpdateCallbacks(true);
        OnSetValues();
    }

    if (m_classDesc->RenderTextureOnTop())
    {
        SetFlags(OBJFLAG_SHOW_ICONONTOP);
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject::~CBaseObject()
{
    for (Childs::iterator c = m_childs.begin(); c != m_childs.end(); c++)
    {
        CBaseObject* child = *c;
        child->m_parent = nullptr;
    }
    m_childs.clear();
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::Done()
{
    DetachThis();

    // From children
    DetachAll();

    SetLookAt(nullptr);
    if (m_lookatSource)
    {
        m_lookatSource->SetLookAt(nullptr);
    }
    SetFlags(m_flags | OBJFLAG_DELETED);

    NotifyListeners(CBaseObject::ON_DELETE);
    m_eventListeners.clear();
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetName(const QString& name)
{
    if (name == m_name)
    {
        return;
    }

    StoreUndo();

    // Notification is expensive and not required if this is during construction.
    bool notify = (!m_name.isEmpty());

    m_name = name;
    GetObjectManager()->RegisterObjectName(name);
    SetModified(false);

    if (notify)
    {
        NotifyListeners(ON_RENAME);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetUniqueName(const QString& name)
{
    SetName(GetObjectManager()->GenerateUniqueObjectName(name));
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::GenerateUniqueName()
{
    if (m_classDesc)
    {
        SetUniqueName(m_classDesc->ClassName());
    }
    else
    {
        SetUniqueName("Object");
    }
}

//////////////////////////////////////////////////////////////////////////
const QString& CBaseObject::GetName() const
{
    return m_name;
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::SetPos(const Vec3& pos, int flags)
{
    const Vec3 currentPos = GetPos();

    bool equal = false;
    if (flags & eObjectUpdateFlags_MoveTool) // very sensitive in case of the move tool
    {
        equal = IsVectorsEqual(currentPos, pos, 0.0f);
    }
    else // less sensitive for others
    {
        equal = IsVectorsEqual(currentPos, pos);
    }

    if (equal)
    {
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    // Check if position is bad.
    if (fabs(pos.x) > AZ::Constants::MaxFloatBeforePrecisionLoss ||
        fabs(pos.y) > AZ::Constants::MaxFloatBeforePrecisionLoss ||
        fabs(pos.z) > AZ::Constants::MaxFloatBeforePrecisionLoss ||
        !_finite(pos.x) || !_finite(pos.y) || !_finite(pos.z))
    {
        CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING,
            "Object %s, SetPos called with invalid position: (%f,%f,%f)",
            GetName().toUtf8().data(), pos.x, pos.y, pos.z);
        return false;
    }

    OnBeforeAreaChange();

    //////////////////////////////////////////////////////////////////////////
    const bool bPositionDelegated = m_pTransformDelegate && m_pTransformDelegate->IsPositionDelegated();
    if (m_pTransformDelegate && (flags & eObjectUpdateFlags_Animated) == 0)
    {
        m_pTransformDelegate->SetTransformDelegatePos(pos);
    }

    //////////////////////////////////////////////////////////////////////////
    if (!bPositionDelegated && (flags & eObjectUpdateFlags_RestoreUndo) == 0 && (flags & eObjectUpdateFlags_Animated) == 0)
    {
        StoreUndo(true, flags);
    }

    if (!bPositionDelegated)
    {
        m_pos = pos;
    }

    if (!(flags & eObjectUpdateFlags_DoNotInvalidate))
    {
        InvalidateTM(flags | eObjectUpdateFlags_PositionChanged);
    }

    SetModified(true);

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::SetRotation(const Quat& rotate, int flags)
{
    const Quat currentRotate = GetRotation();

    if (currentRotate.w == rotate.w && currentRotate.v.x == rotate.v.x
        && currentRotate.v.y == rotate.v.y && currentRotate.v.z == rotate.v.z)
    {
        return false;
    }

    if (flags & eObjectUpdateFlags_ScaleTool)
    {
        return false;
    }

    OnBeforeAreaChange();

    const bool bRotationDelegated = m_pTransformDelegate && m_pTransformDelegate->IsRotationDelegated();
    if (m_pTransformDelegate && (flags & eObjectUpdateFlags_Animated) == 0)
    {
        m_pTransformDelegate->SetTransformDelegateRotation(rotate);
    }

    if (!bRotationDelegated && (flags & eObjectUpdateFlags_RestoreUndo) == 0 && (flags & eObjectUpdateFlags_Animated) == 0)
    {
        StoreUndo(true, flags);
    }

    if (!bRotationDelegated)
    {
        m_rotate = rotate;
    }

    if (m_bMatrixValid && !(flags & eObjectUpdateFlags_DoNotInvalidate))
    {
        InvalidateTM(flags | eObjectUpdateFlags_RotationChanged);
    }

    SetModified(true);

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::SetScale(const Vec3& scale, int flags)
{
    if (IsVectorsEqual(GetScale(), scale))
    {
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    // Check if scale is bad.
    if (scale.x < 0.01f || scale.y < 0.01f || scale.z < 0.01f)
    {
        CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Object %s, SetScale called with invalid scale: (%f,%f,%f)", GetName().toUtf8().data(), scale.x, scale.y, scale.z);
        return false;
    }
    //////////////////////////////////////////////////////////////////////////

    OnBeforeAreaChange();

    const bool bScaleDelegated = m_pTransformDelegate && m_pTransformDelegate->IsScaleDelegated();
    if (m_pTransformDelegate && (flags & eObjectUpdateFlags_Animated) == 0)
    {
        m_pTransformDelegate->SetTransformDelegateScale(scale);
    }

    if (!bScaleDelegated && (flags & eObjectUpdateFlags_RestoreUndo) == 0 && (flags & eObjectUpdateFlags_Animated) == 0)
    {
        StoreUndo(true, flags);
    }

    if (!bScaleDelegated)
    {
        m_scale = scale;
    }

    if (m_bMatrixValid && !(flags & eObjectUpdateFlags_DoNotInvalidate))
    {
        InvalidateTM(flags | eObjectUpdateFlags_ScaleChanged);
    }

    SetModified(true);

    return true;
}

//////////////////////////////////////////////////////////////////////////
const Vec3 CBaseObject::GetPos() const
{
    if (!m_pTransformDelegate)
    {
        return m_pos;
    }

    return m_pTransformDelegate->GetTransformDelegatePos(m_pos);
}

//////////////////////////////////////////////////////////////////////////
const Quat CBaseObject::GetRotation() const
{
    if (!m_pTransformDelegate)
    {
        return m_rotate;
    }

    return m_pTransformDelegate->GetTransformDelegateRotation(m_rotate);
}

//////////////////////////////////////////////////////////////////////////
const Vec3 CBaseObject::GetScale() const
{
    if (!m_pTransformDelegate)
    {
        return m_scale;
    }

    return m_pTransformDelegate->GetTransformDelegateScale(m_scale);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::ChangeColor(const QColor& color)
{
    if (color == m_color)
    {
        return;
    }

    StoreUndo(true);

    SetColor(color);
    SetModified(false);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetColor(const QColor& color)
{
    m_color = color;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetArea(float area)
{
    if (m_flattenArea == area)
    {
        return;
    }

    StoreUndo(true);

    m_flattenArea = area;
    SetModified(false);
};

//! Get bounding box of object in world coordinate space.
void CBaseObject::GetBoundBox(AABB& box)
{
    if (!m_bWorldBoxValid)
    {
        GetLocalBounds(m_worldBounds);
        if (!m_worldBounds.IsReset() && !m_worldBounds.IsEmpty())
        {
            m_worldBounds.SetTransformedAABB(GetWorldTM(), m_worldBounds);
            m_bWorldBoxValid = true;
        }
    }
    box = m_worldBounds;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::GetLocalBounds(AABB& box)
{
    box.min.Set(0, 0, 0);
    box.max.Set(0, 0, 0);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetModified(bool)
{
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DrawDimensions(DisplayContext&, AABB*)
{
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DrawSelectionHelper(DisplayContext& dc, const Vec3& pos, const QColor& labelColor, [[maybe_unused]] float alpha)
{
    DrawLabel(dc, pos, labelColor);

    dc.SetColor(GetColor());
    if (IsHighlighted() || IsSelected() || IsInSelectionBox())
    {
        dc.SetColor(dc.GetSelectedColor());
    }

    uint32 nPrevState = dc.GetState();
    dc.DepthTestOff();
    float r = dc.view->GetScreenScaleFactor(pos) * 0.006f;
    dc.DrawWireBox(pos - Vec3(r, r, r), pos + Vec3(r, r, r));
    dc.SetState(nPrevState);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DrawLabel(DisplayContext& dc, const Vec3& pos, const QColor& lC, float alpha, float size)
{
    QColor labelColor = lC;

    AABB box;
    GetBoundBox(box);

    //p.z = box.max.z + 0.2f;
    if ((dc.flags & DISPLAY_2D) && labelColor == QColor(255, 255, 255))
    {
        labelColor = QColor(0, 0, 0);
    }

    if (dc.flags & DISPLAY_SELECTION_HELPERS)
    {
        Vec3 c(static_cast<f32>(labelColor.redF()), static_cast<f32>(labelColor.greenF()), static_cast<f32>(labelColor.redF()));
        if (IsSelected())
        {
            c = Vec3(static_cast<f32>(dc.GetSelectedColor().redF()), static_cast<f32>(dc.GetSelectedColor().greenF()), static_cast<f32>(dc.GetSelectedColor().blueF()));
        }

        float col[4] = { c.x, c.y, c.z, 1 };
        if (dc.flags & DISPLAY_SELECTION_HELPERS)
        {
            if (IsHighlighted())
            {
                c = Vec3(static_cast<f32>(dc.GetSelectedColor().redF()), static_cast<f32>(dc.GetSelectedColor().greenF()), static_cast<f32>(dc.GetSelectedColor().blueF()));
            }
            col[0] = c.x;
            col[1] = c.y;
            col[2] = c.z;
        }

        dc.SetColor(col[0], col[1], col[2], col[3] * alpha);
        dc.DrawTextLabel(pos, size, GetName().toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DrawHighlight(DisplayContext& dc)
{
    if (!m_nTextureIcon)
    {
        AABB box;
        GetLocalBounds(box);

        dc.PushMatrix(GetWorldTM());
        dc.DrawWireBox(box.min, box.max);
        dc.SetLineWidth(1);
        dc.PopMatrix();
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DrawBudgetUsage(DisplayContext& dc, const QColor& color)
{
    AABB box;
    GetLocalBounds(box);

    dc.SetColor(color);

    dc.PushMatrix(GetWorldTM());
    dc.DrawWireBox(box.min, box.max);
    dc.PopMatrix();
}

//////////////////////////////////////////////////////////////////////////
void    CBaseObject::DrawAxis([[maybe_unused]] DisplayContext& dc, [[maybe_unused]] const Vec3& pos, [[maybe_unused]] float size)
{
    /*
    dc.renderer->EnableDepthTest(false);
    Vec3 x(size,0,0);
    Vec3 y(0,size,0);
    Vec3 z(0,0,size);

    bool bWorldSpace = false;
    if (dc.flags & DISPLAY_WORLDSPACEAXIS)
        bWorldSpace = true;

    Matrix tm = GetWorldTM();
    Vec3 org = tm.TransformPoint( pos );

    if (!bWorldSpace)
    {
        tm.NoScale();
        x = tm.TransformVector(x);
        y = tm.TransformVector(y);
        z = tm.TransformVector(z);
    }

    float fScreenScale = dc.view->GetScreenScaleFactor(org);
    x = x * fScreenScale;
    y = y * fScreenScale;
    z = z * fScreenScale;

    float col[4] = { 1,1,1,1 };
    float hcol[4] = { 1,0,0,1 };
    dc.renderer->DrawLabelEx( org+x,1.2f,col,true,true,"X" );
    dc.renderer->DrawLabelEx( org+y,1.2f,col,true,true,"Y" );
    dc.renderer->DrawLabelEx( org+z,1.2f,col,true,true,"Z" );

    Vec3 colX(1,0,0),colY(0,1,0),colZ(0,0,1);
    if (s_highlightAxis)
    {
        float col[4] = { 1,0,0,1 };
        if (s_highlightAxis == 1)
        {
            colX.Set(1,1,0);
            dc.renderer->DrawLabelEx( org+x,1.2f,col,true,true,"X" );
        }
        if (s_highlightAxis == 2)
        {
            colY.Set(1,1,0);
            dc.renderer->DrawLabelEx( org+y,1.2f,col,true,true,"Y" );
        }
        if (s_highlightAxis == 3)
        {
            colZ.Set(1,1,0);
            dc.renderer->DrawLabelEx( org+z,1.2f,col,true,true,"Z" );
        }
    }

    x = x * 0.8f;
    y = y * 0.8f;
    z = z * 0.8f;
    float fArrowScale = fScreenScale * 0.07f;
    dc.SetColor( colX );
    dc.DrawArrow( org,org+x,fArrowScale );
    dc.SetColor( colY );
    dc.DrawArrow( org,org+y,fArrowScale );
    dc.SetColor( colZ );
    dc.DrawArrow( org,org+z,fArrowScale );

    //dc.DrawLine( org,org+x,colX,colX );
    //dc.DrawLine( org,org+y,colY,colY );
    //dc.DrawLine( org,org+z,colZ,colZ );

    dc.renderer->EnableDepthTest(true);
    ///dc.SetColor( 0,1,1,1 );
    //dc.DrawLine( p,p+dc.view->m_constructionPlane.m_normal*10.0f );
    */
}

void CBaseObject::DrawArea(DisplayContext& dc)
{
    float area = m_flattenArea;
    if (area > 0)
    {
        dc.SetColor(QColor(5, 5, 255), 1.f); // make it different color from the AI sight radius
        Vec3 wp = GetWorldPos();
        float z = GetIEditor()->GetTerrainElevation(wp.x, wp.y);
        if (fabs(wp.z - z) < 5)
        {
            dc.DrawTerrainCircle(wp, area, 0.2f);
        }
        else
        {
            dc.DrawCircle(wp, area);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::CanBeDrawn(const DisplayContext& dc, bool& outDisplaySelectionHelper) const
{
    bool bResult = true;
    outDisplaySelectionHelper = false;

    if (dc.flags & DISPLAY_SELECTION_HELPERS)
    {
        // Check if this object type is masked for selection.
        if ((GetType() & gSettings.objectSelectMask) && !IsFrozen())
        {
            if (IsSkipSelectionHelper())
            {
                return bResult;
            }
            if (CanBeHightlighted())
            {
                outDisplaySelectionHelper = true;
            }
        }
        else
        {
            // Object helpers should not be displayed when object is not for selection.
            bResult = false;
        }
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::OnEvent(ObjectEvent event)
{
    switch (event)
    {
    case EVENT_CONFIG_SPEC_CHANGE:
        UpdateVisibility(!IsHidden());
        break;
    }
}


//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetHidden(bool bHidden, bool bAnimated)
{
    if (CheckFlags(OBJFLAG_HIDDEN) != bHidden)
    {
        if (!bAnimated)
        {
            StoreUndo();
        }

        if (bHidden)
        {
            SetFlags(OBJFLAG_HIDDEN);
        }
        else
        {
            ClearFlags(OBJFLAG_HIDDEN);
        }

        UpdateVisibility(!IsHidden());
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetFrozen(bool bFrozen)
{
    if (CheckFlags(OBJFLAG_FROZEN) != bFrozen)
    {
        StoreUndo();
        if (bFrozen)
        {
            SetFlags(OBJFLAG_FROZEN);
        }
        else
        {
            ClearFlags(OBJFLAG_FROZEN);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetHighlight(bool bHighlight)
{
    if (bHighlight)
    {
        SetFlags(OBJFLAG_HIGHLIGHT);
    }
    else
    {
        ClearFlags(OBJFLAG_HIGHLIGHT);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetSelected(bool bSelect)
{
    if (bSelect)
    {
        SetFlags(OBJFLAG_SELECTED);
        NotifyListeners(ON_SELECT);

        //CLogFile::FormatLine( "Selected: %s, ID=%u",(const char*)m_name,m_id );
    }
    else
    {
        ClearFlags(OBJFLAG_SELECTED);
        NotifyListeners(ON_UNSELECT);
    }
}

//////////////////////////////////////////////////////////////////////////
//! Returns true if object hidden.
bool CBaseObject::IsHidden() const
{
    return (CheckFlags(OBJFLAG_HIDDEN)) ||
            (m_classDesc && (gSettings.objectHideMask & GetType()));
}

//////////////////////////////////////////////////////////////////////////
//! Returns true if object frozen.
bool CBaseObject::IsFrozen() const
{
    return CheckFlags(OBJFLAG_FROZEN);
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IsSelectable() const
{
    // Not selectable if frozen.
    return IsFrozen() ? false : true;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::Serialize(CObjectArchive& ar)
{
    XmlNodeRef xmlNode = ar.node;

    ITransformDelegate* pTransformDelegate = m_pTransformDelegate;
    m_pTransformDelegate = nullptr;

    if (ar.bLoading)
    {
        // Loading.
        int flags = 0;
        int oldFlags = m_flags;

        QString name = m_name;
        QString mtlName;

        Vec3 pos = m_pos;
        Vec3 scale = m_scale;
        Quat quat = m_rotate;
        Ang3 angles(0, 0, 0);

        QColor color = m_color;
        float flattenArea = m_flattenArea;

        GUID parentId = GUID_NULL;
        GUID lookatId = GUID_NULL;

        xmlNode->getAttr("Name", name);
        xmlNode->getAttr("Pos", pos);
        if (!xmlNode->getAttr("Rotate", quat))
        {
            // Backward compatibility.
            if (xmlNode->getAttr("Angles", angles))
            {
                angles = DEG2RAD(angles);
                //angles.z += gf_PI/2;
                quat.SetRotationXYZ(angles);
            }
        }

        xmlNode->getAttr("Scale", scale);
        xmlNode->getAttr("ColorRGB", color);
        xmlNode->getAttr("FlattenArea", flattenArea);
        xmlNode->getAttr("Flags", flags);
        xmlNode->getAttr("Parent", parentId);
        xmlNode->getAttr("LookAt", lookatId);
        xmlNode->getAttr("Material", mtlName);

        bool bHidden = flags & OBJFLAG_HIDDEN;
        bool bFrozen = flags & OBJFLAG_FROZEN;

        m_flags = flags;
        m_flags &= ~OBJFLAG_PERSISTMASK;
        m_flags |= (oldFlags) & (~OBJFLAG_PERSISTMASK);
        //SetFlags( flags & OBJFLAG_PERSISTMASK );
        m_flags &= ~OBJFLAG_SHARED; // clear shared flag
        m_flags &= ~OBJFLAG_DELETED; // clear deleted flag

        if (ar.bUndo)
        {
            DetachThis(false);
        }

        if (name != m_name)
        {
            // This may change object id.
            SetName(name);
        }

        //////////////////////////////////////////////////////////////////////////
        // Check if position is bad.
        if (fabs(pos.x) > AZ::Constants::MaxFloatBeforePrecisionLoss ||
            fabs(pos.y) > AZ::Constants::MaxFloatBeforePrecisionLoss ||
            fabs(pos.z) > AZ::Constants::MaxFloatBeforePrecisionLoss)
        {
            // File Not found.
            CErrorRecord err;
            err.error = QStringLiteral("Object %1 have invalid position (%2,%3,%4)").arg(GetName()).arg(pos.x).arg(pos.y).arg(pos.z);
            err.pObject = this;
            err.severity = CErrorRecord::ESEVERITY_WARNING;
            GetIEditor()->GetErrorReport()->ReportError(err);
        }
        //////////////////////////////////////////////////////////////////////////

        if (!ar.bUndo)
        {
            SetLocalTM(pos, quat, scale);
        }
        else
        {
            SetLocalTM(pos, quat, scale, eObjectUpdateFlags_Undo);
        }

        SetColor(color);
        SetArea(flattenArea);
        SetFrozen(bFrozen);
        SetHidden(bHidden);

        ar.SetResolveCallback(this, parentId, [this](CBaseObject* parent) { ResolveParent(parent); });
        ar.SetResolveCallback(this, lookatId, [this](CBaseObject* target) { SetLookAt(target); });

        InvalidateTM(0);
        SetModified(false);

        //////////////////////////////////////////////////////////////////////////
    }
    else
    {
        // Saving.

        // This attributed only readed by ObjectManager.
        xmlNode->setAttr("Type", GetTypeName().toUtf8().data());

        xmlNode->setAttr("Id", m_guid);

        xmlNode->setAttr("Name", GetName().toUtf8().data());

        if (m_parent)
        {
            xmlNode->setAttr("Parent", m_parent->GetId());
        }

        if (m_lookat)
        {
            xmlNode->setAttr("LookAt", m_lookat->GetId());
        }

        if (!IsEquivalent(GetPos(), Vec3(0, 0, 0), 0))
        {
            xmlNode->setAttr("Pos", GetPos());
        }

        xmlNode->setAttr("Rotate", m_rotate);

        if (!IsEquivalent(GetScale(), Vec3(1, 1, 1), 0))
        {
            xmlNode->setAttr("Scale", GetScale());
        }

        xmlNode->setAttr("ColorRGB", GetColor());

        if (GetArea() != 0)
        {
            xmlNode->setAttr("FlattenArea", GetArea());
        }

        int flags = m_flags & OBJFLAG_PERSISTMASK;
        if (flags != 0)
        {
            xmlNode->setAttr("Flags", flags);
        }
    }

    // Serialize variables after default entity parameters.
    CVarObject::Serialize(xmlNode, ar.bLoading);

    m_pTransformDelegate = pTransformDelegate;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CBaseObject::Export([[maybe_unused]] const QString& levelPath, XmlNodeRef& xmlNode)
{
    XmlNodeRef objNode = xmlNode->newChild("Object");

    objNode->setAttr("Type", GetTypeName().toUtf8().data());
    objNode->setAttr("Name", GetName().toUtf8().data());

    Vec3 pos, scale;
    Quat rotate;
    if (m_parent)
    {
        // Export world coordinates.
        AffineParts ap;
        ap.SpectralDecompose(GetWorldTM());

        pos = ap.pos;
        rotate = ap.rot;
        scale = ap.scale;
    }
    else
    {
        pos = m_pos;
        rotate = m_rotate;
        scale = m_scale;
    }

    if (!IsEquivalent(pos, Vec3(0, 0, 0), 0))
    {
        objNode->setAttr("Pos", pos);
    }

    if (!rotate.IsIdentity())
    {
        objNode->setAttr("Rotate", rotate);
    }

    if (!IsEquivalent(scale, Vec3(0, 0, 0), 0))
    {
        objNode->setAttr("Scale", scale);
    }

    // Save variables.
    CVarObject::Serialize(objNode, false);

    return objNode;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CBaseObject::FindObject(REFGUID id) const
{
    return GetObjectManager()->FindObject(id);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::StoreUndo(bool minimal, int flags)
{
    // Don't use Sandbox undo for AZ entities, except for the move & scale tools, which rely on it.
    const bool isGizmoTool = 0 != (flags & (eObjectUpdateFlags_MoveTool | eObjectUpdateFlags_ScaleTool | eObjectUpdateFlags_UserInput));
    if (!isGizmoTool && 0 != (m_flags & OBJFLAG_DONT_SAVE))
    {
        return;
    }

    if (CUndo::IsRecording())
    {
        if (minimal)
        {
            CUndo::Record(new CUndoBaseObjectMinimal(this, flags));
        }
        else
        {
            CUndo::Record(new CUndoBaseObject(this));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
QString CBaseObject::GetTypeName() const
{
    QString className = m_classDesc->ClassName();
    QString subClassName = strstr(className.toUtf8().data(), "::");
    if (subClassName.isEmpty())
    {
        return className;
    }

    QString name;
    name.append(className.midRef(0, className.length() - subClassName.length()));
    return name;
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IntersectRectBounds(const AABB& bbox)
{
    AABB aabb;
    GetBoundBox(aabb);

    return aabb.IsIntersectBox(bbox);
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IntersectRayBounds(const Ray& ray)
{
    Vec3 tmpPnt;
    AABB aabb;
    GetBoundBox(aabb);

    return Intersect::Ray_AABB(ray, aabb, tmpPnt);
}

//////////////////////////////////////////////////////////////////////////
namespace
{
    using Edge2D = std::pair<Vec2, Vec2>;
}
bool IsIncludePointsInConvexHull(Edge2D* pEdgeArray0, int nEdgeArray0Size, Edge2D* pEdgeArray1, int nEdgeArray1Size)
{
    if (!pEdgeArray0 || !pEdgeArray1 || nEdgeArray0Size <= 0 || nEdgeArray1Size <= 0)
    {
        return false;
    }

    static const float kPointEdgeMaxInsideDistance(0.05f);

    bool bInside(true);
    for (int i = 0; i < nEdgeArray0Size; ++i)
    {
        const Vec2& v(pEdgeArray0[i].first);
        bInside = true;
        for (int k = 0; k < nEdgeArray1Size; ++k)
        {
            Vec2 v0 = pEdgeArray1[k].first;
            Vec2 v1 = pEdgeArray1[k].second;
            Vec3 direction(v1.x - v0.x, v1.y - v0.y, 0);
            Vec3 up(0, 0, 1);
            Vec3 z = up.Cross(direction);
            Vec2 normal;
            normal.x = z.x;
            normal.y = z.y;
            normal.Normalize();
            float distance = -normal.Dot(v0);
            if (normal.Dot(v) + distance > kPointEdgeMaxInsideDistance)
            {
                bInside = false;
                break;
            }
        }
        if (bInside)
        {
            break;
        }
    }
    return bInside;
}

void ModifyConvexEdgeDirection(Edge2D* pEdgeArray, int nEdgeArraySize)
{
    if (!pEdgeArray || nEdgeArraySize < 2)
    {
        return;
    }
    Vec3 v0(pEdgeArray[0].first.x - pEdgeArray[0].second.x, pEdgeArray[0].first.y - pEdgeArray[0].second.y, 0);
    Vec3 v1(pEdgeArray[1].second.x - pEdgeArray[1].first.x, pEdgeArray[1].second.y - pEdgeArray[1].first.y, 0);
    Vec3 vCross = v0.Cross(v1);
    if (vCross.z < 0)
    {
        for (int i = 0; i < nEdgeArraySize; ++i)
        {
            std::swap(pEdgeArray[i].first, pEdgeArray[i].second);
        }
    }
}

bool CBaseObject::HitTestRectBounds(HitContext& hc, const AABB& box)
{
    if (hc.bUseSelectionHelpers)
    {
        if (IsSkipSelectionHelper())
        {
            return false;
        }
    }

    static const int kNumberOfBoundBoxPt(8);

    // transform all 8 vertices into view space.
    QPoint p[kNumberOfBoundBoxPt] =
    {
        hc.view->WorldToView(Vec3(box.min.x, box.min.y, box.min.z)),
        hc.view->WorldToView(Vec3(box.min.x, box.max.y, box.min.z)),
        hc.view->WorldToView(Vec3(box.max.x, box.min.y, box.min.z)),
        hc.view->WorldToView(Vec3(box.max.x, box.max.y, box.min.z)),
        hc.view->WorldToView(Vec3(box.min.x, box.min.y, box.max.z)),
        hc.view->WorldToView(Vec3(box.min.x, box.max.y, box.max.z)),
        hc.view->WorldToView(Vec3(box.max.x, box.min.y, box.max.z)),
        hc.view->WorldToView(Vec3(box.max.x, box.max.y, box.max.z))
    };

    QRect objrc;
    objrc.setLeft(10000);
    objrc.setTop(10000);
    objrc.setRight(-10000);
    objrc.setBottom(-10000);

    // find new min/max values
    for (int i = 0; i < 8; i++)
    {
        objrc.setLeft(min(objrc.left(), p[i].x()));
        objrc.setRight(max(objrc.right(), p[i].x()));
        objrc.setTop(min(objrc.top(), p[i].y()));
        objrc.setBottom(max(objrc.bottom(), p[i].y()));
    }
    if (objrc.isEmpty())
    {
        // Make objrc at least of size 1.
        objrc.moveBottomRight(objrc.bottomRight() + QPoint(1, 1));
    }

    if (hc.rect.contains(objrc.topLeft()) &&
        hc.rect.contains(objrc.bottomLeft()) &&
        hc.rect.contains(objrc.topRight()) &&
        hc.rect.contains(objrc.bottomRight()))
    {
        hc.object = this;
        return true;
    }

    if (objrc.intersects(hc.rect))
    {
        AABB localAABB;
        GetLocalBounds(localAABB);
        CBaseObject* pOldObj = hc.object;
        hc.object = this;
        if (localAABB.IsEmpty())
        {
            return true;
        }

        const int kMaxSizeOfEdgeList0(4);
        Edge2D edgelist0[kMaxSizeOfEdgeList0] = {
            Edge2D(Vec2(static_cast<f32>(hc.rect.left()),  static_cast<f32>(hc.rect.top())),    Vec2(static_cast<f32>(hc.rect.right()), static_cast<f32>(hc.rect.top()))),
            Edge2D(Vec2(static_cast<f32>(hc.rect.right()), static_cast<f32>(hc.rect.top())),    Vec2(static_cast<f32>(hc.rect.right()), static_cast<f32>(hc.rect.bottom()))),
            Edge2D(Vec2(static_cast<f32>(hc.rect.right()), static_cast<f32>(hc.rect.bottom())), Vec2(static_cast<f32>(hc.rect.left()),  static_cast<f32>(hc.rect.bottom()))),
            Edge2D(Vec2(static_cast<f32>(hc.rect.left()),  static_cast<f32>(hc.rect.bottom())), Vec2(static_cast<f32>(hc.rect.left()),  static_cast<f32>(hc.rect.top())))
        };

        const int kMaxSizeOfEdgeList1(8);
        Edge2D edgelist1[kMaxSizeOfEdgeList1];
        int nEdgeList1Count(kMaxSizeOfEdgeList1);

        const Matrix34& worldTM(GetWorldTM());
        OBB obb = OBB::CreateOBBfromAABB(Matrix33(worldTM), localAABB);
        Vec3 ax = obb.m33.GetColumn0() * obb.h.x;
        Vec3 ay = obb.m33.GetColumn1() * obb.h.y;
        Vec3 az = obb.m33.GetColumn2() * obb.h.z;
        QPoint obb_p[kMaxSizeOfEdgeList1] =
        {
            hc.view->WorldToView(-ax - ay - az + worldTM.GetTranslation()),
            hc.view->WorldToView(-ax - ay + az + worldTM.GetTranslation()),
            hc.view->WorldToView(-ax + ay - az + worldTM.GetTranslation()),
            hc.view->WorldToView(-ax + ay + az + worldTM.GetTranslation()),
            hc.view->WorldToView(ax - ay - az + worldTM.GetTranslation()),
            hc.view->WorldToView(ax - ay + az + worldTM.GetTranslation()),
            hc.view->WorldToView(ax + ay - az + worldTM.GetTranslation()),
            hc.view->WorldToView(ax + ay + az + worldTM.GetTranslation())
        };

        std::vector<Vec3> pointsForRegion1;
        pointsForRegion1.reserve(kMaxSizeOfEdgeList1);
        for (int i = 0; i < kMaxSizeOfEdgeList1; ++i)
        {
            pointsForRegion1.push_back(Vec3(static_cast<f32>(obb_p[i].x()), static_cast<f32>(obb_p[i].y()), 0.0f));
        }

        std::vector<Vec3> convexHullForRegion1;
        ConvexHull2D(convexHullForRegion1, pointsForRegion1);
        nEdgeList1Count = static_cast<int>(convexHullForRegion1.size());
        if (nEdgeList1Count < 3 || nEdgeList1Count > kMaxSizeOfEdgeList1)
        {
            return true;
        }

        for (int i = 0; i < nEdgeList1Count; ++i)
        {
            int iNextI = (i + 1) % nEdgeList1Count;
            edgelist1[i] = Edge2D(Vec2(convexHullForRegion1[i].x, convexHullForRegion1[i].y), Vec2(convexHullForRegion1[iNextI].x, convexHullForRegion1[iNextI].y));
        }

        ModifyConvexEdgeDirection(edgelist0, kMaxSizeOfEdgeList0);
        ModifyConvexEdgeDirection(edgelist1, nEdgeList1Count);

        bool bInside = IsIncludePointsInConvexHull(edgelist0, kMaxSizeOfEdgeList0, edgelist1, nEdgeList1Count);
        if (!bInside)
        {
            bInside = IsIncludePointsInConvexHull(edgelist1, nEdgeList1Count, edgelist0, kMaxSizeOfEdgeList0);
        }
        if (!bInside)
        {
            hc.object = pOldObj;
            return false;
        }

        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::HitTestRect(HitContext& hc)
{
    AZ_PROFILE_FUNCTION(Editor);

    AABB box;

    if (hc.bUseSelectionHelpers)
    {
        if (IsSkipSelectionHelper())
        {
            return false;
        }
        box.min = GetWorldPos();
        box.max = box.min;
    }
    else
    {
        // Retrieve world space bound box.
        GetBoundBox(box);
    }

    bool bHit = HitTestRectBounds(hc, box);
    m_bInSelectionBox = bHit;
    if (bHit)
    {
        hc.object = this;
    }
    return bHit;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CBaseObject::GetChild(size_t const i) const
{
    assert(i < m_childs.size());
    return m_childs[i];
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::AttachChild(CBaseObject* child, bool bKeepPos)
{
    Matrix34 childTM;
    ITransformDelegate* pTransformDelegate;
    ITransformDelegate* pChildTransformDelegate;

    {
        CScopedSuspendUndo suspendUndo;

        assert(child);
        if (!child || child == GetLookAt())
        {
            return;
        }

        child->NotifyListeners(bKeepPos ? ON_PREATTACHEDKEEPXFORM : ON_PREATTACHED);

        pTransformDelegate = m_pTransformDelegate;
        pChildTransformDelegate = child->m_pTransformDelegate;
        SetTransformDelegate(nullptr);
        child->SetTransformDelegate(nullptr);

        if (bKeepPos)
        {
            child->InvalidateTM(0);
            childTM = child->GetWorldTM();
        }

        // If not already attached to this node.
        if (child->m_parent == this)
        {
            return;
        }

        // Add to child list first to make sure node not get deleted while reattaching.
        m_childs.push_back(child);
        if (child->m_parent)
        {
            child->DetachThis(bKeepPos);    // Detach node if attached to other parent.
        }
        //////////////////////////////////////////////////////////////////////////
        child->m_parent = this; // Assign this node as parent to child node.
    }

    {
        CScopedSuspendUndo suspendUndo;

        {
            if (bKeepPos)
            {
                child->SetWorldTM(childTM);
            }

            child->InvalidateTM(0);
        }

        m_pTransformDelegate = pTransformDelegate;
        child->m_pTransformDelegate = pChildTransformDelegate;

        child->NotifyListeners(ON_ATTACHED);

        NotifyListeners(ON_CHILDATTACHED);
    }

    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoAttachBaseObject(child, bKeepPos, true));
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DetachAll(bool bKeepPos)
{
    while (!m_childs.empty())
    {
        CBaseObject* child = *m_childs.begin();
        child->DetachThis(bKeepPos);
        NotifyListeners(ON_CHILDDETACHED);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DetachThis(bool bKeepPos)
{
    if (m_parent)
    {
        if (CUndo::IsRecording())
        {
            CUndo::Record(new CUndoAttachBaseObject(this, bKeepPos, false));
        }

        Matrix34 worldTM;
        ITransformDelegate* pTransformDelegate;

        {
            CScopedSuspendUndo suspendUndo;
            NotifyListeners(bKeepPos ? ON_PREDETACHEDKEEPXFORM : ON_PREDETACHED);

            pTransformDelegate = m_pTransformDelegate;
            SetTransformDelegate(nullptr);

            if (bKeepPos)
            {
                ITransformDelegate* pParentTransformDelegate = m_parent->m_pTransformDelegate;
                m_parent->SetTransformDelegate(nullptr);
                worldTM = GetWorldTM();
                m_parent->SetTransformDelegate(pParentTransformDelegate);
            }
        }

        {
            CScopedSuspendUndo suspendUndo;

            // Copy parent to temp var, erasing child from parent may delete this node if child referenced only from parent.
            CBaseObject* parent = m_parent;
            m_parent = nullptr;
            parent->RemoveChild(this);

            if (bKeepPos)
            {
                // Keep old world space transformation.
                SetWorldTM(worldTM);
            }

            SetTransformDelegate(pTransformDelegate);

            NotifyListeners(ON_DETACHED);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::RemoveChild(CBaseObject* node)
{
    Childs::iterator it = std::find(m_childs.begin(), m_childs.end(), node);
    if (it != m_childs.end())
    {
        m_childs.erase(it);
        NotifyListeners(ON_CHILDDETACHED);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::ResolveParent(CBaseObject* parent)
{
    // even though parent is same as m_parent, adding the member to the parent must be done.
    if (parent)
    {
        parent->AddMember(this, false);
    }
    else
    {
        DetachThis(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::CalcLocalTM(Matrix34& tm) const
{
    tm.SetIdentity();

    if (m_lookat)
    {
        Vec3 pos = GetPos();

        if (m_parent)
        {
            // Get our world position.
            pos = GetParentAttachPointWorldTM().TransformPoint(pos);
        }

        // Calculate local TM matrix differently.
        Vec3 lookatPos = m_lookat->GetWorldPos();
        if (lookatPos == pos) // if target at object pos
        {
            tm.SetTranslation(pos);
        }
        else
        {
            tm = Matrix34(Matrix33::CreateRotationVDir((lookatPos - pos).GetNormalized()), pos);
        }
        if (m_parent)
        {
            Matrix34 invParentTM = m_parent->GetWorldTM();
            invParentTM.Invert();
            tm = invParentTM * tm;
        }
    }
    else
    {
        tm = Matrix34::Create(GetScale(), GetRotation(), GetPos());
    }
}

//////////////////////////////////////////////////////////////////////////
const Matrix34& CBaseObject::GetWorldTM() const
{
    if (!m_bMatrixValid)
    {
        m_worldTM = GetLocalTM();
        m_bMatrixValid = true;
        m_bMatrixInWorldSpace = false;
        m_bWorldBoxValid = false;
    }
    if (!m_bMatrixInWorldSpace)
    {
        CBaseObject* parent = GetParent();
        if (parent)
        {
            m_worldTM = GetParentAttachPointWorldTM() * m_worldTM;
        }
        m_bMatrixInWorldSpace = true;
        m_bWorldBoxValid = false;
    }
    return m_worldTM;
}

//////////////////////////////////////////////////////////////////////////
Matrix34 CBaseObject::GetParentAttachPointWorldTM() const
{
    CBaseObject* pParent = GetParent();
    if (pParent)
    {
        return pParent->GetWorldTM();
    }

    return Matrix34(IDENTITY);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::InvalidateTM([[maybe_unused]] int flags)
{
    bool bMatrixWasValid = m_bMatrixValid;

    m_bMatrixInWorldSpace = false;
    m_bMatrixValid = false;
    m_bWorldBoxValid = false;

    // If matrix was valid, invalidate all childs.
    if (bMatrixWasValid)
    {
        if (m_lookatSource)
        {
            m_lookatSource->InvalidateTM(eObjectUpdateFlags_ParentChanged);
        }

        // Invalidate matrices off all child objects.
        for (int i = 0; i < m_childs.size(); i++)
        {
            if (m_childs[i] != nullptr && m_childs[i]->m_bMatrixValid)
            {
                m_childs[i]->InvalidateTM(eObjectUpdateFlags_ParentChanged);
            }
        }
        NotifyListeners(ON_TRANSFORM);

        // Notify parent that we were modified.
        if (m_parent)
        {
            m_parent->OnChildModified();
        }
    }

    if (m_pTransformDelegate)
    {
        m_pTransformDelegate->MatrixInvalidated();
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetLocalTM(const Vec3& pos, const Quat& rotate, const Vec3& scale, int flags)
{
    const bool b1 = SetPos(pos, flags | eObjectUpdateFlags_DoNotInvalidate);
    const bool b2 = SetRotation(rotate, flags | eObjectUpdateFlags_DoNotInvalidate);
    const bool b3 = SetScale(scale, flags | eObjectUpdateFlags_DoNotInvalidate);

    if (b1 || b2 || b3 || flags == eObjectUpdateFlags_Animated)
    {
        flags = (b1) ? (flags | eObjectUpdateFlags_PositionChanged) : (flags & (~eObjectUpdateFlags_PositionChanged));
        flags = (b2) ? (flags | eObjectUpdateFlags_RotationChanged) : (flags & (~eObjectUpdateFlags_RotationChanged));
        flags = (b3) ? (flags | eObjectUpdateFlags_ScaleChanged) : (flags & (~eObjectUpdateFlags_ScaleChanged));
        InvalidateTM(flags);
    }
}


//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetLocalTM(const Matrix34& tm, int flags)
{
    if (m_lookat)
    {
        bool b1 = SetPos(tm.GetTranslation(), eObjectUpdateFlags_DoNotInvalidate);
        flags = (b1) ? (flags | eObjectUpdateFlags_PositionChanged) : (flags & (~eObjectUpdateFlags_PositionChanged));
        InvalidateTM(flags);
    }
    else
    {
        AffineParts affineParts;
        affineParts.SpectralDecompose(tm);
        SetLocalTM(affineParts.pos, affineParts.rot, affineParts.scale, flags);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetWorldPos(const Vec3& pos, int flags)
{
    if (GetParent())
    {
        Matrix34 invParentTM = GetParentAttachPointWorldTM();
        invParentTM.Invert();
        Vec3 posLocal = invParentTM * pos;
        SetPos(posLocal, flags);
    }
    else
    {
        SetPos(pos, flags);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetWorldTM(const Matrix34& tm, int flags)
{
    if (GetParent())
    {
        Matrix34 invParentTM = GetParentAttachPointWorldTM();
        invParentTM.Invert();
        Matrix34 localTM = invParentTM * tm;
        SetLocalTM(localTM, flags);
    }
    else
    {
        SetLocalTM(tm, flags);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::UpdateVisibility(bool bVisible)
{
    if (bVisible == CheckFlags(OBJFLAG_INVISIBLE))
    {
        if (IObjectManager* objectManager = GetObjectManager())
        {
            objectManager->InvalidateVisibleList();
        }

        if (!bVisible)
        {
            m_flags |= OBJFLAG_INVISIBLE;
        }
        else
        {
            m_flags &= ~OBJFLAG_INVISIBLE;
        }

        NotifyListeners(ON_VISIBILITY);

    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetLookAt(CBaseObject* target)
{
    if (m_lookat == target)
    {
        return;
    }

    StoreUndo();

    if (m_lookat)
    {
        // Unbind current lookat.
        m_lookat->m_lookatSource = nullptr;
    }
    m_lookat = target;
    if (m_lookat)
    {
        m_lookat->m_lookatSource = this;
    }

    InvalidateTM(0);
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IsLookAtTarget() const
{
    return m_lookatSource != nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::AddEventListener(EventListener* listener)
{
    if (find(m_eventListeners.begin(), m_eventListeners.end(), listener) == m_eventListeners.end())
    {
        m_eventListeners.push_back(listener);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::RemoveEventListener(EventListener* listener)
{
    std::vector<EventListener*>::iterator listenerFound = find(m_eventListeners.begin(), m_eventListeners.end(), listener);
    if (listenerFound != m_eventListeners.end())
    {
        (*listenerFound) = nullptr;
    }
}

//////////////////////////////////////////////////////////////////////////
bool IsBaseObjectEventCallbackNULL(CBaseObject::EventListener* listener) { return listener == nullptr; }

//////////////////////////////////////////////////////////////////////////
void CBaseObject::NotifyListeners(EObjectListenerEvent event)
{
    for (auto it = m_eventListeners.begin(); it != m_eventListeners.end(); ++it)
    {
        // Call listener callback.
        if (*it)
        {
            (*it)->OnObjectEvent(this, event);
        }
    }

    m_eventListeners.erase(remove_if(m_eventListeners.begin(), m_eventListeners.end(), IsBaseObjectEventCallbackNULL), std::end(m_eventListeners));
}
//////////////////////////////////////////////////////////////////////////
bool CBaseObject::ConvertFromObject(CBaseObject* object)
{
    SetLocalTM(object->GetLocalTM());
    SetName(object->GetName());
    SetColor(object->GetColor());
    m_flattenArea = object->m_flattenArea;
    if (object->GetParent())
    {
        object->GetParent()->AttachChild(this);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IsPotentiallyVisible() const
{
    if (CheckFlags(OBJFLAG_HIDDEN))
    {
        return false;
    }
    if (gSettings.objectHideMask & GetType())
    {
        return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
//! Analyze errors for this object.
void CBaseObject::Validate(IErrorReport* report)
{
    // Checks for invalid values in base object.

    //////////////////////////////////////////////////////////////////////////
    // Check if position is bad.
    const Vec3 pos = GetPos();

    if (fabs(pos.x) > AZ::Constants::MaxFloatBeforePrecisionLoss ||
        fabs(pos.y) > AZ::Constants::MaxFloatBeforePrecisionLoss ||
        fabs(pos.z) > AZ::Constants::MaxFloatBeforePrecisionLoss)
    {
        // File Not found.
        CErrorRecord err;
        err.error = QStringLiteral("Object %1 have invalid position (%2,%3,%4)").arg(GetName()).arg(pos.x).arg(pos.y).arg(pos.z);
        err.pObject = this;
        report->ReportError(err);
    }
    //////////////////////////////////////////////////////////////////////////

    float minScale = 0.01f;
    float maxScale = 1000.0f;
    //////////////////////////////////////////////////////////////////////////
    // Check if position is bad.
    if (m_scale.x < minScale || m_scale.x > maxScale ||
        m_scale.y < minScale || m_scale.y > maxScale ||
        m_scale.z < minScale || m_scale.z > maxScale)
    {
        // File Not found.
        CErrorRecord err;
        err.error = QStringLiteral("Object %1 have invalid scale (%2,%3,%4)").arg(GetName()).arg(m_scale.x).arg(m_scale.y).arg(m_scale.z);
        err.pObject = this;
        report->ReportError(err);
    }
    //////////////////////////////////////////////////////////////////////////
};

//////////////////////////////////////////////////////////////////////////
void CBaseObject::GatherUsedResources(CUsedResources& resources)
{
    if (GetVarBlock())
    {
        GetVarBlock()->GatherUsedResources(resources);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IsSimilarObject(CBaseObject* pObject)
{
    if (pObject->GetClassDesc() == GetClassDesc() && pObject->metaObject() == metaObject())
    {
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
EScaleWarningLevel CBaseObject::GetScaleWarningLevel() const
{
    EScaleWarningLevel scaleWarningLevel = eScaleWarningLevel_None;

    const float kScaleThreshold = 0.001f;

    if (fabs(m_scale.x - 1.0f) > kScaleThreshold
        || fabs(m_scale.y - 1.0f) > kScaleThreshold
        || fabs(m_scale.z - 1.0f) > kScaleThreshold)
    {
        if (fabs(m_scale.x - m_scale.y) < kScaleThreshold
            && fabs(m_scale.y - m_scale.z) < kScaleThreshold)
        {
            scaleWarningLevel = eScaleWarningLevel_Rescaled;
        }
        else
        {
            scaleWarningLevel = eScaleWarningLevel_RescaledNonUniform;
        }
    }

    return scaleWarningLevel;
}

//////////////////////////////////////////////////////////////////////////
ERotationWarningLevel CBaseObject::GetRotationWarningLevel() const
{
    ERotationWarningLevel rotationWarningLevel = eRotationWarningLevel_None;

    const float kRotationThreshold = 0.01f;

    Ang3 eulerRotation = Ang3(GetRotation());

    if (fabs(eulerRotation.x) > kRotationThreshold
        || fabs(eulerRotation.y) > kRotationThreshold
        || fabs(eulerRotation.z) > kRotationThreshold)
    {
        const float xRotationMod = fabs(fmod(eulerRotation.x, gf_PI / 2.0f));
        const float yRotationMod = fabs(fmod(eulerRotation.y, gf_PI / 2.0f));
        const float zRotationMod = fabs(fmod(eulerRotation.z, gf_PI / 2.0f));

        if ((xRotationMod < kRotationThreshold || xRotationMod > (gf_PI / 2.0f - kRotationThreshold))
            && (yRotationMod < kRotationThreshold || yRotationMod > (gf_PI / 2.0f - kRotationThreshold))
            && (zRotationMod < kRotationThreshold || zRotationMod > (gf_PI / 2.0f - kRotationThreshold)))
        {
            rotationWarningLevel = eRotationWarningLevel_Rotated;
        }
        else
        {
            rotationWarningLevel = eRotationWarningLevel_RotatedNonRectangular;
        }
    }

    return rotationWarningLevel;
}

bool CBaseObject::IsSkipSelectionHelper() const
{
    return false;
}

bool CBaseObject::CanBeHightlighted() const
{
    return true;
}

Matrix33 CBaseObject::GetWorldRotTM() const
{
    if (GetParent())
    {
        return GetParent()->GetWorldRotTM() * Matrix33(GetRotation());
    }
    return Matrix33(GetRotation());
}

Matrix33 CBaseObject::GetWorldScaleTM() const
{
    if (GetParent())
    {
        return GetParent()->GetWorldScaleTM() * Matrix33::CreateScale(GetScale());
    }
    return Matrix33::CreateScale(GetScale());
}

void CBaseObject::SetTransformDelegate(ITransformDelegate* pTransformDelegate)
{
    m_pTransformDelegate = pTransformDelegate;
    InvalidateTM(0);
}

void CBaseObject::AddMember(CBaseObject* pMember, bool bKeepPos /*=true */)
{
    AttachChild(pMember, bKeepPos);
}

void CBaseObject::OnBeforeAreaChange()
{
    AABB aabb;
    GetBoundBox(aabb);
    GetIEditor()->GetGameEngine()->OnAreaModified(aabb);
}

#include <Objects/moc_BaseObject.cpp>
