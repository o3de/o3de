/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "EditorDefs.h"

#include "SurfaceInfoPicker.h"

// AzToolsFramework
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>

// Editor
#include "Material/MaterialManager.h"
#include "Objects/EntityObject.h"
#include "Viewport.h"
#include "QtViewPane.h"

// ComponentEntityEditorPlugin
#include "Plugins/ComponentEntityEditorPlugin/Objects/ComponentEntityObject.h"

static const float kEnoughFarDistance(5000.0f);

CSurfaceInfoPicker::CSurfaceInfoPicker()
    : m_pObjects(NULL)
    , m_pSetObjects(NULL)
    , m_PickOption(0)
{
    m_pActiveView = GetIEditor()->GetActiveView();
}

bool CSurfaceInfoPicker::PickByAABB(const QPoint& point, [[maybe_unused]] int nFlag, IDisplayViewport* pView, CExcludedObjects* pExcludedObjects, std::vector<CBaseObjectPtr>* pOutObjects)
{
    GetIEditor()->GetObjectManager()->GetObjects(m_objects);

    Vec3 vWorldRaySrc, vWorldRayDir;
    m_pActiveView->ViewToWorldRay(point, vWorldRaySrc, vWorldRayDir);
    vWorldRaySrc = vWorldRaySrc + vWorldRayDir * 0.1f;
    vWorldRayDir = vWorldRayDir * kEnoughFarDistance;

    bool bPicked = false;

    for (int i = 0, iCount(m_objects.size()); i < iCount; ++i)
    {
        if (pExcludedObjects && pExcludedObjects->Contains(m_objects[i]))
        {
            continue;
        }

        AABB worldObjAABB;
        m_objects[i]->GetBoundBox(worldObjAABB);

        if (pView)
        {
            float fScreenFactor = pView->GetScreenScaleFactor(m_objects[i]->GetPos());
            worldObjAABB.Expand(0.01f * Vec3(fScreenFactor, fScreenFactor, fScreenFactor));
        }

        Vec3 vHitPos;
        if (Intersect::Ray_AABB(vWorldRaySrc, vWorldRayDir, worldObjAABB, vHitPos))
        {
            if ((vHitPos - vWorldRaySrc).GetNormalized().Dot(vWorldRayDir) > 0 || worldObjAABB.IsContainPoint(vHitPos))
            {
                if (pOutObjects)
                {
                    pOutObjects->push_back(m_objects[i]);
                }
                bPicked = true;
            }
        }
    }

    return bPicked;
}

bool CSurfaceInfoPicker::PickImpl(const QPoint& point,
    _smart_ptr<IMaterial>* ppOutLastMaterial,
    SRayHitInfo& outHitInfo,
    CExcludedObjects* pExcludedObjects,
    int nFlag)
{
    Vec3 vWorldRaySrc;
    Vec3 vWorldRayDir;
    if (!m_pActiveView)
    {
        m_pActiveView = GetIEditor()->GetActiveView();
    }
    m_pActiveView->ViewToWorldRay(point, vWorldRaySrc, vWorldRayDir);
    vWorldRaySrc = vWorldRaySrc + vWorldRayDir * 0.1f;
    vWorldRayDir = vWorldRayDir * kEnoughFarDistance;

    return PickImpl(vWorldRaySrc, vWorldRayDir, ppOutLastMaterial, outHitInfo, pExcludedObjects, nFlag);
}

bool CSurfaceInfoPicker::PickImpl(const Vec3& vWorldRaySrc, const Vec3& vWorldRayDir,
    _smart_ptr<IMaterial>* ppOutLastMaterial,
    SRayHitInfo& outHitInfo,
    CExcludedObjects* pExcludedObjects,
    int nFlag)
{
    memset(&outHitInfo, 0, sizeof(outHitInfo));
    outHitInfo.fDistance = kEnoughFarDistance;

    if (m_pSetObjects)
    {
        m_pObjects = m_pSetObjects;
    }
    else
    {
        GetIEditor()->GetObjectManager()->GetObjects(m_objects);
        m_pObjects = &m_objects;
    }

    if (pExcludedObjects)
    {
        m_ExcludedObjects = *pExcludedObjects;
    }
    else
    {
        m_ExcludedObjects.Clear();
    }

    m_pPickedObject = NULL;

    if (nFlag & ePOG_Entity)
    {
        FindNearestInfoFromEntities(vWorldRaySrc, vWorldRayDir, ppOutLastMaterial, outHitInfo);
    }

    if (!m_pSetObjects)
    {
        m_objects.clear();
    }

    m_ExcludedObjects.Clear();

    return outHitInfo.fDistance < kEnoughFarDistance;
}

