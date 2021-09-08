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
#include "GizmoManager.h"
#include "Include/IIconManager.h"
#include "Objects/SelectionGroup.h"
#include "Objects/ObjectManager.h"
#include "ViewManager.h"
#include "IEditorImpl.h"
#include "GameEngine.h"
#include <IEntityRenderState.h>
#include <IStatObj.h>
// To use the Andrew's algorithm in order to make convex hull from the points, this header is needed.
#include "Util/GeometryUtil.h"

namespace {
    QColor kLinkColorParent = QColor(0, 255, 255);
    QColor kLinkColorChild = QColor(0, 0, 255);
    QColor kLinkColorGray = QColor(128, 128, 128);
}

extern CObjectManager* g_pObjectManager;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//! Undo object for CBaseObject.
class CUndoBaseObject
    : public IUndoObject
{
public:
    CUndoBaseObject(CBaseObject* pObj, const char* undoDescription);

protected:
    int GetSize() override { return sizeof(*this);   }
    QString GetDescription() override { return m_undoDescription; };
    QString GetObjectName() override;

    void Undo(bool bUndo) override;
    void Redo() override;

protected:
    QString m_undoDescription;
    GUID m_guid;
    XmlNodeRef m_undo;
    XmlNodeRef m_redo;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//! Undo object for CBaseObject that only stores its transform, color, area and minSpec
class CUndoBaseObjectMinimal
    : public IUndoObject
{
public:
    CUndoBaseObjectMinimal(CBaseObject* obj, const char* undoDescription, int flags);

protected:
    int GetSize() override { return sizeof(*this); }
    QString GetDescription() override { return m_undoDescription; };
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
        int minSpec;
    };

    void SetTransformsFromState(CBaseObject* pObject, const StateStruct& state, bool bUndo);

    GUID m_guid;
    QString m_undoDescription;
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
    QString GetDescription() override { return "Attachment Changed"; }

    GUID m_attachedObjectGUID;
    GUID m_parentObjectGUID;
    bool m_bKeepPos;
    bool m_bAttach;
};

//////////////////////////////////////////////////////////////////////////
CUndoBaseObject::CUndoBaseObject(CBaseObject* obj, const char* undoDescription)
{
    // Stores the current state of this object.
    assert(obj != 0);
    m_undoDescription = undoDescription;
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
CUndoBaseObjectMinimal::CUndoBaseObjectMinimal(CBaseObject* pObj, const char* undoDescription, [[maybe_unused]] int flags)
{
    // Stores the current state of this object.
    assert(pObj != nullptr);
    m_undoDescription = undoDescription;
    m_guid = pObj->GetId();

    ZeroStruct(m_redoState);

    m_undoState.pos = pObj->GetPos();
    m_undoState.rotate = pObj->GetRotation();
    m_undoState.scale = pObj->GetScale();
    m_undoState.color = pObj->GetColor();
    m_undoState.area = pObj->GetArea();
    m_undoState.minSpec = pObj->GetMinSpec();
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
    if (!pObject || pObject->GetType() == OBJTYPE_DUMMY)
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
        m_redoState.minSpec = pObject->GetMinSpec();
    }

    SetTransformsFromState(pObject, m_undoState, bUndo);

    pObject->ChangeColor(m_undoState.color);
    pObject->SetArea(m_undoState.area);
    pObject->SetMinSpec(m_undoState.minSpec, false);

    using namespace AzToolsFramework;
    ComponentEntityObjectRequestBus::Event(pObject, &ComponentEntityObjectRequestBus::Events::UpdatePreemptiveUndoCache);
}

