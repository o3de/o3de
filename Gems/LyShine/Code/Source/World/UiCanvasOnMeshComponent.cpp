/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasOnMeshComponent.h"
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/World/UiCanvasRefBus.h>

#include <Cry_Geo.h>
#include <IIndexedMesh.h>

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
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/UiCanvasOnMesh.svg")
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