void CSurfaceInfoPicker::FindNearestInfoFromEntities(
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    _smart_ptr<IMaterial>* pOutLastMaterial,
    SRayHitInfo& outHitInfo) const
{
    for (size_t i = 0; i < m_pObjects->size(); ++i)
    {
        CBaseObject* object((*m_pObjects)[i]);

        if (object == nullptr
            || !qobject_cast<CEntityObject*>(object)
            || object->IsHidden()
            || IsFrozen(object)
            || m_ExcludedObjects.Contains(object)
            )
        {
            continue;
        }

        CEntityObject* entityObject = static_cast<CEntityObject*>(object);

        // If a legacy entity doesn't have a material override, we'll get the default material for that statObj
        _smart_ptr<IMaterial> statObjDefaultMaterial = nullptr;
        // And in some cases the material we want does comes from RayIntersection(), and we will skip AssignObjectMaterial().
        _smart_ptr<IMaterial> pickedMaterial = nullptr;

        bool hit = false;

        // entityObject is a component entity...
        if (entityObject->GetType() == OBJTYPE_AZENTITY)
        {
            AZ::EntityId id;
            AzToolsFramework::ComponentEntityObjectRequestBus::EventResult(id, entityObject, &AzToolsFramework::ComponentEntityObjectRequestBus::Events::GetAssociatedEntityId);

            // If the entity has an ActorComponent or another component that overrides RenderNodeRequestBus, it will hit here
            if (!hit)
            {
                // There might be multiple components with render nodes on the same entity
                // This will get the highest priority one, as determined by RenderNodeRequests::GetRenderNodeRequestBusOrder
                IRenderNode* renderNode = entityObject->GetEngineNode();

                // If the renderNode exists and is physicalized, it will hit here
                hit = RayIntersection_IRenderNode(vWorldRaySrc, vWorldRayDir, renderNode, &pickedMaterial, object->GetWorldTM(), outHitInfo);
                if (!hit)
                {
                    // If the renderNode is not physicalized, such as an actor component, but still exists and has a valid material we might want to pick
                    if (renderNode && renderNode->GetMaterial())
                    {
                        // Do a hit test with anything in this entity that has overridden EditorComponentSelectionRequestsBus          
                        CComponentEntityObject* componentEntityObject = static_cast<CComponentEntityObject*>(entityObject);
                        HitContext hc;
                        hc.raySrc = vWorldRaySrc;
                        hc.rayDir = vWorldRayDir;                       
                        bool intersects = componentEntityObject->HitTest(hc);
                        
                        if (intersects)
                        {
                            // If the distance is closer than the nearest distance so far
                            if (hc.dist < outHitInfo.fDistance)
                            {
                                hit = true;
                                outHitInfo.vHitPos = hc.raySrc + hc.rayDir * hc.dist;
                                outHitInfo.fDistance = hc.dist;
                                // We don't get material/sub-material information back from HitTest, so just use the material from the render node
                                pickedMaterial = renderNode->GetMaterial(); 
                                outHitInfo.nHitMatID = 0;
                                // We don't get normal information back from HitTest, so just orient the selection disk towards the camera
                                outHitInfo.vHitNormal = vWorldRayDir.normalized();
                            }
                        }
                    }
                }
            }
        }

        if (hit)
        {
            if(pickedMaterial)
            {
                AssignMaterial(pickedMaterial, outHitInfo, pOutLastMaterial);
            }
            else
            {
                if (object->GetMaterial())
                {
                    // If the entity has a material override, assign the object material
                    AssignObjectMaterial(object, outHitInfo, pOutLastMaterial);
                }
                else
                {
                    // Otherwise assign the default material for that object
                    AssignMaterial(statObjDefaultMaterial, outHitInfo, pOutLastMaterial);
                }
            }

            m_pPickedObject = object;
        }
    }
}

bool CSurfaceInfoPicker::RayIntersection_CBaseObject(
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    CBaseObject* pBaseObject,
    _smart_ptr<IMaterial>* pOutLastMaterial,
    SRayHitInfo& outHitInfo)
{
    if (pBaseObject == NULL)
    {
        return false;
    }
    IRenderNode* pRenderNode(pBaseObject->GetEngineNode());
    if (pRenderNode == NULL)
    {
        return false;
    }
    IStatObj* pStatObj(pRenderNode->GetEntityStatObj());
    if (pStatObj == NULL)
    {
        return false;
    }
    return RayIntersection(vWorldRaySrc, vWorldRayDir, pRenderNode, pStatObj, pBaseObject->GetWorldTM(), outHitInfo, pOutLastMaterial);
}