//////////////////////////////////////////////////////////////////////////
void CUndoBaseObjectMinimal::Redo()
{
    CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(m_guid);
    if (!pObject || pObject->GetType() == OBJTYPE_DUMMY)
    {
        return;
    }

    SetTransformsFromState(pObject, m_redoState, true);

    pObject->ChangeColor(m_redoState.color);
    pObject->SetArea(m_redoState.area);
    pObject->SetMinSpec(m_redoState.minSpec, false);

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
    , m_floorNumber(-1)
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
    , m_nMaterialLayersMask(0)
    , m_nMinSpec(0)
    , m_vDrawIconPos(0, 0, 0)
    , m_nIconFlags(0)
    , m_hideOrder(CBaseObject::s_invalidHiddenID)
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
        m_nMaterialLayersMask = prev->m_nMaterialLayersMask;
        SetMinSpec(prev->GetMinSpec(), false);

        // Copy all basic variables.
        EnableUpdateCallbacks(false);
        CopyVariableValues(prev);
        EnableUpdateCallbacks(true);
        OnSetValues();
    }

    m_nTextureIcon = m_classDesc->GetTextureIconId();
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

    StoreUndo("Name");

    // Notification is expensive and not required if this is during construction.
    bool notify = (!m_name.isEmpty());

    m_name = name;
    GetObjectManager()->RegisterObjectName(name);
    SetModified(false);

    if (notify)
    {
        NotifyListeners(ON_RENAME);
        static_cast<CObjectManager*>(GetIEditor()->GetObjectManager())->NotifyObjectListeners(this, ON_RENAME);
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
QString CBaseObject::GetWarningsText() const
{
    QString warnings;

    if (gSettings.viewports.bShowScaleWarnings)
    {
        const EScaleWarningLevel scaleWarningLevel = GetScaleWarningLevel();
        if (scaleWarningLevel == eScaleWarningLevel_Rescaled)
        {
            warnings += "\\n  Warning: Object Scale is not 100%.";
        }
        else if (scaleWarningLevel == eScaleWarningLevel_RescaledNonUniform)
        {
            warnings += "\\n  Warning: Object has non-uniform scale.";
        }
    }

    if (gSettings.viewports.bShowRotationWarnings)
    {
        const ERotationWarningLevel rotationWarningLevel = GetRotationWarningLevel();

        if (rotationWarningLevel == eRotationWarningLevel_Rotated)
        {
            warnings += "\\n  Warning: Object is rotated.";
        }
        else if (rotationWarningLevel == eRotationWarningLevel_RotatedNonRectangular)
        {
            warnings += "\\n  Warning: Object is rotated non-orthogonally.";
        }
    }

    return warnings;
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IsSameClass(CBaseObject* obj)
{
    return GetClassDesc() == obj->GetClassDesc();
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
        StoreUndo("Position", true, flags);
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
        StoreUndo("Rotate", true, flags);
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
        StoreUndo("Scale", true, flags);
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

    StoreUndo("Color", true);

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

    StoreUndo("Area", true);

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

void CBaseObject::DrawDefault(DisplayContext& dc, const QColor& labelColor)
{
    Vec3 wp = GetWorldPos();

    bool bDisplaySelectionHelper = false;
    if (!CanBeDrawn(dc, bDisplaySelectionHelper))
    {
        return;
    }

    // Draw link between parent and child.
    if (dc.flags & DISPLAY_LINKS)
    {
        if (GetParent())
        {
            dc.DrawLine(GetParentAttachPointWorldTM().GetTranslation(), wp, IsFrozen() ? kLinkColorGray : kLinkColorParent, IsFrozen() ? kLinkColorGray : kLinkColorChild);
        }
        size_t nChildCount = GetChildCount();
        for (size_t i = 0; i < nChildCount; ++i)
        {
            const CBaseObject* pChild = GetChild(i);
            dc.DrawLine(pChild->GetParentAttachPointWorldTM().GetTranslation(), pChild->GetWorldPos(), pChild->IsFrozen() ? kLinkColorGray : kLinkColorParent, pChild->IsFrozen() ? kLinkColorGray : kLinkColorChild);
        }
    }

    // Draw Bounding box
    if (dc.flags & DISPLAY_BBOX)
    {
        AABB box;
        GetBoundBox(box);
        dc.SetColor(Vec3(1, 1, 1));
        dc.DrawWireBox(box.min, box.max);
    }

    if (IsHighlighted())
    {
        DrawHighlight(dc);
    }

    if (IsSelected())
    {
        DrawArea(dc);

        CSelectionGroup* pSelection = GetObjectManager()->GetSelection();

        // If the number of selected object is over 2, the merged boundbox should be used to render the measurement axis.
        if (!pSelection || (pSelection && pSelection->GetCount() == 1))
        {
            DrawDimensions(dc);
        }
    }

    if (bDisplaySelectionHelper)
    {
        DrawSelectionHelper(dc, wp, labelColor, 1.0f);
    }
    else if (!(dc.flags & DISPLAY_HIDENAMES))
    {
        DrawLabel(dc, wp, labelColor);
    }

    SetDrawTextureIconProperties(dc, wp);
    DrawTextureIcon(dc, wp);
    DrawWarningIcons(dc, wp);
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
void CBaseObject::SetDrawTextureIconProperties(DisplayContext& dc, const Vec3& pos, float alpha, int texIconFlags)
{
    if (gSettings.viewports.bShowIcons || gSettings.viewports.bShowSizeBasedIcons)
    {
        if (IsHighlighted())
        {
            dc.SetColor(QColor(255, 120, 0), 0.8f * alpha);
        }
        else if (IsSelected())
        {
            dc.SetSelectedColor(alpha);
        }
        else if (IsFrozen())
        {
            dc.SetFreezeColor();
        }
        else
        {
            dc.SetColor(QColor(255, 255, 255), alpha);
        }

        m_vDrawIconPos = pos;

        int nIconFlags = texIconFlags;
        if (CheckFlags(OBJFLAG_SHOW_ICONONTOP))
        {
            Vec3 objectPos = GetWorldPos();

            AABB box;
            GetBoundBox(box);
            m_vDrawIconPos.z = (m_vDrawIconPos.z - objectPos.z) + box.max.z;
            nIconFlags |= DisplayContext::TEXICON_ALIGN_BOTTOM;
        }
        m_nIconFlags = nIconFlags;
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DrawTextureIcon(DisplayContext& dc, [[maybe_unused]] const Vec3& pos, [[maybe_unused]] float alpha)
{
    if (m_nTextureIcon && (gSettings.viewports.bShowIcons || gSettings.viewports.bShowSizeBasedIcons))
    {
        dc.DrawTextureLabel(GetTextureIconDrawPos(), OBJECT_TEXTURE_ICON_SIZEX, OBJECT_TEXTURE_ICON_SIZEY, GetTextureIcon(), GetTextureIconFlags());
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DrawWarningIcons(DisplayContext& dc, const Vec3& pos)
{
    // Don't draw warning icons if they are beyond draw distance
    if ((dc.camera->GetPosition() - pos).GetLength() > gSettings.viewports.fWarningIconsDrawDistance)
    {
        return;
    }

    if (gSettings.viewports.bShowIcons || gSettings.viewports.bShowSizeBasedIcons)
    {
        const int warningIconSizeX = OBJECT_TEXTURE_ICON_SIZEX / 2;
        const int warningIconSizeY = OBJECT_TEXTURE_ICON_SIZEY / 2;

        const int iconOffsetX = m_nTextureIcon ? (-OBJECT_TEXTURE_ICON_SIZEX / 2) : 0;
        const int iconOffsetY = m_nTextureIcon ? (-OBJECT_TEXTURE_ICON_SIZEY / 2) : 0;

        if (gSettings.viewports.bShowScaleWarnings)
        {
            const EScaleWarningLevel scaleWarningLevel = GetScaleWarningLevel();

            if (scaleWarningLevel != eScaleWarningLevel_None)
            {
                dc.SetColor(QColor(255, scaleWarningLevel == eScaleWarningLevel_RescaledNonUniform ? 50 : 255, 50), 1.0f);
                dc.DrawTextureLabel(GetTextureIconDrawPos(), warningIconSizeX, warningIconSizeY,
                    GetIEditor()->GetIconManager()->GetIconTexture(eIcon_ScaleWarning), GetTextureIconFlags(),
                    -warningIconSizeX / 2, iconOffsetX - (warningIconSizeY / 2));
            }
        }

        if (gSettings.viewports.bShowRotationWarnings)
        {
            const ERotationWarningLevel rotationWarningLevel = GetRotationWarningLevel();
            if (rotationWarningLevel != eRotationWarningLevel_None)
            {
                dc.SetColor(QColor(255, rotationWarningLevel == eRotationWarningLevel_RotatedNonRectangular ? 50 : 255, 50), 1.0f);
                dc.DrawTextureLabel(GetTextureIconDrawPos(), warningIconSizeX, warningIconSizeY,
                    GetIEditor()->GetIconManager()->GetIconTexture(eIcon_RotationWarning), GetTextureIconFlags(),
                    warningIconSizeX / 2, iconOffsetY - (warningIconSizeY / 2));
            }
        }
    }
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

    float camDist = dc.camera->GetPosition().GetDistance(pos);
    float maxDist = dc.settings->GetLabelsDistance();
    if (camDist < dc.settings->GetLabelsDistance() || (dc.flags & DISPLAY_SELECTION_HELPERS))
    {
        float range = maxDist / 2.0f;
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
        else if (camDist > range)
        {
            col[3] = col[3] * (1.0f - (camDist - range) / range);
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
bool CBaseObject::IsInCameraView(const CCamera& camera)
{
    AABB bbox;
    GetBoundBox(bbox);
    return (camera.IsAABBVisible_F(AABB(bbox.min, bbox.max)));
}

//////////////////////////////////////////////////////////////////////////
float CBaseObject::GetCameraVisRatio(const CCamera& camera)
{
    AABB bbox;
    GetBoundBox(bbox);

    static const float defaultVisRatio = 1000.0f;

    const float objectHeightSq = max(1.0f, (bbox.max - bbox.min).GetLengthSquared());
    const float camdistSq = (bbox.min - camera.GetPosition()).GetLengthSquared();
    float visRatio = defaultVisRatio;
    if (camdistSq > FLT_EPSILON)
    {
        visRatio = objectHeightSq / camdistSq;
    }

    return visRatio;
}

//////////////////////////////////////////////////////////////////////////
int CBaseObject::MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    AZ_PROFILE_FUNCTION(Editor);

    if (event == eMouseMove || event == eMouseLDown)
    {
        Vec3 pos;
        if (GetIEditor()->GetAxisConstrains() != AXIS_TERRAIN)
        {
            pos = view->MapViewToCP(point);
        }
        else
        {
            // Snap to terrain.
            bool hitTerrain;
            pos = view->ViewToWorld(point, &hitTerrain);
            if (hitTerrain)
            {
                pos.z = GetIEditor()->GetTerrainElevation(pos.x, pos.y) + 1.0f;
            }
            pos = view->SnapToGrid(pos);
        }
        SetPos(pos);

        if (event == eMouseLDown)
        {
            return MOUSECREATE_OK;
        }
    }

    if (event == eMouseWheel)
    {
        float angle = 1;
        Quat rot = GetRotation();
        rot.SetRotationXYZ(Ang3(0.f, 0.f, rot.GetRotZ() + DEG2RAD(flags > 0 ? angle * (-1) : angle)));
        SetRotation(rot);
    }
    return MOUSECREATE_CONTINUE;
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
void CBaseObject::SetShared([[maybe_unused]] bool bShared)
{
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetHidden(bool bHidden, uint64 hiddenID, bool bAnimated)
{
    if (CheckFlags(OBJFLAG_HIDDEN) != bHidden)
    {
        if (!bAnimated)
        {
            StoreUndo("Hide Object");
        }

        if (bHidden)
        {
            SetFlags(OBJFLAG_HIDDEN);
        }
        else
        {
            ClearFlags(OBJFLAG_HIDDEN);
        }

        m_hideOrder = hiddenID;
        UpdateVisibility(!IsHidden());
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetFrozen(bool bFrozen)
{
    if (CheckFlags(OBJFLAG_FROZEN) != bFrozen)
    {
        StoreUndo("Freeze Object");
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
bool CBaseObject::IsHiddenBySpec() const
{
    if (!gSettings.bApplyConfigSpecInEditor)
    {
        return false;
    }

    return (m_nMinSpec != 0 && gSettings.editorConfigSpec != 0 && m_nMinSpec > static_cast<uint32>(gSettings.editorConfigSpec));
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
        if (ar.ShouldResetInternalMembers())
        {
            m_flags = 0;
            m_flattenArea = 0.0f;
            m_nMinSpec = 0;
            m_scale.Set(1.0f, 1.0f, 1.0f);
        }

        int flags = 0;
        int oldFlags = m_flags;

        QString name = m_name;
        QString mtlName;

        Vec3 pos = m_pos;
        Vec3 scale = m_scale;
        Quat quat = m_rotate;
        Ang3 angles(0, 0, 0);
        uint32 nMinSpec = m_nMinSpec;

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
        xmlNode->getAttr("MinSpec", nMinSpec);
        xmlNode->getAttr("FloorNumber", m_floorNumber);

        if (nMinSpec <= CONFIG_VERYHIGH_SPEC) // Ignore invalid values.
        {
            m_nMinSpec = nMinSpec;
        }

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

        if (ar.bUndo)
        {
            // If we are selected update UI Panel.
            xmlNode->getAttr("HideOrder", m_hideOrder);
        }

        // We reseted the min spec and deserialized it so set it internally
        if (ar.ShouldResetInternalMembers())
        {
            SetMinSpec(m_nMinSpec);
        }
    }
    else
    {
        // Saving.

        // This attributed only readed by ObjectManager.
        xmlNode->setAttr("Type", GetTypeName().toUtf8().data());

        xmlNode->setAttr("Id", m_guid);

        xmlNode->setAttr("Name", GetName().toUtf8().data());
        xmlNode->setAttr("HideOrder", m_hideOrder);

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

        xmlNode->setAttr("FloorNumber", m_floorNumber);

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

        if (m_nMinSpec != 0)
        {
            xmlNode->setAttr("MinSpec", (uint32)m_nMinSpec);
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

    if (m_nMinSpec != 0)
    {
        objNode->setAttr("MinSpec", (uint32)m_nMinSpec);
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
void CBaseObject::StoreUndo(const char* UndoDescription, bool minimal, int flags)
{
    if (m_objType == OBJTYPE_DUMMY)
    {
        return;
    }

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
            CUndo::Record(new CUndoBaseObjectMinimal(this, UndoDescription, flags));
        }
        else
        {
            CUndo::Record(new CUndoBaseObject(this, UndoDescription));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IsCreateGameObjects() const
{
    return GetObjectManager()->IsCreateGameObjects();
}

//////////////////////////////////////////////////////////////////////////
QString CBaseObject::GetTypeName() const
{
    if (m_objType == OBJTYPE_DUMMY)
    {
        return "";
    }
    QString className = m_classDesc->ClassName();
    QString subClassName = strstr(className.toUtf8().data(), "::");
    if (subClassName.isEmpty())
    {
        return className;
    }

    QString name;
    name.append(className.mid(0, className.length() - subClassName.length()));
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
bool CBaseObject::HitHelperTest(HitContext& hc)
{
    return HitHelperAtTest(hc, GetWorldPos());
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::HitHelperAtTest(HitContext& hc, const Vec3& pos)
{
    AZ_PROFILE_FUNCTION(Editor);

    bool bResult = false;

    if (m_nTextureIcon && (gSettings.viewports.bShowIcons || gSettings.viewports.bShowSizeBasedIcons) && !hc.bUseSelectionHelpers)
    {
        int iconSizeX = OBJECT_TEXTURE_ICON_SIZEX;
        int iconSizeY = OBJECT_TEXTURE_ICON_SIZEY;

        if (gSettings.viewports.bDistanceScaleIcons)
        {
            float fScreenScale = hc.view->GetScreenScaleFactor(pos);

            iconSizeX = static_cast<int>(static_cast<float>(iconSizeX) * OBJECT_TEXTURE_ICON_SCALE / fScreenScale);
            iconSizeY = static_cast<int>(static_cast<float>(iconSizeY) * OBJECT_TEXTURE_ICON_SCALE / fScreenScale);
        }

        // Hit Test icon of this object.
        Vec3 testPos = pos;
        int y0 = -(iconSizeY / 2);
        int y1 = +(iconSizeY / 2);
        if (CheckFlags(OBJFLAG_SHOW_ICONONTOP))
        {
            Vec3 objectPos = GetWorldPos();

            AABB box;
            GetBoundBox(box);
            testPos.z = (pos.z - objectPos.z) + box.max.z;
            y0 = -(iconSizeY);
            y1 = 0;
        }
        QPoint pnt = hc.view->WorldToView(testPos);

        if (hc.point2d.x() >= pnt.x() - (iconSizeX / 2) && hc.point2d.x() <= pnt.x() + (iconSizeX / 2) &&
            hc.point2d.y() >= pnt.y() + y0 && hc.point2d.y() <= pnt.y() + y1)
        {
            hc.dist = hc.raySrc.GetDistance(testPos) - 0.2f;
            hc.iconHit = true;
            bResult = true;
        }
    }
    else if (hc.bUseSelectionHelpers)
    {
        // Check potentially children first
        bResult = HitHelperTestForChildObjects(hc);

        // If no hit check this object
        if (!bResult)
        {
            // Hit test helper.
            Vec3 w = pos - hc.raySrc;
            w = hc.rayDir.Cross(w);
            float d = w.GetLengthSquared();

            static const float screenScaleToRadiusFactor = 0.008f;
            const float radius = hc.view->GetScreenScaleFactor(pos) * screenScaleToRadiusFactor;
            const float pickDistance = hc.raySrc.GetDistance(pos);
            if (d < radius * radius + hc.distanceTolerance && hc.dist >= pickDistance)
            {
                hc.dist = pickDistance;
                hc.object = this;
                bResult = true;
            }
        }
    }


    return bResult;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CBaseObject::GetChild(size_t const i) const
{
    assert(i < m_childs.size());
    return m_childs[i];
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IsChildOf(CBaseObject* node)
{
    CBaseObject* p = m_parent;
    while (p && p != node)
    {
        p = p->m_parent;
    }
    if (p == node)
    {
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
void CBaseObject::CloneChildren(CBaseObject* pFromObject)
{
    if (pFromObject == nullptr)
    {
        return;
    }

    for (size_t i = 0, nChildCount(pFromObject->GetChildCount()); i < nChildCount; ++i)
    {
        CBaseObject* pFromChildObject = pFromObject->GetChild(i);

        CBaseObject* pChildClone = GetObjectManager()->CloneObject(pFromChildObject);
        if (pChildClone == nullptr)
        {
            continue;
        }

        pChildClone->CloneChildren(pFromChildObject);
        AddMember(pChildClone, false);
    }
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

        static_cast<CObjectManager*>(GetObjectManager())->NotifyObjectListeners(child, ON_PREATTACHED);
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

        static_cast<CObjectManager*>(GetObjectManager())->NotifyObjectListeners(child, ON_ATTACHED);
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
            static_cast<CObjectManager*>(GetObjectManager())->NotifyObjectListeners(this, ON_PREDETACHED);
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

            static_cast<CObjectManager*>(GetObjectManager())->NotifyObjectListeners(this, ON_DETACHED);
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
bool CBaseObject::IsParentAttachmentValid() const
{
    return true;
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
void CBaseObject::AddGizmo(CGizmo* gizmo)
{
    GetObjectManager()->GetGizmoManager()->AddGizmo(gizmo);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::RemoveGizmo(CGizmo* gizmo)
{
    GetObjectManager()->GetGizmoManager()->RemoveGizmo(gizmo);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetLookAt(CBaseObject* target)
{
    if (m_lookat == target)
    {
        return;
    }

    StoreUndo("Change LookAt");

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
Ang3 CBaseObject::GetWorldAngles() const
{
    if (m_scale == Vec3(1, 1, 1))
    {
        Quat q = Quat(GetWorldTM());
        Ang3 angles = RAD2DEG(Ang3::GetAnglesXYZ(Matrix33(q)));
        return angles;
    }
    else
    {
        Matrix34 tm = GetWorldTM();
        tm.OrthonormalizeFast();
        Quat q = Quat(tm);
        Ang3 angles = RAD2DEG(Ang3::GetAnglesXYZ(Matrix33(q)));
        return angles;
    }
};

//////////////////////////////////////////////////////////////////////////
void CBaseObject::PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx)
{
    CBaseObject* pFromParent = pFromObject->GetParent();
    if (pFromParent)
    {
        SetFloorNumber(pFromObject->GetFloorNumber());

        CBaseObject* pFromParentInContext = ctx.FindClone(pFromParent);
        if (pFromParentInContext)
        {
            pFromParentInContext->AddMember(this, false);
        }
        else
        {
            pFromParent->AddMember(this, false);
        }
    }
    if (pFromObject->ShouldCloneChildren())
    {
        for (int i = 0; i < pFromObject->GetChildCount(); i++)
        {
            CBaseObject* pChildObject = pFromObject->GetChild(i);
            CBaseObject* pClonedChild = GetObjectManager()->CloneObject(pChildObject);
            ctx.AddClone(pChildObject, pClonedChild);
        }
        for (int i = 0; i < pFromObject->GetChildCount(); i++)
        {
            CBaseObject* pChildObject = pFromObject->GetChild(i);
            CBaseObject* pClonedChild = ctx.FindClone(pChildObject);
            if (pClonedChild)
            {
                pClonedChild->PostClone(pChildObject, ctx);
            }
        }
    }
}

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
void CBaseObject::SetMinSpec(uint32 nSpec, bool bSetChildren)
{
    m_nMinSpec = nSpec;
    UpdateVisibility(!IsHidden());

    // Set min spec for all childs.
    if (bSetChildren)
    {
        for (int i = static_cast<int>(m_childs.size()) - 1; i >= 0; --i)
        {
            m_childs[i]->SetMinSpec(nSpec, true);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::OnPropertyChanged(IVariable*)
{
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::OnMultiSelPropertyChanged(IVariable*)
{
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::OnMenuShowInAssetBrowser()
{
    if (!IsSelected())
    {
        CUndo undo("Select Object");
        GetIEditor()->GetObjectManager()->ClearSelection();
        GetIEditor()->SelectObject(this);
    }

    GetIEditor()->ExecuteCommand("asset_browser.show_viewport_selection");
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::OnContextMenu(QMenu* menu)
{
    if (!menu->isEmpty())
    {
        menu->addSeparator();
    }
    CUsedResources resources;
    GatherUsedResources(resources);

    static_cast<CEditorImpl*>(GetIEditor())->OnObjectContextMenuOpened(menu, this);
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IntersectRayMesh(const Vec3& raySrc, const Vec3& rayDir, SRayHitInfo& outHitInfo) const
{
    const float fRenderMeshTestDistance = 0.2f;
    IRenderNode* pRenderNode = GetEngineNode();
    if (!pRenderNode)
    {
        return false;
    }

    Matrix34 worldTM;
    IStatObj* pStatObj = pRenderNode->GetEntityStatObj(0, 0, &worldTM);
    if (!pStatObj)
    {
        return false;
    }

    // transform decal into object space
    Matrix34 worldTM_Inverted = worldTM.GetInverted();
    Matrix33 worldRot(worldTM_Inverted);
    worldRot.Transpose();
    // put hit direction into the object space
    Vec3 vRayDir = rayDir.GetNormalized() * worldRot;
    // put hit position into the object space
    Vec3 vHitPos = worldTM_Inverted.TransformPoint(raySrc);
    Vec3 vLineP1 = vHitPos - vRayDir * fRenderMeshTestDistance;

    memset(&outHitInfo, 0, sizeof(outHitInfo));
    outHitInfo.inReferencePoint = vHitPos;
    outHitInfo.inRay.origin = vLineP1;
    outHitInfo.inRay.direction = vRayDir;
    outHitInfo.bInFirstHit = false;
    outHitInfo.bUseCache = false;

    return pStatObj->RayIntersection(outHitInfo, nullptr);
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
