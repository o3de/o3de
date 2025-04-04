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
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Math/IntersectPoint.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/World/UiCanvasRefBus.h>
#include <LyShine/UiSerializeHelpers.h>

#include <Cry_Geo.h>
#include <IIndexedMesh.h>


#include <AzFramework/Render/GeometryIntersectionStructures.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Anonymous namespace
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector2 ConvertBarycentricCoordsToUVCoords(float u, float v, float w, AZ::Vector2 uv0, AZ::Vector2 uv1, AZ::Vector2 uv2)
    {
        float arrVertWeight[3] = { max(0.f, u), max(0.f, v), max(0.f, w) };
        float fDiv = 1.f / (arrVertWeight[0] + arrVertWeight[1] + arrVertWeight[2]);
        arrVertWeight[0] *= fDiv;
        arrVertWeight[1] *= fDiv;
        arrVertWeight[2] *= fDiv;

        AZ::Vector2 uvResult = uv0 * arrVertWeight[0] + uv1 * arrVertWeight[1] + uv2 * arrVertWeight[2];
        return uvResult;
    }

} // Anonymous namespace

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasOnMeshComponent::UiCanvasOnMeshComponent()
{}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasOnMeshComponent::ProcessHitInputEvent(
    const AzFramework::InputChannel::Snapshot& inputSnapshot,
    const AzFramework::RenderGeometry::RayRequest& rayRequest)
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

        // Calculate UV texture coordinates of the intersected geometry
        AZ::Vector2 uv(0.0f);
        if (CalculateUVFromRayIntersection(rayRequest, uv))
        {
            AZ::Vector2 canvasSize;
            UiCanvasBus::EventResult(canvasSize, uiCanvasInterfacePtr, &UiCanvasInterface::GetCanvasSize);
            AZ::Vector2 canvasPoint = AZ::Vector2(uv.GetX() * canvasSize.GetX(), uv.GetY() * canvasSize.GetY());

            bool handledByCanvas = false;
            UiCanvasBus::EventResult(handledByCanvas, uiCanvasInterfacePtr,
                &UiCanvasInterface::HandleInputPositionalEvent, inputSnapshot, canvasPoint);

            if (handledByCanvas)
            {
                return true;
            }
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasOnMeshComponent::OnCanvasLoadedIntoEntity(AZ::EntityId uiCanvasEntity)
{
    if (uiCanvasEntity.IsValid() && m_attachmentImageAssetOverride)
    {
        UiCanvasBus::Event(uiCanvasEntity, &UiCanvasInterface::SetAttachmentImageAsset, m_attachmentImageAssetOverride);
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
            ->Version(2, &VersionConverter)
            ->Field("AttachmentImageAssetOverride", &UiCanvasOnMeshComponent::m_attachmentImageAssetOverride);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            auto editInfo = editContext->Class<UiCanvasOnMeshComponent>(
                    "UI Canvas on Mesh", "The UI Canvas on Mesh component allows you to place a UI Canvas on an entity in the 3D world that a player can interact with via ray casts");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/UiCanvasOnMesh.svg")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/UiCanvasOnMesh.svg")
                ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/ui/canvas-on-mesh/")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"));

            editInfo->DataElement(0, &UiCanvasOnMeshComponent::m_attachmentImageAssetOverride,
                "Render target override",
                "If not empty, this asset overrides the render target set on the UI canvas.\n"
                "This is useful if multiple instances of the same UI canvas are rendered in the level.");
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

    // Check if a UI canvas has already been loaded into the entity
    AZ::EntityId canvasEntityId;
    UiCanvasRefBus::EventResult(
        canvasEntityId, GetEntityId(), &UiCanvasRefBus::Events::GetCanvas);
    if (canvasEntityId.IsValid())
    {
        OnCanvasLoadedIntoEntity(canvasEntityId);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasOnMeshComponent::Deactivate()
{
    UiCanvasAssetRefNotificationBus::Handler::BusDisconnect();
    UiCanvasOnMeshBus::Handler::BusDisconnect();
    UiCanvasManagerNotificationBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasOnMeshComponent::CalculateUVFromRayIntersection(const AzFramework::RenderGeometry::RayRequest& rayRequest, AZ::Vector2& outUv)
{
    outUv = AZ::Vector2(0.0f);

    // Make sure we can get the model asset
    AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset;
    AZ::Render::MeshComponentRequestBus::EventResult(
        modelAsset, GetEntityId(), &AZ::Render::MeshComponentRequestBus::Events::GetModelAsset);
    AZ::RPI::ModelAsset* asset = modelAsset.Get();
    if (!asset)
    {
        return false;
    }

    // Calculate the nearest point of collision
    AZ::Transform meshWorldTM;
    AZ::TransformBus::EventResult(meshWorldTM, GetEntityId(), &AZ::TransformInterface::GetWorldTM);
    AZ::Transform meshWorldTMInverse = meshWorldTM.GetInverse();

    AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
    AZ::NonUniformScaleRequestBus::EventResult(nonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

    const AZ::Vector3 clampedNonUniformScale = nonUniformScale.GetMax(AZ::Vector3(AZ::MinTransformScale));
    AZ::Vector3 rayOrigin = meshWorldTMInverse.TransformPoint(rayRequest.m_startWorldPosition) / clampedNonUniformScale;
    AZ::Vector3 rayEnd = meshWorldTMInverse.TransformPoint(rayRequest.m_endWorldPosition) / clampedNonUniformScale;
    AZ::Vector3 rayDirection = rayEnd - rayOrigin;

    // When a segment intersects a triangle, the returned hit distance will be between [0, 1].
    // Initialize min hit distance to be greater than 1 so that the first hit will be the new min
    float minResultDistance = 2.0f;
    bool foundResult = false;

    auto lods = modelAsset->GetLodAssets();
    if (lods.empty())
    {
        return false;
    }

    auto meshes = lods[0]->GetMeshes();
    for (const AZ::RPI::ModelLodAsset::Mesh& mesh : meshes)
    {
        // Find position and UV semantics
        static const AZ::Name positionName = AZ::Name::FromStringLiteral("POSITION", AZ::Interface<AZ::NameDictionary>::Get());
        static const AZ::Name uvName = AZ::Name::FromStringLiteral("UV", AZ::Interface<AZ::NameDictionary>::Get());
        auto streamBufferList = mesh.GetStreamBufferInfoList();
        const AZ::RPI::ModelLodAsset::Mesh::StreamBufferInfo* positionBuffer = nullptr;
        const AZ::RPI::ModelLodAsset::Mesh::StreamBufferInfo* uvBuffer = nullptr;

        for (const AZ::RPI::ModelLodAsset::Mesh::StreamBufferInfo& bufferInfo : streamBufferList)
        {
            if (bufferInfo.m_semantic.m_name == positionName)
            {
                positionBuffer = &bufferInfo;
            }
            else if ((bufferInfo.m_semantic.m_name == uvName) && (bufferInfo.m_semantic.m_index == 0))
            {
                uvBuffer = &bufferInfo;
            }
        }

        if (!positionBuffer || !uvBuffer)
        {
            continue;
        }

        auto positionBufferAsset = positionBuffer->m_bufferAssetView.GetBufferAsset();
        const float* rawPositionBuffer = (const float*)(positionBufferAsset->GetBuffer().begin());
        AZ_Assert(
            positionBuffer->m_bufferAssetView.GetBufferViewDescriptor().m_elementFormat == AZ::RHI::Format::R32G32B32_FLOAT,
            "Unexpected position element format.");

        auto uvBufferAsset = uvBuffer->m_bufferAssetView.GetBufferAsset();
        const float* rawUvBuffer = (const float*)(uvBufferAsset->GetBuffer().begin());
        AZ_Assert(
            uvBuffer->m_bufferAssetView.GetBufferViewDescriptor().m_elementFormat == AZ::RHI::Format::R32G32_FLOAT,
            "Unexpected UV element format.");

        auto indexBuffer = mesh.GetIndexBufferAssetView().GetBufferAsset();
        const uint32_t* rawIndexBuffer = (const uint32_t*)(indexBuffer->GetBuffer().begin());
        AZ_Assert(
            (indexBuffer->GetBufferViewDescriptor().m_elementCount % 3) == 0,
            "index buffer not a multiple of 3");

        AZ::Intersect::SegmentTriangleHitTester hitTester(rayOrigin, rayEnd);

        for (uint32_t index = 0; index < indexBuffer->GetBufferViewDescriptor().m_elementCount; index += 3)
        {
            uint32_t index1 = rawIndexBuffer[index];
            uint32_t index2 = rawIndexBuffer[index + 1];
            uint32_t index3 = rawIndexBuffer[index + 2];

            AZ::Vector3 vertex1(
                rawPositionBuffer[index1 * 3], rawPositionBuffer[(index1 * 3) + 1], rawPositionBuffer[(index1 * 3) + 2]);
            AZ::Vector3 vertex2(
                rawPositionBuffer[index2 * 3], rawPositionBuffer[(index2 * 3) + 1], rawPositionBuffer[(index2 * 3) + 2]);
            AZ::Vector3 vertex3(
                rawPositionBuffer[index3 * 3], rawPositionBuffer[(index3 * 3) + 1], rawPositionBuffer[(index3 * 3) + 2]);
            AZ::Vector3 resultNormal;

            float resultDistance = 0.0f;
            if (hitTester.IntersectSegmentTriangle(vertex1, vertex2, vertex3, resultNormal, resultDistance))
            {
                if (resultDistance < minResultDistance)
                {
                    AZ::Vector3 hitPosition = rayOrigin + (rayDirection * resultDistance);
                    AZ::Vector3 uvw = AZ::Intersect::Barycentric(vertex1, vertex2, vertex3, hitPosition);
                    if (uvw.IsGreaterEqualThan(AZ::Vector3::CreateZero()))
                    {
                        AZ::Vector3 uv1(rawUvBuffer[index1 * 2], rawUvBuffer[(index1 * 2) + 1], 0.0f);
                        AZ::Vector3 uv2(rawUvBuffer[index2 * 2], rawUvBuffer[(index2 * 2) + 1], 0.0f);
                        AZ::Vector3 uv3(rawUvBuffer[index3 * 2], rawUvBuffer[(index3 * 2) + 1], 0.0f);
                        outUv = ConvertBarycentricCoordsToUVCoords(
                            uvw.GetX(), uvw.GetY(), uvw.GetZ(), AZ::Vector2(uv1), AZ::Vector2(uv2), AZ::Vector2(uv3));
                        minResultDistance = resultDistance;
                        foundResult = true;
                    }
                }
            }
        }
    }

    return foundResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasOnMeshComponent::GetCanvas()
{
    AZ::EntityId result;
    UiCanvasRefBus::EventResult(result, GetEntityId(), &UiCanvasRefBus::Events::GetCanvas);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasOnMeshComponent::VersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    // conversion from version 1 to 2:
    // - Need to remove render target name as it was replaced with attachment image asset
    if (classElement.GetVersion() < 2)
    {
        if (!LyShine::RemoveRenderTargetAsString(context, classElement, "RenderTargetOverride"))
        {
            return false;
        }
    }

    return true;
}