bool CSurfaceInfoPicker::RayIntersection(
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    IRenderNode* pRenderNode,
    IStatObj* pStatObj,
    const Matrix34A& WorldTM,
    SRayHitInfo& outHitInfo,
    _smart_ptr<IMaterial>* ppOutLastMaterial)
{
    SRayHitInfo hitInfo;
    bool bRayIntersection = false;
    _smart_ptr<IMaterial> pMaterial(NULL);

    bRayIntersection = RayIntersection_IStatObj(vWorldRaySrc, vWorldRayDir, pStatObj, &pMaterial, WorldTM, hitInfo);
    if (!bRayIntersection)
    {
        bRayIntersection = RayIntersection_IRenderNode(vWorldRaySrc, vWorldRayDir, pRenderNode, &pMaterial, WorldTM, hitInfo);
    }

    if (bRayIntersection)
    {
        hitInfo.fDistance = vWorldRaySrc.GetDistance(hitInfo.vHitPos);
        if (hitInfo.fDistance < outHitInfo.fDistance)
        {
            if (ppOutLastMaterial)
            {
                *ppOutLastMaterial = pMaterial;
            }
            memcpy(&outHitInfo, &hitInfo, sizeof(SRayHitInfo));
            outHitInfo.vHitNormal.Normalize();
            return true;
        }
    }
    return false;
}

bool CSurfaceInfoPicker::RayIntersection_IStatObj(
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    IStatObj* pStatObj,
    _smart_ptr<IMaterial>* ppOutLastMaterial,
    const Matrix34A& worldTM,
    SRayHitInfo& outHitInfo)
{
    if (pStatObj == NULL)
    {
        return false;
    }

    Vec3 localRaySrc;
    Vec3 localRayDir;
    if (!RayWorldToLocal(worldTM, vWorldRaySrc, vWorldRayDir, localRaySrc, localRayDir))
    {
        return false;
    }

    // the outHitInfo contains information about the previous closest hit and should not be cleared / replaced unless you have a better hit!
    // all of the intersection functions called (such as statobj's RayIntersection) only modify the hit info if it actually hits it closer than the current distance of the last hit.

    outHitInfo.inReferencePoint = localRaySrc;
    outHitInfo.inRay                = Ray(localRaySrc, localRayDir);
    outHitInfo.bInFirstHit  = false;
    outHitInfo.bUseCache        = false;

    _smart_ptr<IMaterial> hitMaterial = nullptr;

    Vec3 hitPosOnAABB;
    if (Intersect::Ray_AABB(outHitInfo.inRay, pStatObj->GetAABB(), hitPosOnAABB) == 0x00)
    {
        return false;
    }

    if (pStatObj->RayIntersection(outHitInfo))
    {
        if (outHitInfo.fDistance < 0)
        {
            return false;
        }
        outHitInfo.vHitPos = worldTM.TransformPoint(outHitInfo.vHitPos);
        outHitInfo.vHitNormal = worldTM.GetTransposed().GetInverted().TransformVector(outHitInfo.vHitNormal);

        // we need to set nHitSurfaceID anyway - so we need to do this regardless of whether the caller
        // has asked for detailed material info by passing in a non-null ppOutLastMaterial
        hitMaterial = pStatObj->GetMaterial();

        if (hitMaterial)
        {
            if (outHitInfo.nHitMatID >= 0)
            {
                if (hitMaterial->GetSubMtlCount() > 0 && outHitInfo.nHitMatID < hitMaterial->GetSubMtlCount())
                {
                    _smart_ptr<IMaterial> subMaterial = hitMaterial->GetSubMtl(outHitInfo.nHitMatID);
                    if (subMaterial)
                    {
                        hitMaterial = subMaterial;
                    }
                }
            }
            outHitInfo.nHitSurfaceID = hitMaterial->GetSurfaceTypeId();
        }

        if ((ppOutLastMaterial) && (hitMaterial))
        {
            *ppOutLastMaterial = hitMaterial;
        }

        return true;
    }

    return outHitInfo.fDistance < kEnoughFarDistance;
}

