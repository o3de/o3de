/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : StaticObject implementation.


#include "EditorDefs.h"

#include "EntityObject.h"

// AzToolsFramework
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>

// Editor
#include "Settings.h"
#include "Viewport.h"
#include "LineGizmo.h"
#include "Include/IObjectManager.h"
#include "Objects/ObjectManager.h"
#include "ViewManager.h"
#include "AnimationContext.h"
#include "HitContext.h"
#include "Objects/SelectionGroup.h"

static constexpr int VIEW_DISTANCE_MULTIPLIER_MAX = 100;

//////////////////////////////////////////////////////////////////////////
//! Undo Entity Link
class CUndoEntityLink
    : public IUndoObject
{
public:
    CUndoEntityLink(CSelectionGroup* pSelection)
    {
        int nObjectSize = pSelection->GetCount();
        m_Links.reserve(nObjectSize);
        for (int i = 0; i < nObjectSize; ++i)
        {
            CBaseObject* pObj = pSelection->GetObject(i);
            if (qobject_cast<CEntityObject*>(pObj))
            {
                SLink link;
                link.entityID = pObj->GetId();
                link.linkXmlNode = XmlHelpers::CreateXmlNode("undo");
                ((CEntityObject*)pObj)->SaveLink(link.linkXmlNode);
                m_Links.push_back(link);
            }
        }
    }

protected:
    void Release() override { delete this; };
    int GetSize() override { return sizeof(*this); }; // Return size of xml state.
    QString GetObjectName() override{ return ""; };

    void Undo([[maybe_unused]] bool bUndo) override
    {
        for (int i = 0, iLinkSize = static_cast<int>(m_Links.size()); i < iLinkSize; ++i)
        {
            SLink& link = m_Links[i];
            CBaseObject* pObj = GetIEditor()->GetObjectManager()->FindObject(link.entityID);
            if (pObj == nullptr)
            {
                continue;
            }
            if (!qobject_cast<CEntityObject*>(pObj))
            {
                continue;
            }
            CEntityObject* pEntity = (CEntityObject*)pObj;
            if (link.linkXmlNode->getChildCount() == 0)
            {
                continue;
            }
            pEntity->LoadLink(link.linkXmlNode->getChild(0));
        }
    }
    void Redo() override{}

private:

    struct SLink
    {
        GUID entityID;
        XmlNodeRef linkXmlNode;
    };

    std::vector<SLink> m_Links;
};

//////////////////////////////////////////////////////////////////////////
//! Undo object for attach/detach changes
class CUndoAttachEntity
    : public IUndoObject
{
public:
    CUndoAttachEntity(CEntityObject* pAttachedObject, bool bAttach)
        : m_attachedEntityGUID(pAttachedObject->GetId())
        , m_attachmentTarget(pAttachedObject->GetAttachTarget())
        , m_attachmentType(pAttachedObject->GetAttachType())
        , m_bAttach(bAttach)
    {}

    void Undo([[maybe_unused]] bool bUndo) override
    {
        if (!m_bAttach)
        {
            SetAttachmentTypeAndTarget();
        }
    }

    void Redo() override
    {
        if (m_bAttach)
        {
            SetAttachmentTypeAndTarget();
        }
    }

private:
    void SetAttachmentTypeAndTarget()
    {
        CObjectManager* pObjectManager = static_cast<CObjectManager*>(GetIEditor()->GetObjectManager());
        CEntityObject* pEntity = static_cast<CEntityObject*>(pObjectManager->FindObject(m_attachedEntityGUID));

        if (pEntity)
        {
            pEntity->SetAttachType(m_attachmentType);
            pEntity->SetAttachTarget(m_attachmentTarget.toUtf8().data());
        }
    }

    int GetSize() override { return sizeof(CUndoAttachEntity); }

    GUID m_attachedEntityGUID;
    CEntityObject::EAttachmentType m_attachmentType;
    QString m_attachmentTarget;
    bool m_bAttach;
};

//////////////////////////////////////////////////////////////////////////
// CBase implementation.
//////////////////////////////////////////////////////////////////////////

namespace
{
    CEntityObject* s_pPropertyPanelEntityObject = nullptr;

    // Prevent OnPropertyChange to be executed when loading many properties at one time.
    static bool s_ignorePropertiesUpdate = false;
};

//////////////////////////////////////////////////////////////////////////
CEntityObject::CEntityObject()
{
    m_bLoadFailed = false;

    m_box.min.Set(0, 0, 0);
    m_box.max.Set(0, 0, 0);

    m_proximityRadius = 0;
    m_innerRadius = 0;
    m_outerRadius = 0;
    m_boxSizeX = 1;
    m_boxSizeY = 1;
    m_boxSizeZ = 1;
    m_bProjectInAllDirs = false;
    m_bProjectorHasTexture = false;

    m_bDisplayBBox = true;
    m_bBBoxSelection = false;
    m_bDisplaySolidBBox = false;
    m_bDisplayAbsoluteRadius = false;
    m_bIconOnTop = false;
    m_bDisplayArrow = false;
    m_entityId = 0;
    m_bVisible = true;
    m_bCalcPhysics = true;
    m_bLight = false;
    m_bAreaLight = false;
    m_fAreaWidth = 1;
    m_fAreaHeight = 1;
    m_fAreaLightSize = 0.05f;
    m_bBoxProjectedCM = false;
    m_fBoxWidth = 1;
    m_fBoxHeight = 1;
    m_fBoxLength = 1;

    m_bEnableReload = true;

    m_lightColor = Vec3(1, 1, 1);

    SetColor(QColor(255, 255, 0));

    // Init Variables.
    mv_castShadow = true;
    mv_castShadowMinSpec = CONFIG_LOW_SPEC;
    mv_outdoor          =   false;
    mv_recvWind = false;
    mv_renderNearest = false;
    mv_noDecals = false;

    mv_createdThroughPool = false;

    mv_obstructionMultiplier = 1.f;
    mv_obstructionMultiplier.SetLimits(0.f, 1.f, 0.01f);

    mv_hiddenInGame = false;
    mv_ratioLOD = 100;
    mv_viewDistanceMultiplier = 1.0f;
    mv_ratioLOD.SetLimits(0, 255);
    mv_viewDistanceMultiplier.SetLimits(0.0f, VIEW_DISTANCE_MULTIPLIER_MAX);

    m_physicsState = nullptr;

    m_attachmentType = eAT_Pivot;

    // cache all the variable callbacks, must match order of enum defined in header
    m_onSetCallbacksCache.emplace_back([this](IVariable* var) { OnAreaHeightChange(var); });
    m_onSetCallbacksCache.emplace_back([this](IVariable* var) { OnAreaLightChange(var); });
    m_onSetCallbacksCache.emplace_back([this](IVariable* var) { OnAreaLightSizeChange(var); });
    m_onSetCallbacksCache.emplace_back([this](IVariable* var) { OnAreaWidthChange(var); });
    m_onSetCallbacksCache.emplace_back([this](IVariable* var) { OnBoxHeightChange(var); });
    m_onSetCallbacksCache.emplace_back([this](IVariable* var) { OnBoxLengthChange(var); });
    m_onSetCallbacksCache.emplace_back([this](IVariable* var) { OnBoxProjectionChange(var); });
    m_onSetCallbacksCache.emplace_back([this](IVariable* var) { OnBoxSizeXChange(var); });
    m_onSetCallbacksCache.emplace_back([this](IVariable* var) { OnBoxSizeYChange(var); });
    m_onSetCallbacksCache.emplace_back([this](IVariable* var) { OnBoxSizeZChange(var); });
    m_onSetCallbacksCache.emplace_back([this](IVariable* var) { OnBoxWidthChange(var); });
    m_onSetCallbacksCache.emplace_back([this](IVariable* var) { OnColorChange(var); });
    m_onSetCallbacksCache.emplace_back([this](IVariable* var) { OnInnerRadiusChange(var); });
    m_onSetCallbacksCache.emplace_back([this](IVariable* var) { OnOuterRadiusChange(var); });
    m_onSetCallbacksCache.emplace_back([this](IVariable* var) { OnProjectInAllDirsChange(var); });
    m_onSetCallbacksCache.emplace_back([this](IVariable* var) { OnProjectorFOVChange(var); });
    m_onSetCallbacksCache.emplace_back([this](IVariable* var) { OnProjectorTextureChange(var); });
    m_onSetCallbacksCache.emplace_back([this](IVariable* var) { OnRadiusChange(var); });
}

