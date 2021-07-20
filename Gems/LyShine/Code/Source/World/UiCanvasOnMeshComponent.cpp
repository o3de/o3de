/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "LyShine_precompiled.h"
#include "UiCanvasOnMeshComponent.h"
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <LmbrCentral/Rendering/RenderNodeBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/World/UiCanvasRefBus.h>

#include <Cry_Geo.h>
#include <IIndexedMesh.h>
#include <IEntityRenderState.h>
#include <IStatObj.h>

#if !defined(_RELEASE)
#include <IRenderAuxGeom.h>

// set this to 1 to enable debug display
#define UI_CANVAS_ON_MESH_DEBUG 0
#endif // !defined(_RELEASE)

#include <AzFramework/Render/GeometryIntersectionStructures.h>


////////////////////////////////////////////////////////////////////////////////////////////////////
// Anonymous namespace
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
#if UI_CANVAS_ON_MESH_DEBUG
    // Debug draw methods only used for debugging the collision

    static const ColorB debugHitColor(255, 0, 0, 255);
    static const ColorB debugCollisionMeshColor(255, 255, 0, 255);
    static const ColorB debugRenderMeshAttempt1Color(0, 255, 0, 255);
    static const ColorB debugRenderMeshAttempt2Color(0, 0, 255, 255);

    static const float debugDrawSphereSize = 0.01f;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void DrawSphere(const Vec3& point, const ColorB& color, float size)
    {
        IRenderAuxGeom* pRenderAux = gEnv->pRenderer->GetIRenderAuxGeom();
        pRenderAux->DrawSphere(point, size, color);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void DrawTrianglePoints(const Vec3& v0, const Vec3& v1, const Vec3& v2, const ColorB& color, float size)
    {
        IRenderAuxGeom* pRenderAux = gEnv->pRenderer->GetIRenderAuxGeom();
        pRenderAux->DrawSphere(v0, size, color);
        pRenderAux->DrawSphere(v1, size, color);
        pRenderAux->DrawSphere(v2, size, color);
    }

    void DrawCollisionMeshTrianglePoints(
        int triIndex,
        const IPhysicalEntity* collider,
        int partIndex,
        const Matrix34& slotWorldTM
        )
    {
        if (collider)
        {
            const IPhysicalEntity* pe = collider;
            pe_params_part partParams;
            partParams.ipart = partIndex;
            pe->GetParams(&partParams);
            phys_geometry* pPhysGeom = partParams.pPhysGeom;
            phys_geometry* pPhysGeomProxy = partParams.pPhysGeomProxy;

            IGeometry* geom = pPhysGeom->pGeom;
            Vec3 featurePoint[3];
            int feat = geom->GetFeature(triIndex, 0, featurePoint);
            primitives::triangle prim;
            int primResult = geom->GetPrimitive(triIndex, &prim);

            {
                // get verts in world space
                Vec3 wv0 = slotWorldTM.TransformPoint(prim.pt[0]);
                Vec3 wv1 = slotWorldTM.TransformPoint(prim.pt[1]);
                Vec3 wv2 = slotWorldTM.TransformPoint(prim.pt[2]);

                // offset the points that we draw by the debug sphere radius so they can be seen when
                // they are on top of the render mesh points
                Vec3 worldNormal = slotWorldTM.TransformVector(prim.n);

                wv0 += worldNormal * debugDrawSphereSize;
                wv1 += worldNormal * debugDrawSphereSize;
                wv2 += worldNormal * debugDrawSphereSize;

                DrawTrianglePoints(wv0, wv1, wv2, debugCollisionMeshColor, debugDrawSphereSize * 0.5f);
            }
        }
    }
#endif

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool GetBarycentricCoordinates(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& p, float& u, float& v, float& w, float fBorder)
    {
        // Compute vectors
        Vec3 v0 = b - a;
        Vec3 v1 = c - a;
        Vec3 v2 = p - a;

        // Compute dot products
        float dot00 = v0.Dot(v0);
        float dot01 = v0.Dot(v1);
        float dot02 = v0.Dot(v2);
        float dot11 = v1.Dot(v1);
        float dot12 = v1.Dot(v2);

        // Compute barycentric coordinates
        float invDenom = 1.f / (dot00 * dot11 - dot01 * dot01);
        v = (dot11 * dot02 - dot01 * dot12) * invDenom;
        w = (dot00 * dot12 - dot01 * dot02) * invDenom;
        u = 1.f - v - w;

        // Check if point is in triangle
        return (u >= -fBorder) && (v >= -fBorder) && (w >= -fBorder);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool SnapToPlaneAndGetBarycentricCoordinates(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& p, float& u, float& v, float& w)
    {
        // get face normal
        Vec3 uVec = b - a;
        Vec3 vVec = c - a;
        Vec3 faceNormal = uVec.cross(vVec);
        faceNormal.NormalizeSafe();
        Vec3 aToPt = p - a;
        float dist = aToPt.Dot(faceNormal);
        float distSq = dist * dist;
        float triLenSq = uVec.len2() + vVec.len2();

        // Is the point "close enough" to the plane of the triangle?
        if (distSq < triLenSq * 0.1f)
        {
            // snap the point to the plane of the triangle
            Vec3 coplanarP = p - dist * faceNormal;

            return GetBarycentricCoordinates(a, b, c, coplanarP, u, v, w, 0.0f);
        }

        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    Vec2 ConvertBarycentricCoordsToUVCoords(float u, float v, float w, Vec2 uv0, Vec2 uv1, Vec2 uv2)
    {
        float arrVertWeight[3] = { max(0.f, u), max(0.f, v), max(0.f, w) };
        float fDiv = 1.f / (arrVertWeight[0] + arrVertWeight[1] + arrVertWeight[2]);
        arrVertWeight[0] *= fDiv;
        arrVertWeight[1] *= fDiv;
        arrVertWeight[2] *= fDiv;

        Vec2 uvResult = uv0 * arrVertWeight[0] + uv1 * arrVertWeight[1] + uv2 * arrVertWeight[2];
        return uvResult;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool GetTexCoordFromRayHitOnIndexedMesh(
        int triIndex,
        Vec3 hitPoint,
        [[maybe_unused]] const IPhysicalEntity* collider,
        [[maybe_unused]] int partIndex,
        const Matrix34& slotWorldTM,
        const IIndexedMesh* indexedMesh,
        Vec2& texCoord)
    {
        IIndexedMesh::SMeshDescription meshDesc;
        indexedMesh->GetMeshDescription(meshDesc);

#if UI_CANVAS_ON_MESH_DEBUG
        DrawSphere(hitPoint, debugHitColor, debugDrawSphereSize);
#endif

        // triIndex is -1 if this is not a mesh collision (i.e. collided with a parametric primitive)
        if (triIndex >= 0 && triIndex * 3 <= meshDesc.m_nIndexCount)
        {
            // convert TriIndex into the indices into the index buffer
            int i0 = triIndex * 3;
            int i1 = i0 + 1;
            int i2 = i0 + 2;

            // get the vertex indices from the index buffer
            int vIndex0 = meshDesc.m_pIndices[i0];
            int vIndex1 = meshDesc.m_pIndices[i1];
            int vIndex2 = meshDesc.m_pIndices[i2];

            // get verts in local space
            Vec3 v0 = meshDesc.m_pVerts[vIndex0];
            Vec3 v1 = meshDesc.m_pVerts[vIndex1];
            Vec3 v2 = meshDesc.m_pVerts[vIndex2];

            // get verts in world space
            Vec3 wv0 = slotWorldTM.TransformPoint(v0);
            Vec3 wv1 = slotWorldTM.TransformPoint(v1);
            Vec3 wv2 = slotWorldTM.TransformPoint(v2);


#if UI_CANVAS_ON_MESH_DEBUG
            DrawCollisionMeshTrianglePoints(triIndex, collider, partIndex, slotWorldTM);
#endif

            float u, v, w;
            if (SnapToPlaneAndGetBarycentricCoordinates(wv0, wv1, wv2, hitPoint, u, v, w))
            {
#if UI_CANVAS_ON_MESH_DEBUG
                DrawTrianglePoints(wv0, wv1, wv2, debugRenderMeshAttempt1Color, debugDrawSphereSize);
#endif

                // get the texcoord for each vert of the triangle
                Vec2 uv0 = meshDesc.m_pTexCoord[vIndex0].GetUV();
                Vec2 uv1 = meshDesc.m_pTexCoord[vIndex1].GetUV();
                Vec2 uv2 = meshDesc.m_pTexCoord[vIndex2].GetUV();

                texCoord = ConvertBarycentricCoordsToUVCoords(u, v, w, uv0, uv1, uv2);

                return true;
            }
        }

        // If we got here then EITHER, the iPrim is 0xffffffff meaning that the collision
        // was a primitive rather than a mesh collision OR the iPrim is not the right
        // triangle index in the render mesh. This sometimes happens, presumably due to
        // some modifications that are made automatically to the collision mesh by the
        // physics system or something to do with how the IndexedMesh is generated on
        // demand in IStatObj:::GetIndexedMesh.
        // We do have the collision point though. So we go through all the triangles in
        // the render mesh and try to find the right triangle.
        // NOTE: This could be optimized by converting the hit point to local space.
        // NOTE: Currently we use the first triangle where the point is "close enough" to the plane
        // of the triangle and the barycentric calculation says that the point is within the
        // triangle. This "close enough" test is rather arbitrary and could get a false positive in
        // some edge cases.
        // Another approach would be to go through all the triangles doing the barycentric
        // test and keep track of which one that passes is closest to the plane of the triangle.
        int triCount = meshDesc.m_nIndexCount / 3;
        for (int i = 0; i < triCount; ++i)
        {
            // convert TriIndex into the indices into the index buffer
            int i0 = i * 3;
            int i1 = i0 + 1;
            int i2 = i0 + 2;

            // get the vertex indices from the index buffer
            int vIndex0 = meshDesc.m_pIndices[i0];
            int vIndex1 = meshDesc.m_pIndices[i1];
            int vIndex2 = meshDesc.m_pIndices[i2];

            // get verts in local space
            Vec3 v0 = meshDesc.m_pVerts[vIndex0];
            Vec3 v1 = meshDesc.m_pVerts[vIndex1];
            Vec3 v2 = meshDesc.m_pVerts[vIndex2];

            // get verts in world space
            Vec3 wv0 = slotWorldTM.TransformPoint(v0);
            Vec3 wv1 = slotWorldTM.TransformPoint(v1);
            Vec3 wv2 = slotWorldTM.TransformPoint(v2);

            float u, v, w;
            if (SnapToPlaneAndGetBarycentricCoordinates(wv0, wv1, wv2, hitPoint, u, v, w))
            {
#if UI_CANVAS_ON_MESH_DEBUG
                DrawTrianglePoints(wv0, wv1, wv2, debugRenderMeshAttempt2Color, debugDrawSphereSize);
#endif

                // get the texcoord for each vert of the triangle
                Vec2 uv0 = meshDesc.m_pTexCoord[vIndex0].GetUV();
                Vec2 uv1 = meshDesc.m_pTexCoord[vIndex1].GetUV();
                Vec2 uv2 = meshDesc.m_pTexCoord[vIndex2].GetUV();

                texCoord = ConvertBarycentricCoordsToUVCoords(u, v, w, uv0, uv1, uv2);

                return true;
            }
        }

        return false;
    }
} // Anonymous namespace

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasOnMeshComponent::UiCanvasOnMeshComponent()
{}

bool UiCanvasOnMeshComponent::ProcessHitInputEvent(const AzFramework::InputChannel::Snapshot& inputSnapshot, const AzFramework::RenderGeometry::RayResult& rayResult)
{
    return ProcessCollisionInputEventInternal(inputSnapshot, rayResult);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasOnMeshComponent::OnCanvasLoadedIntoEntity(AZ::EntityId uiCanvasEntity)
{
    if (uiCanvasEntity.IsValid() && !m_renderTargetOverride.empty())
    {
        EBUS_EVENT_ID(uiCanvasEntity, UiCanvasBus, SetRenderTargetName, m_renderTargetOverride);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasOnMeshComponent::OnCanvasReloaded(AZ::EntityId canvasEntityId)
{
    if (canvasEntityId == GetCanvas())
    {
        // The canvas that we are using has been reloaded, we may need to override the render target
        OnCanvasLoadedIntoEntity(canvasEntityId);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasOnMeshComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiCanvasOnMeshComponent, AZ::Component>()
            ->Version(1)
            ->Field("RenderTargetOverride", &UiCanvasOnMeshComponent::m_renderTargetOverride);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            auto editInfo = editContext->Class<UiCanvasOnMeshComponent>(
                    "UI Canvas on Mesh", "The UI Canvas on Mesh component allows you to place a UI Canvas on an entity in the 3D world that a player can interact with via ray casts");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/UiCanvasOnMesh.svg")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/UiCanvasOnMesh.png")
                ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/ui-canvas-on-mesh/")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c));

            editInfo->DataElement(0, &UiCanvasOnMeshComponent::m_renderTargetOverride,
                "Render target override",
                "If not empty, this name overrides the render target set on the UI canvas.\n"
                "This is useful if multiple instances the same UI canvas are rendered in the level.");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasOnMeshComponent::Activate()
{
    UiCanvasOnMeshBus::Handler::BusConnect(GetEntityId());
    UiCanvasAssetRefNotificationBus::Handler::BusConnect(GetEntityId());
    UiCanvasManagerNotificationBus::Handler::BusConnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasOnMeshComponent::Deactivate()
{
    UiCanvasAssetRefNotificationBus::Handler::BusDisconnect();
    UiCanvasOnMeshBus::Handler::BusDisconnect();
    UiCanvasManagerNotificationBus::Handler::BusDisconnect();
}

bool UiCanvasOnMeshComponent::ProcessCollisionInputEventInternal(const AzFramework::InputChannel::Snapshot& inputSnapshot, const AzFramework::RenderGeometry::RayResult& rayResult)
{
    AZ::EntityId canvasEntityId = GetCanvas();

    if (canvasEntityId.IsValid())
    {
        // Cache bus pointer as it will be used twice
        UiCanvasBus::BusPtr uiCanvasInterfacePtr;
        UiCanvasBus::Bind(uiCanvasInterfacePtr, canvasEntityId);
        if (!uiCanvasInterfacePtr)
        {
            return false;
        }

        AZ::Vector2 canvasSize;
        UiCanvasBus::EventResult(canvasSize, uiCanvasInterfacePtr, &UiCanvasInterface::GetCanvasSize);

        AZ::Vector2 canvasPoint = AZ::Vector2(rayResult.m_uv.GetX() * canvasSize.GetX(), rayResult.m_uv.GetY() * canvasSize.GetY());

        bool handledByCanvas = false;
        UiCanvasBus::EventResult(handledByCanvas, uiCanvasInterfacePtr,
            &UiCanvasInterface::HandleInputPositionalEvent, inputSnapshot, canvasPoint);

        if (handledByCanvas)
        {
            return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasOnMeshComponent::GetCanvas()
{
    AZ::EntityId result;
    EBUS_EVENT_ID_RESULT(result, GetEntityId(), UiCanvasRefBus, GetCanvas);
    return result;
}