#if defined(USE_GEOM_CACHES)
bool CSurfaceInfoPicker::RayIntersection_IGeomCacheRenderNode(
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    IGeomCacheRenderNode* pGeomCacheRenderNode,
    _smart_ptr<IMaterial>* ppOutLastMaterial,
    [[maybe_unused]] const Matrix34A& worldTM,
    SRayHitInfo& outHitInfo)
{
    if (!pGeomCacheRenderNode)
    {
        return false;
    }

    SRayHitInfo newHitInfo = outHitInfo;
    newHitInfo.inReferencePoint = vWorldRaySrc;
    newHitInfo.inRay                = Ray(vWorldRaySrc, vWorldRayDir);
    newHitInfo.bInFirstHit  = false;
    newHitInfo.bUseCache        = false;

    if (pGeomCacheRenderNode->RayIntersection(newHitInfo))
    {
        if (newHitInfo.fDistance < 0 || newHitInfo.fDistance > kEnoughFarDistance || (outHitInfo.fDistance != 0 && newHitInfo.fDistance > outHitInfo.fDistance))
        {
            return false;
        }

        // Only override outHitInfo if the new hit is closer than the original hit
        outHitInfo = newHitInfo;
        if (ppOutLastMaterial)
        {
            _smart_ptr<IMaterial> pMaterial = pGeomCacheRenderNode->GetMaterial();
            if (pMaterial)
            {
                *ppOutLastMaterial = pMaterial;
                if (outHitInfo.nHitMatID >= 0)
                {
                    if (pMaterial->GetSubMtlCount() > 0 && outHitInfo.nHitMatID < pMaterial->GetSubMtlCount())
                    {
                        *ppOutLastMaterial = pMaterial->GetSubMtl(outHitInfo.nHitMatID);
                    }
                }
            }
        }
        return true;
    }

    return false;
}
#endif

bool CSurfaceInfoPicker::RayIntersection_IRenderNode(
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    IRenderNode* pRenderNode,
    _smart_ptr<IMaterial>* pOutLastMaterial,
    [[maybe_unused]] const Matrix34A& WorldTM,
    SRayHitInfo& outHitInfo)
{
    if (pRenderNode == NULL)
    {
        return false;
    }

    AZ_UNUSED(vWorldRaySrc)
    AZ_UNUSED(vWorldRayDir)
    AZ_UNUSED(pRenderNode)
    AZ_UNUSED(pOutLastMaterial)
    AZ_UNUSED(WorldTM)
    AZ_UNUSED(outHitInfo)
    return false;
}

bool CSurfaceInfoPicker::RayWorldToLocal(
    const Matrix34A& WorldTM,
    const Vec3& vWorldRaySrc,
    const Vec3& vWorldRayDir,
    Vec3& outRaySrc,
    Vec3& outRayDir)
{
    if (!WorldTM.IsValid())
    {
        return false;
    }
    Matrix34A invertedM(WorldTM.GetInverted());
    if (!invertedM.IsValid())
    {
        return false;
    }
    outRaySrc = invertedM.TransformPoint(vWorldRaySrc);
    outRayDir = invertedM.TransformVector(vWorldRayDir).GetNormalized();
    return true;
}

bool CSurfaceInfoPicker::IsMaterialValid(CMaterial* pMaterial)
{
    if (pMaterial == NULL)
    {
        return false;
    }
    return !(pMaterial->GetMatInfo()->GetFlags() & MTL_FLAG_NODRAW);
}

void CSurfaceInfoPicker::AssignObjectMaterial(CBaseObject* pObject, const SRayHitInfo& outHitInfo, _smart_ptr<IMaterial>* pOutMaterial)
{
    CMaterial* material = pObject->GetMaterial();
    if (material)
    {
        if (material->GetMatInfo())
        {
            if (pOutMaterial)
            {
                *pOutMaterial = material->GetMatInfo();
                if (*pOutMaterial)
                {
                    if (outHitInfo.nHitMatID >= 0 && (*pOutMaterial)->GetSubMtlCount() > 0 && outHitInfo.nHitMatID < (*pOutMaterial)->GetSubMtlCount())
                    {
                        *pOutMaterial = (*pOutMaterial)->GetSubMtl(outHitInfo.nHitMatID);
                    }
                }
            }
        }
    }
}

void CSurfaceInfoPicker::AssignMaterial(_smart_ptr<IMaterial> pMaterial, const SRayHitInfo& outHitInfo, _smart_ptr<IMaterial>* pOutMaterial)
{
    if (pOutMaterial)
    {
        *pOutMaterial = pMaterial;
        if (*pOutMaterial)
        {
            if (outHitInfo.nHitMatID >= 0 && (*pOutMaterial)->GetSubMtlCount() > 0 && outHitInfo.nHitMatID < (*pOutMaterial)->GetSubMtlCount())
            {
                *pOutMaterial = (*pOutMaterial)->GetSubMtl(outHitInfo.nHitMatID);
            }
        }
    }
}

void CSurfaceInfoPicker::SetActiveView(IDisplayViewport* view)
{
    if (view)
    {
        m_pActiveView = view;
    }
    else
    {
        m_pActiveView = GetIEditor()->GetActiveView();
    }
}