CEntityObject::~CEntityObject()
{
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::InitVariables()
{
    mv_castShadowMinSpec.AddEnumItem("Never",          END_CONFIG_SPEC_ENUM);
    mv_castShadowMinSpec.AddEnumItem("Low",                CONFIG_LOW_SPEC);
    mv_castShadowMinSpec.AddEnumItem("Medium",     CONFIG_MEDIUM_SPEC);
    mv_castShadowMinSpec.AddEnumItem("High",               CONFIG_HIGH_SPEC);
    mv_castShadowMinSpec.AddEnumItem("VeryHigh",       CONFIG_VERYHIGH_SPEC);

    mv_castShadow.SetFlags(mv_castShadow.GetFlags() | IVariable::UI_INVISIBLE);
    mv_castShadowMinSpec->SetFlags(mv_castShadowMinSpec->GetFlags() | IVariable::UI_UNSORTED);

    AddVariable(mv_outdoor, "OutdoorOnly", tr("Outdoor Only"));
    AddVariable(mv_castShadow, "CastShadow", tr("Cast Shadow"));
    AddVariable(mv_castShadowMinSpec, "CastShadowMinspec", tr("Cast Shadow MinSpec"));

    AddVariable(mv_ratioLOD, "LodRatio");
    AddVariable(mv_viewDistanceMultiplier, "ViewDistanceMultiplier");
    AddVariable(mv_hiddenInGame, "HiddenInGame");
    AddVariable(mv_recvWind, "RecvWind", tr("Receive Wind"));

    // [artemk]: Add RenderNearest entity param because of animator request.
    // This will cause that slot zero is rendered with ENTITY_SLOT_RENDER_NEAREST flag raised.
    AddVariable(mv_renderNearest, "RenderNearest");
    mv_renderNearest.SetDescription("Used to eliminate z-buffer artifacts when rendering from first person view");
    AddVariable(mv_noDecals, "NoStaticDecals");

    AddVariable(mv_createdThroughPool, "CreatedThroughPool", tr("Created Through Pool"));

    AddVariable(mv_obstructionMultiplier, "ObstructionMultiplier", tr("Obstruction Multiplier"));
}

//////////////////////////////////////////////////////////////////////////&
void CEntityObject::Done()
{
    DeleteEntity();

    ReleaseEventTargets();
    RemoveAllEntityLinks();

    CBaseObject::Done();
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::FreeGameData()
{
    DeleteEntity();
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::Init(IEditor* pEditor, CBaseObject* pPrev, const QString& file)
{
    CBaseObject::Init(pEditor, pPrev, file);

    if (pPrev)
    {
        CEntityObject* pPreviousEntity = ( CEntityObject* )pPrev;

        // Clone Properties.
        if (pPreviousEntity->m_pProperties)
        {
            m_pProperties = CloneProperties(pPreviousEntity->m_pProperties);
        }
        if (pPreviousEntity->m_pProperties2)
        {
            m_pProperties2 = CloneProperties(pPreviousEntity->m_pProperties2);
        }

        mv_createdThroughPool = pPreviousEntity->mv_createdThroughPool;
    }
    else if (!file.isEmpty())
    {
        SetUniqueName(file);
        m_entityClass = file;
    }

    ResetCallbacks();

    return true;
}

//////////////////////////////////////////////////////////////////////////
/*static*/ CEntityObject* CEntityObject::FindFromEntityId(const AZ::EntityId& id)
{
    CEntityObject* retEntity = nullptr;
    AzToolsFramework::ComponentEntityEditorRequestBus::EventResult(retEntity, id, &AzToolsFramework::ComponentEntityEditorRequestBus::Events::GetSandboxObject);
    return retEntity;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetTransformDelegate(ITransformDelegate* pTransformDelegate)
{
    CBaseObject::SetTransformDelegate(pTransformDelegate);

    if (this == s_pPropertyPanelEntityObject)
    {
        return;
    }

    s_ignorePropertiesUpdate = true;
    ForceVariableUpdate();
    s_ignorePropertiesUpdate = false;
    ResetCallbacks();
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::ConvertFromObject(CBaseObject* object)
{
    CBaseObject::ConvertFromObject(object);

    if (qobject_cast<CEntityObject*>(object))
    {
        CEntityObject* pObject = ( CEntityObject* )object;

        mv_outdoor = pObject->mv_outdoor;
        mv_castShadowMinSpec = pObject->mv_castShadowMinSpec;
        mv_ratioLOD = pObject->mv_ratioLOD;
        mv_viewDistanceMultiplier = pObject->mv_viewDistanceMultiplier;
        mv_hiddenInGame = pObject->mv_hiddenInGame;
        mv_recvWind = pObject->mv_recvWind;
        mv_renderNearest = pObject->mv_renderNearest;
        mv_noDecals = pObject->mv_noDecals;

        mv_createdThroughPool = pObject->mv_createdThroughPool;

        mv_obstructionMultiplier = pObject->mv_obstructionMultiplier;
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::GetLocalBounds(AABB& box)
{
    box = m_box;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::HitTest(HitContext& hc)
{
    if (!hc.b2DViewport)
    {
        // Test 3D viewport.
    }

    //////////////////////////////////////////////////////////////////////////
    if ((m_bDisplayBBox && gSettings.viewports.bShowTriggerBounds) || hc.b2DViewport || (m_bDisplayBBox && m_bBBoxSelection))
    {
        float hitEpsilon = hc.view->GetScreenScaleFactor(GetWorldPos()) * 0.01f;
        float hitDist;

        float fScale = GetScale().x;
        AABB boxScaled;
        boxScaled.min = m_box.min * fScale;
        boxScaled.max = m_box.max * fScale;

        Matrix34 invertWTM = GetWorldTM();
        invertWTM.Invert();

        Vec3 xformedRaySrc = invertWTM.TransformPoint(hc.raySrc);
        Vec3 xformedRayDir = invertWTM.TransformVector(hc.rayDir);
        xformedRayDir.Normalize();

        {
            Vec3 intPnt;
            if (m_bBBoxSelection)
            {
                // Check intersection with bbox.
                if (Intersect::Ray_AABB(xformedRaySrc, xformedRayDir, boxScaled, intPnt))
                {
                    hc.dist = xformedRaySrc.GetDistance(intPnt);
                    hc.object = this;
                    return true;
                }
            }
            else
            {
                // Check intersection with bbox edges.
                if (Intersect::Ray_AABBEdge(xformedRaySrc, xformedRayDir, boxScaled, hitEpsilon, hitDist, intPnt))
                {
                    hc.dist = xformedRaySrc.GetDistance(intPnt);
                    hc.object = this;
                    return true;
                }
            }
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::HitTestRect(HitContext& hc)
{
    bool bResult = CBaseObject::HitTestRect(hc);

    if (bResult)
    {
        hc.object = this;
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
IVariable* CEntityObject::FindVariableInSubBlock(CVarBlockPtr& properties, IVariable* pSubBlockVar, const char* pVarName)
{
    IVariable* pVar = pSubBlockVar ? pSubBlockVar->FindVariable(pVarName) : properties->FindVariable(pVarName);
    return pVar;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::AdjustLightProperties(CVarBlockPtr& properties, const char* pSubBlock)
{
    IVariable* pSubBlockVar = pSubBlock ? properties->FindVariable(pSubBlock) : nullptr;

    if (IVariable* pRadius = FindVariableInSubBlock(properties, pSubBlockVar, "Radius"))
    {
        // The value of 0.01 was found through asking Crysis 2 designer
        // team.
        pRadius->SetLimits(0.01f, 100.0f, 0.0f, true, false);
    }

    if (IVariable* pBoxSizeX = FindVariableInSubBlock(properties, pSubBlockVar, "BoxSizeX"))
    {
        pBoxSizeX->SetLimits(0.01f, 100.0f, 0.0f, true, false);
    }

    if (IVariable* pBoxSizeY = FindVariableInSubBlock(properties, pSubBlockVar, "BoxSizeY"))
    {
        pBoxSizeY->SetLimits(0.01f, 100.0f, 0.0f, true, false);
    }

    if (IVariable* pBoxSizeZ = FindVariableInSubBlock(properties, pSubBlockVar, "BoxSizeZ"))
    {
        pBoxSizeZ->SetLimits(0.01f, 100.0f, 0.0f, true, false);
    }

    if (IVariable* pProjectorFov = FindVariableInSubBlock(properties, pSubBlockVar, "fProjectorFov"))
    {
        pProjectorFov->SetLimits(0.01f, 180.0f, 0.0f, true, true);
    }

    if (IVariable* pPlaneWidth = FindVariableInSubBlock(properties, pSubBlockVar, "fPlaneWidth"))
    {
        pPlaneWidth->SetLimits(0.01f, 10.0f, 0.0f, true, false);
        pPlaneWidth->SetHumanName("SourceWidth");
    }

    if (IVariable* pPlaneHeight = FindVariableInSubBlock(properties, pSubBlockVar, "fPlaneHeight"))
    {
        pPlaneHeight->SetLimits(0.01f, 10.0f, 0.0f, true, false);
        pPlaneHeight->SetHumanName("SourceDiameter");
    }

    // For backwards compatibility with old lights (avoids changing settings in Lua which will break loading compatibility).
    // Todo: Change the Lua property names on the next big light refactor.
    if (IVariable* pAreaLight = FindVariableInSubBlock(properties, pSubBlockVar, "bAreaLight"))
    {
        pAreaLight->SetHumanName("PlanarLight");
    }

    bool bCastShadowLegacy = false;  // Backward compatibility for existing shadow casting lights
    if (IVariable* pCastShadowVarLegacy = FindVariableInSubBlock(properties, pSubBlockVar, "bCastShadow"))
    {
        pCastShadowVarLegacy->SetFlags(pCastShadowVarLegacy->GetFlags() | IVariable::UI_INVISIBLE);
        const QString zeroPrefix("0");
        if (!pCastShadowVarLegacy->GetDisplayValue().startsWith(zeroPrefix))
        {
            bCastShadowLegacy = true;
            pCastShadowVarLegacy->SetDisplayValue(zeroPrefix);
        }
    }

    if (IVariable* pCastShadowVar = FindVariableInSubBlock(properties, pSubBlockVar, "nCastShadows"))
    {
        if (bCastShadowLegacy)
        {
            pCastShadowVar->SetDisplayValue("1");
        }
        pCastShadowVar->SetDataType(IVariable::DT_UIENUM);
        pCastShadowVar->SetFlags(pCastShadowVar->GetFlags() | IVariable::UI_UNSORTED);
    }

    if (IVariable* pShadowMinRes = FindVariableInSubBlock(properties, pSubBlockVar, "nShadowMinResPercent"))
    {
        pShadowMinRes->SetDataType(IVariable::DT_UIENUM);
        pShadowMinRes->SetFlags(pShadowMinRes->GetFlags() | IVariable::UI_UNSORTED);
    }

    if (IVariable* pFade = FindVariableInSubBlock(properties, pSubBlockVar, "vFadeDimensionsLeft"))
    {
        pFade->SetFlags(pFade->GetFlags() | IVariable::UI_INVISIBLE);
    }

    if (IVariable* pFade = FindVariableInSubBlock(properties, pSubBlockVar, "vFadeDimensionsRight"))
    {
        pFade->SetFlags(pFade->GetFlags() | IVariable::UI_INVISIBLE);
    }

    if (IVariable* pFade = FindVariableInSubBlock(properties, pSubBlockVar, "vFadeDimensionsNear"))
    {
        pFade->SetFlags(pFade->GetFlags() | IVariable::UI_INVISIBLE);
    }

    if (IVariable* pFade = FindVariableInSubBlock(properties, pSubBlockVar, "vFadeDimensionsFar"))
    {
        pFade->SetFlags(pFade->GetFlags() | IVariable::UI_INVISIBLE);
    }

    if (IVariable* pFade = FindVariableInSubBlock(properties, pSubBlockVar, "vFadeDimensionsTop"))
    {
        pFade->SetFlags(pFade->GetFlags() | IVariable::UI_INVISIBLE);
    }

    if (IVariable* pFade = FindVariableInSubBlock(properties, pSubBlockVar, "vFadeDimensionsBottom"))
    {
        pFade->SetFlags(pFade->GetFlags() | IVariable::UI_INVISIBLE);
    }

    if (IVariable* pSortPriority = FindVariableInSubBlock(properties, pSubBlockVar, "SortPriority"))
    {
        pSortPriority->SetLimits(0, 255, 1, true, true);
    }

    if (IVariable* pAttenFalloffMax = FindVariableInSubBlock(properties, pSubBlockVar, "fAttenuationFalloffMax"))
    {
        pAttenFalloffMax->SetLimits(0.0f, 1.0f, 1.0f / 255.0f, true, true);
    }

    if (IVariable* pVer = FindVariableInSubBlock(properties, pSubBlockVar, "_nVersion"))
    {
        int version = -1;
        pVer->Get(version);
        if (version == -1)
        {
            version++;
            pVer->Set(version);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetName(const QString& name)
{
    if (name == GetName())
    {
        return;
    }

    const QString oldName = GetName();

    CBaseObject::SetName(name);

}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetSelected(bool bSelect)
{
    CBaseObject::SetSelected(bSelect);

    if (bSelect)
    {
        UpdateLightProperty();
    }

}

template <typename T>
struct IVariableType {};
template <>
struct IVariableType<bool>
{
    enum
    {
        value = IVariable::BOOL
    };
};
template <>
struct IVariableType<int>
{
    enum
    {
        value = IVariable::INT
    };
};
template <>
struct IVariableType<float>
{
    enum
    {
        value = IVariable::FLOAT
    };
};
template <>
struct IVariableType<QString>
{
    enum
    {
        value = IVariable::STRING
    };
};
template <>
struct IVariableType<Vec3>
{
    enum
    {
        value = IVariable::VECTOR
    };
};

void CEntityObject::DrawExtraLightInfo(DisplayContext& dc)
{
    IObjectManager* objMan = GetIEditor()->GetObjectManager();

    if (objMan)
    {
        if (objMan->IsLightClass(this) && GetProperties())
        {
            QString csText("");

            if (GetEntityPropertyBool("bAmbient"))
            {
                csText += "A";
            }

            if (!GetEntityPropertyString("texture_Texture").isEmpty())
            {
                csText += "P";
            }

            int nLightType = GetEntityPropertyInteger("nCastShadows");
            if (nLightType > 0)
            {
                csText += "S";
            }

            float fScale = GetIEditor()->GetViewManager()->GetView(ET_ViewportUnknown)->GetScreenScaleFactor(GetWorldPos());
            Vec3 vDrawPos(GetWorldPos());
            vDrawPos.z += fScale / 25;

            ColorB col(255, 255, 255);
            dc.SetColor(col);
            dc.DrawTextLabel(vDrawPos, 1.3f, csText.toUtf8().data());
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CEntityObject::DrawProjectorPyramid(DisplayContext& dc, float dist)
{
    const int numPoints = 16; // per one arc
    const int numArcs = 6;

    if (m_projectorFOV < FLT_EPSILON)
    {
        return;
    }

    Vec3 points[numPoints * numArcs];
    {
        // generate 4 arcs on intersection of sphere with pyramid
        const float fov = DEG2RAD(m_projectorFOV);

        const Vec3 lightAxis(dist, 0.0f, 0.0f);
        const float tanA = tan(fov * 0.5f);
        const float fovProj = asinf(1.0f / sqrtf(2.0f + 1.0f / (tanA * tanA))) * 2.0f;

        const float halfFov = 0.5f * fov;
        const float halfFovProj = fovProj * 0.5f;
        const float anglePerSegmentOfFovProj = 1.0f / (numPoints - 1) * fovProj;

        const Quat yRot = Quat::CreateRotationY(halfFov);
        Vec3* arcPoints = points;
        for (int i = 0; i < numPoints; ++i)
        {
            float angle = i * anglePerSegmentOfFovProj - halfFovProj;
            arcPoints[i] = lightAxis * Quat::CreateRotationZ(angle) * yRot;
        }

        const Quat zRot = Quat::CreateRotationZ(halfFov);
        arcPoints += numPoints;
        for (int i = 0; i < numPoints; ++i)
        {
            float angle = (numPoints - i - 1) * anglePerSegmentOfFovProj - halfFovProj;
            arcPoints[i] = lightAxis * Quat::CreateRotationY(angle) * zRot;
        }

        const Quat nyRot = Quat::CreateRotationY(-halfFov);
        arcPoints += numPoints;
        for (int i = 0; i < numPoints; ++i)
        {
            float angle = (numPoints - i - 1) * anglePerSegmentOfFovProj - halfFovProj;
            arcPoints[i] = lightAxis * Quat::CreateRotationZ(angle) * nyRot;
        }

        const Quat nzRot = Quat::CreateRotationZ(-halfFov);
        arcPoints += numPoints;
        for (int i = 0; i < numPoints; ++i)
        {
            float angle = i * anglePerSegmentOfFovProj - halfFovProj;
            arcPoints[i] = lightAxis * Quat::CreateRotationY(angle) * nzRot;
        }

        // generate cross
        arcPoints += numPoints;
        const float anglePerSegmentOfFov = 1.0f / (numPoints - 1) * fov;
        for (int i = 0; i < numPoints; ++i)
        {
            float angle = i * anglePerSegmentOfFov - halfFov;
            arcPoints[i] = lightAxis * Quat::CreateRotationY(angle);
        }

        arcPoints += numPoints;
        for (int i = 0; i < numPoints; ++i)
        {
            float angle = i * anglePerSegmentOfFov - halfFov;
            arcPoints[i] = lightAxis * Quat::CreateRotationZ(angle);
        }
    }
    // draw pyramid and sphere intersection
    dc.DrawPolyLine(points, numPoints * 4, false);

    // draw cross
    dc.DrawPolyLine(points + numPoints * 4, numPoints, false);
    dc.DrawPolyLine(points + numPoints * 5, numPoints, false);

    Vec3 org(0.0f, 0.0f, 0.0f);
    dc.DrawLine(org, points[numPoints * 0]);
    dc.DrawLine(org, points[numPoints * 1]);
    dc.DrawLine(org, points[numPoints * 2]);
    dc.DrawLine(org, points[numPoints * 3]);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::DrawProjectorFrustum(DisplayContext& dc, Vec2 size, float dist)
{
    static const Vec3 org(0.0f, 0.0f, 0.0f);
    const Vec3 corners[4] =
    {
        Vec3(dist, -size.x, -size.y),
        Vec3(dist, size.x, -size.y),
        Vec3(dist, -size.x, size.y),
        Vec3(dist, size.x, size.y)
    };

    dc.DrawLine(org, corners[0]);
    dc.DrawLine(org, corners[1]);
    dc.DrawLine(org, corners[2]);
    dc.DrawLine(org, corners[3]);

    dc.DrawWireBox(Vec3(dist, -size.x, -size.y), Vec3(dist, size.x, size.y));
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::Serialize(CObjectArchive& ar)
{
    CBaseObject::Serialize(ar);
    XmlNodeRef xmlNode = ar.node;
    if (ar.bLoading)
    {
        // Load
        QString entityClass = m_entityClass;
        m_bLoadFailed = false;

        xmlNode->getAttr("EntityClass", entityClass);
        m_physicsState = xmlNode->findChild("PhysicsState");

        Vec3 angles;
        // Backward compatibility, with FarCry levels.
        if (xmlNode->getAttr("Angles", angles))
        {
            angles = DEG2RAD(angles);
            angles.z += gf_PI / 2;
            Quat quat;
            quat.SetRotationXYZ(Ang3(angles));
            SetRotation(quat);
        }

        // Load Event Targets.
        ReleaseEventTargets();
        XmlNodeRef eventTargets = xmlNode->findChild("EventTargets");
        if (eventTargets)
        {
            for (int i = 0; i < eventTargets->getChildCount(); i++)
            {
                XmlNodeRef eventTarget = eventTargets->getChild(i);
                CEntityEventTarget et;
                et.target = nullptr;
                GUID targetId = GUID_NULL;
                eventTarget->getAttr("TargetId", targetId);
                eventTarget->getAttr("Event", et.event);
                eventTarget->getAttr("SourceEvent", et.sourceEvent);
                m_eventTargets.emplace_back(AZStd::move(et));
                if (targetId != GUID_NULL)
                {
                    ar.SetResolveCallback(
                        this, targetId,
                        [this,i](CBaseObject* object) { ResolveEventTarget(object, i); });
                }
            }
        }

        XmlNodeRef propsNode = xmlNode->findChild("Properties");
        XmlNodeRef props2Node = xmlNode->findChild("Properties2");

        QString attachmentType;
        xmlNode->getAttr("AttachmentType", attachmentType);

        if (attachmentType == "CharacterBone")
        {
            m_attachmentType = eAT_CharacterBone;
        }
        else
        {
            m_attachmentType = eAT_Pivot;
        }

        xmlNode->getAttr("AttachmentTarget", m_attachmentTarget);

        if (ar.bUndo)
        {
            RemoveAllEntityLinks();
            PostLoad(ar);
        }

        if ((mv_castShadowMinSpec == CONFIG_LOW_SPEC) && !mv_castShadow) // backwards compatibility check
        {
            mv_castShadowMinSpec = END_CONFIG_SPEC_ENUM;
            mv_castShadow = true;
        }
    }
    else
    {
        if (m_attachmentType != eAT_Pivot)
        {
            if (m_attachmentType == eAT_CharacterBone)
            {
                xmlNode->setAttr("AttachmentType", "CharacterBone");
            }

            xmlNode->setAttr("AttachmentTarget", m_attachmentTarget.toUtf8().data());
        }

        // Saving.
        if (!m_entityClass.isEmpty())
        {
            xmlNode->setAttr("EntityClass", m_entityClass.toUtf8().data());
        }

        if (m_physicsState)
        {
            xmlNode->addChild(m_physicsState);
        }

        //! Save properties.
        if (m_pProperties)
        {
            XmlNodeRef propsNode = xmlNode->newChild("Properties");
            m_pProperties->Serialize(propsNode, ar.bLoading);
        }

        //! Save properties.
        if (m_pProperties2)
        {
            XmlNodeRef propsNode = xmlNode->newChild("Properties2");
            m_pProperties2->Serialize(propsNode, ar.bLoading);
        }

        // Save Event Targets.
        if (!m_eventTargets.empty())
        {
            XmlNodeRef eventTargets = xmlNode->newChild("EventTargets");
            for (int i = 0; i < m_eventTargets.size(); i++)
            {
                CEntityEventTarget& et = m_eventTargets[i];
                GUID targetId = GUID_NULL;
                if (et.target != nullptr)
                {
                    targetId = et.target->GetId();
                }

                XmlNodeRef eventTarget = eventTargets->newChild("EventTarget");
                eventTarget->setAttr("TargetId", targetId);
                eventTarget->setAttr("Event", et.event.toUtf8().data());
                eventTarget->setAttr("SourceEvent", et.sourceEvent.toUtf8().data());
            }
        }

        // Save Entity Links.
        SaveLink(xmlNode);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::PostLoad(CObjectArchive& ar)
{
    //////////////////////////////////////////////////////////////////////////
    // Load Links.
    XmlNodeRef linksNode = ar.node->findChild("EntityLinks");
    LoadLink(linksNode, &ar);
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CEntityObject::Export([[maybe_unused]] const QString& levelPath, XmlNodeRef& xmlExportNode)
{
    if (m_bLoadFailed)
    {
        return nullptr;
    }

    // Do not export entity with bad id.
    if (!m_entityId)
    {
        return XmlHelpers::CreateXmlNode("Temp");
    }

    // Export entities to entities.ini
    XmlNodeRef objNode = xmlExportNode->newChild("Entity");

    objNode->setAttr("Name", GetName().toUtf8().data());

    Vec3 pos = GetPos(), scale = GetScale();
    Quat rotate = GetRotation();

    if (GetParent())
    {
        if (qobject_cast<CEntityObject*>(GetParent()))
        {
            // Store parent entity id.
            CEntityObject* parentEntity = ( CEntityObject* )GetParent();
            if (parentEntity)
            {
                objNode->setAttr("ParentId", parentEntity->GetEntityId());
                if (m_attachmentType != eAT_Pivot)
                {
                    if (m_attachmentType == eAT_CharacterBone)
                    {
                        objNode->setAttr("AttachmentType", "CharacterBone");
                    }

                    objNode->setAttr("AttachmentTarget", m_attachmentTarget.toUtf8().data());
                }
            }
        }
        else
        {
            // Export world coordinates.
            AffineParts ap;
            ap.SpectralDecompose(GetWorldTM());
            pos = ap.pos;
            rotate = ap.rot;
            scale = ap.scale;
        }
    }

    if (!IsEquivalent(pos, Vec3(0, 0, 0), 0))
    {
        objNode->setAttr("Pos", pos);
    }

    if (!rotate.IsIdentity())
    {
        objNode->setAttr("Rotate", rotate);
    }

    if (!IsEquivalent(scale, Vec3(1, 1, 1), 0))
    {
        objNode->setAttr("Scale", scale);
    }

    objNode->setTag("Entity");
    objNode->setAttr("EntityClass", m_entityClass.toUtf8().data());
    objNode->setAttr("EntityId", m_entityId);

    if (mv_ratioLOD != 100)
    {
        objNode->setAttr("LodRatio", ( int )mv_ratioLOD);
    }

    if (fabs(mv_viewDistanceMultiplier - 1.f) > FLT_EPSILON)
    {
        objNode->setAttr("ViewDistanceMultiplier", mv_viewDistanceMultiplier);
    }

    objNode->setAttr("CastShadowMinSpec", mv_castShadowMinSpec);

    if (mv_recvWind)
    {
        objNode->setAttr("RecvWind", true);
    }

    if (mv_noDecals)
    {
        objNode->setAttr("NoDecals", true);
    }

    if (mv_outdoor)
    {
        objNode->setAttr("OutdoorOnly", true);
    }

    if (GetMinSpec() != 0)
    {
        objNode->setAttr("MinSpec", ( uint32 )GetMinSpec());
    }

    if (mv_hiddenInGame)
    {
        objNode->setAttr("HiddenInGame", true);
    }

    if (mv_createdThroughPool)
    {
        objNode->setAttr("CreatedThroughPool", true);
    }

    if (mv_obstructionMultiplier != 1.f)
    {
        objNode->setAttr("ObstructionMultiplier", (float)mv_obstructionMultiplier);
    }

    if (m_physicsState)
    {
        objNode->addChild(m_physicsState);
    }

    // Export Event Targets.
    if (!m_eventTargets.empty())
    {
        XmlNodeRef eventTargets = objNode->newChild("EventTargets");
        for (int i = 0; i < m_eventTargets.size(); i++)
        {
            CEntityEventTarget& et = m_eventTargets[i];

            int entityId = 0;
            if (et.target)
            {
                if (qobject_cast<CEntityObject*>(et.target))
                {
                    entityId = (( CEntityObject* )et.target)->GetEntityId();
                }
            }

            XmlNodeRef eventTarget = eventTargets->newChild("EventTarget");
            eventTarget->setAttr("Target", entityId);
            eventTarget->setAttr("Event", et.event.toUtf8().data());
            eventTarget->setAttr("SourceEvent", et.sourceEvent.toUtf8().data());
        }
    }

    // Save Entity Links.
    if (!m_links.empty())
    {
        XmlNodeRef linksNode = objNode->newChild("EntityLinks");
        for (size_t i = 0, num = m_links.size(); i < num; i++)
        {
            if (m_links[i].target)
            {
                XmlNodeRef linkNode = linksNode->newChild("Link");
                linkNode->setAttr("TargetId", m_links[i].target->GetEntityId());
                linkNode->setAttr("Name", m_links[i].name.toUtf8().data());
            }
        }
    }

    //! Export properties.
    if (m_pProperties)
    {
        XmlNodeRef propsNode = objNode->newChild("Properties");
        m_pProperties->Serialize(propsNode, false);
    }
    //! Export properties.
    if (m_pProperties2)
    {
        XmlNodeRef propsNode = objNode->newChild("Properties2");
        m_pProperties2->Serialize(propsNode, false);
    }

    return objNode;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnEvent(ObjectEvent event)
{
    CBaseObject::OnEvent(event);

    switch (event)
    {
    case EVENT_RELOAD_ENTITY:
        GetIEditor()->GetErrorReport()->SetCurrentValidatorObject(this);
        break;

    case EVENT_RELOAD_GEOM:
        GetIEditor()->GetErrorReport()->SetCurrentValidatorObject(this);
        break;

    case EVENT_FREE_GAME_DATA:
        FreeGameData();
        break;

    case EVENT_CONFIG_SPEC_CHANGE:
    {
        break;
    }
    default:
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::UpdateVisibility(bool bVisible)
{
    CBaseObject::UpdateVisibility(bVisible);

    bool bVisibleWithSpec = bVisible && !IsHiddenBySpec();
    if (bVisibleWithSpec != static_cast<bool>(m_bVisible))
    {
        m_bVisible = bVisibleWithSpec;
    }

    size_t const numChildren = GetChildCount();
    for (size_t i = 0; i < numChildren; ++i)
    {
        CBaseObject* const pChildObject = GetChild(i);
        pChildObject->SetHidden(!m_bVisible);

        if (qobject_cast<CEntityObject*>(pChildObject))
        {
            pChildObject->UpdateVisibility(m_bVisible);
        }
    }
};

//////////////////////////////////////////////////////////////////////////
IVariable* CEntityObject::GetLightVariable(const char* name0) const
{
    if (m_pProperties2)
    {
        IVariable* pLightProperties = m_pProperties2->FindVariable("LightProperties_Base");

        if (pLightProperties)
        {
            for (int i = 0; i < pLightProperties->GetNumVariables(); ++i)
            {
                IVariable* pChild = pLightProperties->GetVariable(i);

                if (pChild == nullptr)
                {
                    continue;
                }

                QString name(pChild->GetName());
                if (name == name0)
                {
                    return pChild;
                }
            }
        }
    }

    return m_pProperties ? m_pProperties->FindVariable(name0) : nullptr;
}

//////////////////////////////////////////////////////////////////////////
QString CEntityObject::GetLightAnimation() const
{
    IVariable* pStyleGroup = GetLightVariable("Style");
    if (pStyleGroup)
    {
        for (int i = 0; i < pStyleGroup->GetNumVariables(); ++i)
        {
            IVariable* pChild = pStyleGroup->GetVariable(i);

            if (pChild == nullptr)
            {
                continue;
            }

            QString name(pChild->GetName());
            if (name == "lightanimation_LightAnimation")
            {
                QString lightAnimationName;
                pChild->Get(lightAnimationName);
                return lightAnimationName;
            }
        }
    }

    return "";
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::ResolveEventTarget(CBaseObject* object, unsigned int index)
{
    // Find target id.
    assert(index < m_eventTargets.size());
    if (object)
    {
        object->AddEventListener(this);
    }
    m_eventTargets[index].target = object;

    // Make line gizmo.
    if (!m_eventTargets[index].pLineGizmo && object)
    {
        CLineGizmo* pLineGizmo = new CLineGizmo;
        pLineGizmo->SetObjects(this, object);
        pLineGizmo->SetColor(Vec3(0.8f, 0.4f, 0.4f), Vec3(0.8f, 0.4f, 0.4f));
        pLineGizmo->SetName(m_eventTargets[index].event.toUtf8().data());
        AddGizmo(pLineGizmo);
        m_eventTargets[index].pLineGizmo = pLineGizmo;
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::RemoveAllEntityLinks()
{
    while (!m_links.empty())
    {
        RemoveEntityLink(static_cast<int>(m_links.size() - 1));
    }
    m_links.clear();
    SetModified(false);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::ReleaseEventTargets()
{
    while (!m_eventTargets.empty())
    {
        RemoveEventTarget(static_cast<int>(m_eventTargets.size() - 1), false);
    }
    m_eventTargets.clear();
    SetModified(false);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::LoadLink(XmlNodeRef xmlNode, CObjectArchive* pArchive)
{
    RemoveAllEntityLinks();

    if (!xmlNode)
    {
        return;
    }

    QString name;
    GUID targetId;

    for (int i = 0; i < xmlNode->getChildCount(); i++)
    {
        XmlNodeRef linkNode = xmlNode->getChild(i);
        linkNode->getAttr("Name", name);

        if (linkNode->getAttr("TargetId", targetId))
        {
            int version = 0;
            linkNode->getAttr("Version", version);

            GUID newTargetId = pArchive ? pArchive->ResolveID(targetId) : targetId;

            // Backwards compatibility with old bone attachment system
            const char kOldBoneLinkPrefix = '@';
            if (version == 0 && !name.isEmpty() && name[0] == kOldBoneLinkPrefix)
            {
                CBaseObject* pObject = FindObject(newTargetId);
                if (qobject_cast<CEntityObject*>(pObject))
                {
                    CEntityObject* pTargetEntity = static_cast<CEntityObject*>(pObject);

                    Quat relRot(IDENTITY);
                    linkNode->getAttr("RelRot", relRot);
                    Vec3 relPos(IDENTITY);
                    linkNode->getAttr("RelPos", relPos);

                    SetAttachType(eAT_CharacterBone);
                    SetAttachTarget(name.mid(1).toUtf8().data());
                    pTargetEntity->AttachChild(this);

                    SetPos(relPos);
                    SetRotation(relRot);
                }
            }
            else
            {
                AddEntityLink(name, newTargetId);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SaveLink(XmlNodeRef xmlNode)
{
    if (m_links.empty())
    {
        return;
    }

    XmlNodeRef linksNode = xmlNode->newChild("EntityLinks");
    for (size_t i = 0, num = m_links.size(); i < num; i++)
    {
        XmlNodeRef linkNode = linksNode->newChild("Link");
        linkNode->setAttr("TargetId", m_links[i].targetId);
        linkNode->setAttr("Name", m_links[i].name.toUtf8().data());
        linkNode->setAttr("Version", 1);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnObjectEvent(CBaseObject* target, int event)
{
    // When event target is deleted.
    if (event == CBaseObject::ON_DELETE)
    {
        // Find this target in events list and remove.
        int numTargets = static_cast<int>(m_eventTargets.size());
        for (int i = 0; i < numTargets; i++)
        {
            if (m_eventTargets[i].target == target)
            {
                RemoveEventTarget(i);
                numTargets = static_cast<int>(m_eventTargets.size());
                i--;
            }
        }
    }
    else if (event == CBaseObject::ON_PREDELETE)
    {
        int numTargets = static_cast<int>(m_links.size());
        for (int i = 0; i < numTargets; i++)
        {
            if (m_links[i].target == target)
            {
                RemoveEntityLink(i);
                numTargets = static_cast<int>(m_eventTargets.size());
                i--;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
int CEntityObject::AddEventTarget(CBaseObject* target, const QString& event, const QString& sourceEvent, [[maybe_unused]] bool bUpdateScript)
{
    StoreUndo();
    CEntityEventTarget et;
    et.target = target;
    et.event = event;
    et.sourceEvent = sourceEvent;

    // Assign event target.
    if (et.target)
    {
        et.target->AddEventListener(this);
    }

    if (target)
    {
        // Make line gizmo.
        CLineGizmo* pLineGizmo = new CLineGizmo;
        pLineGizmo->SetObjects(this, target);
        pLineGizmo->SetColor(Vec3(0.8f, 0.4f, 0.4f), Vec3(0.8f, 0.4f, 0.4f));
        pLineGizmo->SetName(event.toUtf8().data());
        AddGizmo(pLineGizmo);
        et.pLineGizmo = pLineGizmo;
    }

    m_eventTargets.push_back(et);

    SetModified(false);
    return static_cast<int>(m_eventTargets.size() - 1);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::RemoveEventTarget(int index, [[maybe_unused]] bool bUpdateScript)
{
    if (index >= 0 && index < m_eventTargets.size())
    {
        StoreUndo();

        if (m_eventTargets[index].pLineGizmo)
        {
            RemoveGizmo(m_eventTargets[index].pLineGizmo);
        }

        if (m_eventTargets[index].target)
        {
            m_eventTargets[index].target->RemoveEventListener(this);
        }
        m_eventTargets.erase(m_eventTargets.begin() + index);

        SetModified(false);
    }
}

//////////////////////////////////////////////////////////////////////////
int CEntityObject::AddEntityLink(const QString& name, GUID targetEntityId)
{
    CEntityObject* target = nullptr;
    if (targetEntityId != GUID_NULL)
    {
        CBaseObject* pObject = FindObject(targetEntityId);
        if (qobject_cast<CEntityObject*>(pObject))
        {
            target = ( CEntityObject* )pObject;

            // Legacy entities and AZ entities shouldn't be linked.
            if (target->GetType() == OBJTYPE_AZENTITY || GetType() == OBJTYPE_AZENTITY)
            {
                return -1;
            }
        }
    }

    StoreUndo();

    CLineGizmo* pLineGizmo = nullptr;

    // Assign event target.
    if (target)
    {
        target->AddEventListener(this);

        // Make line gizmo.
        pLineGizmo = new CLineGizmo;
        pLineGizmo->SetObjects(this, target);
        pLineGizmo->SetColor(Vec3(0.4f, 1.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
        pLineGizmo->SetName(name.toUtf8().data());
        AddGizmo(pLineGizmo);
    }

    CEntityLink lnk;
    lnk.targetId = targetEntityId;
    lnk.target = target;
    lnk.name = name;
    lnk.pLineGizmo = pLineGizmo;
    m_links.push_back(lnk);

    SetModified(false);

    return static_cast<int>(m_links.size() - 1);
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::EntityLinkExists(const QString& name, GUID targetEntityId)
{
    for (size_t i = 0, num = m_links.size(); i < num; ++i)
    {
        if (m_links[i].targetId == targetEntityId && name.compare(m_links[i].name, Qt::CaseInsensitive) == 0)
        {
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::RemoveEntityLink(int index)
{
    if (index >= 0 && index < m_links.size())
    {
        CEntityLink& link = m_links[index];
        StoreUndo();

        if (link.pLineGizmo)
        {
            RemoveGizmo(link.pLineGizmo);
        }

        if (link.target)
        {
            link.target->RemoveEventListener(this);
            link.target->EntityUnlinked(link.name, GetId());
        }
        m_links.erase(m_links.begin() + index);

        SetModified(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::RenameEntityLink(int index, const QString& newName)
{
    if (index >= 0 && index < m_links.size())
    {
        StoreUndo();

        if (m_links[index].pLineGizmo)
        {
            m_links[index].pLineGizmo->SetName(newName.toUtf8().data());
        }

        m_links[index].name = newName;

        SetModified(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnRadiusChange(IVariable* var)
{
    var->Get(m_proximityRadius);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnInnerRadiusChange(IVariable* var)
{
    var->Get(m_innerRadius);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnOuterRadiusChange(IVariable* var)
{
    var->Get(m_outerRadius);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnBoxSizeXChange(IVariable* var)
{
    var->Get(m_boxSizeX);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnBoxSizeYChange(IVariable* var)
{
    var->Get(m_boxSizeY);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnBoxSizeZChange(IVariable* var)
{
    var->Get(m_boxSizeZ);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnProjectorFOVChange(IVariable* var)
{
    var->Get(m_projectorFOV);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnProjectInAllDirsChange(IVariable* var)
{
    int value;
    var->Get(value);
    m_bProjectInAllDirs = value;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnProjectorTextureChange(IVariable* var)
{
    QString texture;
    var->Get(texture);
    m_bProjectorHasTexture = !texture.isEmpty();
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnAreaLightChange(IVariable* var)
{
    int value;
    var->Get(value);
    m_bAreaLight = value;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnAreaWidthChange(IVariable* var)
{
    var->Get(m_fAreaWidth);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnAreaHeightChange(IVariable* var)
{
    var->Get(m_fAreaHeight);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnAreaLightSizeChange(IVariable* var)
{
    var->Get(m_fAreaLightSize);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnColorChange(IVariable* var)
{
    var->Get(m_lightColor);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnBoxProjectionChange(IVariable* var)
{
    int value;
    var->Get(value);
    m_bBoxProjectedCM = value;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnBoxWidthChange(IVariable* var)
{
    var->Get(m_fBoxWidth);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnBoxHeightChange(IVariable* var)
{
    var->Get(m_fBoxHeight);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnBoxLengthChange(IVariable* var)
{
    var->Get(m_fBoxLength);
}

//////////////////////////////////////////////////////////////////////////
CVarBlock* CEntityObject::CloneProperties(CVarBlock* srcProperties)
{
    assert(srcProperties);
    return srcProperties->Clone(true);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnLoadFailed()
{
    m_bLoadFailed = true;

    CErrorRecord err;
    err.error = tr("Entity %1 Failed to Spawn (Script: %2)").arg(GetName(), m_entityClass);
    err.pObject = this;
    GetIEditor()->GetErrorReport()->ReportError(err);
}

//////////////////////////////////////////////////////////////////////////
//! Analyze errors for this object.
void CEntityObject::Validate(IErrorReport* report)
{
    CBaseObject::Validate(report);

    if (!m_entityClass.isEmpty())
    {
        CErrorRecord err;
        err.error = tr("Entity %1 Failed to Spawn (Script: %2)").arg(GetName(), m_entityClass);
        err.pObject = this;
        report->ReportError(err);
        return;
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::GatherUsedResources(CUsedResources& resources)
{
    CBaseObject::GatherUsedResources(resources);
    if (m_pProperties)
    {
        m_pProperties->GatherUsedResources(resources);
    }
    if (m_pProperties2)
    {
        m_pProperties2->GatherUsedResources(resources);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CEntityObject::IsSimilarObject(CBaseObject* pObject)
{
    if (pObject->GetClassDesc() == GetClassDesc() && pObject->metaObject() == metaObject())
    {
        CEntityObject* pEntity = ( CEntityObject* )pObject;
        if (m_entityClass == pEntity->m_entityClass &&
            m_proximityRadius == pEntity->m_proximityRadius &&
            m_innerRadius == pEntity->m_innerRadius &&
            m_outerRadius == pEntity->m_outerRadius)
        {
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::PreInitLightProperty()
{
    if (!IsLight() || !m_pProperties)
    {
        return;
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::UpdateLightProperty()
{
    if (!IsLight() || !m_pProperties)
    {
        return;
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::ForceVariableUpdate()
{
    if (m_pProperties)
    {
        m_pProperties->OnSetValues();
    }
    if (m_pProperties2)
    {
        m_pProperties2->OnSetValues();
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::ResetCallbacks()
{
    ClearCallbacks();

    CVarBlock* pProperties = m_pProperties;
    CVarBlock* pProperties2 = m_pProperties2;

    if (pProperties)
    {
        m_callbacks.reserve(6);

        //@FIXME Hack to display radii of properties.
        // wires properties from param block, to this entity internal variables.
        IVariable* var = nullptr;
        var = pProperties->FindVariable("Radius", false);
        if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
        {
            var->Get(m_proximityRadius);
            SetVariableCallback(var, &m_onSetCallbacksCache[VariableCallbackIndex::OnRadiusChange]);
        }
        else
        {
            var = pProperties->FindVariable("radius", false);
            if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
            {
                var->Get(m_proximityRadius);
                SetVariableCallback(var, &m_onSetCallbacksCache[VariableCallbackIndex::OnRadiusChange]);
            }
        }

        var = pProperties->FindVariable("InnerRadius", false);
        if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
        {
            var->Get(m_innerRadius);
            SetVariableCallback(var, &m_onSetCallbacksCache[VariableCallbackIndex::OnInnerRadiusChange]);
        }
        var = pProperties->FindVariable("OuterRadius", false);
        if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
        {
            var->Get(m_outerRadius);
            SetVariableCallback(var, &m_onSetCallbacksCache[VariableCallbackIndex::OnOuterRadiusChange]);
        }

        var = pProperties->FindVariable("BoxSizeX", false);
        if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
        {
            var->Get(m_boxSizeX);
            SetVariableCallback(var, &m_onSetCallbacksCache[VariableCallbackIndex::OnBoxSizeXChange]);
        }
        var = pProperties->FindVariable("BoxSizeY", false);
        if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
        {
            var->Get(m_boxSizeY);
            SetVariableCallback(var, &m_onSetCallbacksCache[VariableCallbackIndex::OnBoxSizeYChange]);
        }
        var = pProperties->FindVariable("BoxSizeZ", false);
        if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
        {
            var->Get(m_boxSizeZ);
            SetVariableCallback(var, &m_onSetCallbacksCache[VariableCallbackIndex::OnBoxSizeZChange]);
        }

        var = pProperties->FindVariable("fAttenuationBulbSize");
        if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
        {
            var->Get(m_fAreaLightSize);
            SetVariableCallback(var, &m_onSetCallbacksCache[VariableCallbackIndex::OnAreaLightSizeChange]);
        }

        IVariable* pProjector = pProperties->FindVariable("Projector");
        if (pProjector)
        {
            var = pProjector->FindVariable("fProjectorFov");
            if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
            {
                var->Get(m_projectorFOV);
                SetVariableCallback(var, &m_onSetCallbacksCache[VariableCallbackIndex::OnProjectorFOVChange]);
            }
            var = pProjector->FindVariable("bProjectInAllDirs");
            if (var && var->GetType() == IVariable::BOOL)
            {
                int value;
                var->Get(value);
                m_bProjectInAllDirs = value;
                SetVariableCallback(var, &m_onSetCallbacksCache[VariableCallbackIndex::OnProjectInAllDirsChange]);
            }
            var = pProjector->FindVariable("texture_Texture");
            if (var && var->GetType() == IVariable::STRING)
            {
                QString projectorTexture;
                var->Get(projectorTexture);
                m_bProjectorHasTexture = !projectorTexture.isEmpty();
                SetVariableCallback(var, &m_onSetCallbacksCache[VariableCallbackIndex::OnProjectorTextureChange]);
            }
        }

        IVariable* pColorGroup = pProperties->FindVariable("Color", false);
        if (pColorGroup)
        {
            const int nChildCount = pColorGroup->GetNumVariables();
            for (int i = 0; i < nChildCount; ++i)
            {
                IVariable* pChild = pColorGroup->GetVariable(i);
                if (!pChild)
                {
                    continue;
                }

                QString name(pChild->GetName());
                if (name == "clrDiffuse")
                {
                    pChild->Get(m_lightColor);
                    SetVariableCallback(pChild, &m_onSetCallbacksCache[VariableCallbackIndex::OnColorChange]);
                    break;
                }
            }
        }

        IVariable* pType = pProperties->FindVariable("Shape");
        if (pType)
        {
            var = pType->FindVariable("bAreaLight");
            if (var && var->GetType() == IVariable::BOOL)
            {
                int value;
                var->Get(value);
                m_bAreaLight = value;
                SetVariableCallback(var, &m_onSetCallbacksCache[VariableCallbackIndex::OnAreaLightChange]);
            }
            var = pType->FindVariable("fPlaneWidth");
            if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
            {
                var->Get(m_fAreaWidth);
                SetVariableCallback(var, &m_onSetCallbacksCache[VariableCallbackIndex::OnAreaWidthChange]);
            }
            var = pType->FindVariable("fPlaneHeight");
            if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
            {
                var->Get(m_fAreaHeight);
                SetVariableCallback(var, &m_onSetCallbacksCache[VariableCallbackIndex::OnAreaHeightChange]);
            }
        }

        IVariable* pProjection = pProperties->FindVariable("Projection");
        if (pProjection)
        {
            var = pProjection->FindVariable("bBoxProject");
            if (var && var->GetType() == IVariable::BOOL)
            {
                int value;
                var->Get(value);
                m_bBoxProjectedCM = value;
                SetVariableCallback(var, &m_onSetCallbacksCache[VariableCallbackIndex::OnBoxProjectionChange]);
            }
            var = pProjection->FindVariable("fBoxWidth");
            if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
            {
                var->Get(m_fBoxWidth);
                SetVariableCallback(var, &m_onSetCallbacksCache[VariableCallbackIndex::OnBoxWidthChange]);
            }
            var = pProjection->FindVariable("fBoxHeight");
            if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
            {
                var->Get(m_fBoxHeight);
                SetVariableCallback(var, &m_onSetCallbacksCache[VariableCallbackIndex::OnBoxHeightChange]);
            }
            var = pProjection->FindVariable("fBoxLength");
            if (var && (var->GetType() == IVariable::FLOAT || var->GetType() == IVariable::INT))
            {
                var->Get(m_fBoxLength);
                SetVariableCallback(var, &m_onSetCallbacksCache[VariableCallbackIndex::OnBoxLengthChange]);
            }
        }

        // Each property must have callback to our OnPropertyChange.
        pProperties->AddOnSetCallback(&m_onSetCallbacksCache[VariableCallbackIndex::OnPropertyChange]);
    }

    if (pProperties2)
    {
        pProperties2->AddOnSetCallback(&m_onSetCallbacksCache[VariableCallbackIndex::OnPropertyChange]);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetVariableCallback(IVariable* pVar, IVariable::OnSetCallback* func)
{
    pVar->AddOnSetCallback(func);
    m_callbacks.push_back(std::make_pair(pVar, func));
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::ClearCallbacks()
{
    if (m_pProperties)
    {
        m_pProperties->RemoveOnSetCallback(&m_onSetCallbacksCache[VariableCallbackIndex::OnPropertyChange]);
    }

    if (m_pProperties2)
    {
        m_pProperties2->RemoveOnSetCallback(&m_onSetCallbacksCache[VariableCallbackIndex::OnPropertyChange]);
    }

    for (auto iter = m_callbacks.begin(); iter != m_callbacks.end(); ++iter)
    {
        iter->first->RemoveOnSetCallback(iter->second);
    }

    m_callbacks.clear();
}

void CEntityObject::StoreUndoEntityLink(CSelectionGroup* pGroup)
{
    if (!pGroup)
    {
        return;
    }

    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoEntityLink(pGroup));
    }
}

template <typename T>
T CEntityObject::GetEntityProperty(const char* pName, T defaultvalue) const
{
    CVarBlock* pProperties = GetProperties2();
    IVariable* pVariable = nullptr;
    if (pProperties)
    {
        pVariable = pProperties->FindVariable(pName);
    }

    if (!pVariable)
    {
        pProperties = GetProperties();
        if (pProperties)
        {
            pVariable = pProperties->FindVariable(pName);
        }

        if (!pVariable)
        {
            return defaultvalue;
        }
    }

    if (pVariable->GetType() != IVariableType<T>::value)
    {
        return defaultvalue;
    }

    T value;
    pVariable->Get(value);
    return value;
}

template <typename T>
void CEntityObject::SetEntityProperty(const char* pName, T value)
{
    CVarBlock* pProperties = GetProperties2();
    IVariable* pVariable = nullptr;
    if (pProperties)
    {
        pVariable = pProperties->FindVariable(pName);
    }

    if (!pVariable)
    {
        pProperties = GetProperties();
        if (pProperties)
        {
            pVariable = pProperties->FindVariable(pName);
        }

        if (!pVariable)
        {
            throw std::runtime_error((QString("\"") + pName + "\" is an invalid property.").toUtf8().data());
        }
    }

    if (pVariable->GetType() != IVariableType<T>::value)
    {
        throw std::logic_error("Data type is invalid.");
    }
    pVariable->Set(value);
}

bool CEntityObject::GetEntityPropertyBool(const char* name) const
{
    return GetEntityProperty<bool>(name, false);
}

int CEntityObject::GetEntityPropertyInteger(const char* name) const
{
    return GetEntityProperty<int>(name, 0);
}

float CEntityObject::GetEntityPropertyFloat(const char* name) const
{
    return GetEntityProperty<float>(name, 0.0f);
}

QString CEntityObject::GetEntityPropertyString(const char* name) const
{
    return GetEntityProperty<QString>(name, "");
}

void CEntityObject::SetEntityPropertyBool(const char* name, bool value)
{
    SetEntityProperty<bool>(name, value);
}

void CEntityObject::SetEntityPropertyInteger(const char* name, int value)
{
    SetEntityProperty<int>(name, value);
}

void CEntityObject::SetEntityPropertyFloat(const char* name, float value)
{
    SetEntityProperty<float>(name, value);
}

void CEntityObject::SetEntityPropertyString(const char* name, const QString& value)
{
    SetEntityProperty<QString>(name, value);
}

#include <Objects/moc_EntityObject.cpp>
