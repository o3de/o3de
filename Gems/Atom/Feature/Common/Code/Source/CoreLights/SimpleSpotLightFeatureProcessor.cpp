/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/SimpleSpotLightFeatureProcessor.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>

#include <Atom/Feature/CoreLights/CoreLightsConstants.h>
#include <Atom/Feature/CoreLights/LightCommon.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>

#include <Atom/RHI/Factory.h>

#include <Atom/RPI.Public/ColorManagement/TransformColor.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>





#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>
#include <Atom/Feature/AuxGeom/AuxGeomFeatureProcessor.h>



namespace AZ
{
    namespace Render
    {
        void SimpleSpotLightFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto * serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<SimpleSpotLightFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        SimpleSpotLightFeatureProcessor::SimpleSpotLightFeatureProcessor()
            : SimpleSpotLightFeatureProcessorInterface()
        {
        }

        void SimpleSpotLightFeatureProcessor::Activate()
        {
            GpuBufferHandler::Descriptor desc;
            desc.m_bufferName = "SimpleSpotLightBuffer";
            desc.m_bufferSrgName = "m_simpleSpotLights";
            desc.m_elementCountSrgName = "m_simpleSpotLightCount";
            desc.m_elementSize = sizeof(SimpleSpotLightData);
            desc.m_srgLayout = RPI::RPISystemInterface::Get()->GetViewSrgLayout().get();

            m_lightBufferHandler = GpuBufferHandler(desc);

            MeshFeatureProcessor* meshFeatureProcessor = GetParentScene()->GetFeatureProcessor<MeshFeatureProcessor>();
            if (meshFeatureProcessor)
            {
                m_lightMeshFlag = meshFeatureProcessor->GetFlagRegistry()->AcquireTag(AZ::Name("SimpleSpotLight"));
            }
        }

        void SimpleSpotLightFeatureProcessor::Deactivate()
        {
            m_lightData.Clear();
            m_lightBufferHandler.Release();
        }

        SimpleSpotLightFeatureProcessor::LightHandle SimpleSpotLightFeatureProcessor::AcquireLight()
        {
            uint16_t id = m_lightData.GetFreeSlotIndex();

            if (id == MultiIndexedDataVector<SimpleSpotLightData>::NoFreeSlot)
            {
                return LightHandle::Null;
            }
            else
            {
                m_deviceBufferNeedsUpdate = true;
                return LightHandle(id);
            }
        }

        bool SimpleSpotLightFeatureProcessor::ReleaseLight(LightHandle& handle)
        {
            if (handle.IsValid())
            {
                m_lightData.RemoveIndex(handle.GetIndex());
                m_deviceBufferNeedsUpdate = true;
                handle.Reset();
                return true;
            }
            return false;
        }

        SimpleSpotLightFeatureProcessor::LightHandle SimpleSpotLightFeatureProcessor::CloneLight(LightHandle sourceLightHandle)
        {
            AZ_Assert(sourceLightHandle.IsValid(), "Invalid LightHandle passed to SimpleSpotLightFeatureProcessor::CloneLight().");

            LightHandle handle = AcquireLight();
            if (handle.IsValid())
            {
                m_lightData.GetData<0>(handle.GetIndex()) = m_lightData.GetData<0>(sourceLightHandle.GetIndex());
                m_lightData.GetData<1>(handle.GetIndex()) = m_lightData.GetData<1>(sourceLightHandle.GetIndex());
                m_deviceBufferNeedsUpdate = true;
            }
            return handle;
        }

        void SimpleSpotLightFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "SimpleSpotLightFeatureProcessor: Simulate");
            AZ_UNUSED(packet);

            if (m_deviceBufferNeedsUpdate)
            {
                m_lightBufferHandler.UpdateBuffer(m_lightData.GetDataVector<0>());
                m_deviceBufferNeedsUpdate = false;
            }
            LightCommon::MarkMeshesWithLightType(GetParentScene(), AZStd::span<const AZ::Frustum>(m_lightData.GetDataVector<1>()), m_lightMeshFlag.GetIndex());
        }

        void SimpleSpotLightFeatureProcessor::Render(const SimpleSpotLightFeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "SimpleSpotLightFeatureProcessor: Render");

            for (const RPI::ViewPtr& view : packet.m_views)
            {
                m_lightBufferHandler.UpdateSrg(view->GetShaderResourceGroup().get());
            }
        }

        void SimpleSpotLightFeatureProcessor::SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightRgbIntensity)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimpleSpotLightFeatureProcessor::SetRgbIntensity().");

            auto transformedColor = AZ::RPI::TransformColor(lightRgbIntensity, AZ::RPI::ColorSpaceId::LinearSRGB, AZ::RPI::ColorSpaceId::ACEScg);

            AZStd::array<float, 3>& rgbIntensity = m_lightData.GetData<0>(handle.GetIndex()).m_rgbIntensity;
            rgbIntensity[0] = transformedColor.GetR();
            rgbIntensity[1] = transformedColor.GetG();
            rgbIntensity[2] = transformedColor.GetB();

            m_deviceBufferNeedsUpdate = true;
        }

        void SimpleSpotLightFeatureProcessor::SetPosition(LightHandle handle, const AZ::Vector3& lightPosition)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimpleSpotLightFeatureProcessor::SetPosition().");

            AZStd::array<float, 3>& position = m_lightData.GetData<0>(handle.GetIndex()).m_position;
            lightPosition.StoreToFloat3(position.data());

            UpdateFrustum(handle);

            m_deviceBufferNeedsUpdate = true;
        }
        
        void SimpleSpotLightFeatureProcessor::SetDirection(LightHandle handle, const AZ::Vector3& lightDirection)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimpleSpotLightFeatureProcessor::SetDirection().");

            AZStd::array<float, 3>& direction = m_lightData.GetData<0>(handle.GetIndex()).m_direction;
            lightDirection.StoreToFloat3(direction.data());

            UpdateFrustum(handle);

            m_deviceBufferNeedsUpdate = true;
        }

        void SimpleSpotLightFeatureProcessor::SetConeAngles(LightHandle handle, float innerRadians, float outerRadians)
        {
            SimpleSpotLightData& data = m_lightData.GetData<0>(handle.GetIndex());
            data.m_cosInnerConeAngle = cosf(innerRadians);
            data.m_cosOuterConeAngle = cosf(outerRadians);
            data.m_fovRadians = outerRadians * 2.0f;

            UpdateFrustum(handle);
        }

        void SimpleSpotLightFeatureProcessor::SetAttenuationRadius(LightHandle handle, float attenuationRadius)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimpleSpotLightFeatureProcessor::SetAttenuationRadius().");

            attenuationRadius = AZStd::max<float>(attenuationRadius, 0.001f); // prevent divide by zero.
            float invAttenuationRadiusSquared = 1.0f / (attenuationRadius * attenuationRadius);

            SimpleSpotLightData& data = m_lightData.GetData<0>(handle.GetIndex());
            data.m_invAttenuationRadiusSquared = invAttenuationRadiusSquared;
            data.m_radius = attenuationRadius;

            UpdateFrustum(handle);

            m_deviceBufferNeedsUpdate = true;
        }

        void SimpleSpotLightFeatureProcessor::SetAffectsGI(LightHandle handle, bool affectsGI)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimpleSpotLightFeatureProcessor::SetAffectsGI().");

            m_lightData.GetData<0>(handle.GetIndex()).m_affectsGI = affectsGI;
            m_deviceBufferNeedsUpdate = true;
        }

        void SimpleSpotLightFeatureProcessor::SetAffectsGIFactor(LightHandle handle, float affectsGIFactor)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimpleSpotLightFeatureProcessor::SetAffectsGIFactor().");

            m_lightData.GetData<0>(handle.GetIndex()).m_affectsGIFactor = affectsGIFactor;
            m_deviceBufferNeedsUpdate = true;
        }

        const Data::Instance<RPI::Buffer> SimpleSpotLightFeatureProcessor::GetLightBuffer() const
        {
            return m_lightBufferHandler.GetBuffer();
        }

        uint32_t SimpleSpotLightFeatureProcessor::GetLightCount() const
        {
            return m_lightBufferHandler.GetElementCount();
        }

        void DebugDrawFrustum(const AZ::Frustum& f, RPI::AuxGeomDraw* auxGeom, const AZ::Color color, [[maybe_unused]] AZ::u8 lineWidth = 1)
        {
            using namespace ShapeIntersection;

            enum CornerIndices {
                NearTopLeft, NearTopRight, NearBottomLeft, NearBottomRight,
                FarTopLeft, FarTopRight, FarBottomLeft, FarBottomRight
            };
            Vector3 corners[8];

            if (IntersectThreePlanes(f.GetPlane(Frustum::PlaneId::Near), f.GetPlane(Frustum::PlaneId::Top), f.GetPlane(Frustum::PlaneId::Left), corners[NearTopLeft]) &&
                IntersectThreePlanes(f.GetPlane(Frustum::PlaneId::Near), f.GetPlane(Frustum::PlaneId::Top), f.GetPlane(Frustum::PlaneId::Right), corners[NearTopRight]) &&
                IntersectThreePlanes(f.GetPlane(Frustum::PlaneId::Near), f.GetPlane(Frustum::PlaneId::Bottom), f.GetPlane(Frustum::PlaneId::Left), corners[NearBottomLeft]) &&
                IntersectThreePlanes(f.GetPlane(Frustum::PlaneId::Near), f.GetPlane(Frustum::PlaneId::Bottom), f.GetPlane(Frustum::PlaneId::Right), corners[NearBottomRight]) &&
                IntersectThreePlanes(f.GetPlane(Frustum::PlaneId::Far), f.GetPlane(Frustum::PlaneId::Top), f.GetPlane(Frustum::PlaneId::Left), corners[FarTopLeft]) &&
                IntersectThreePlanes(f.GetPlane(Frustum::PlaneId::Far), f.GetPlane(Frustum::PlaneId::Top), f.GetPlane(Frustum::PlaneId::Right), corners[FarTopRight]) &&
                IntersectThreePlanes(f.GetPlane(Frustum::PlaneId::Far), f.GetPlane(Frustum::PlaneId::Bottom), f.GetPlane(Frustum::PlaneId::Left), corners[FarBottomLeft]) &&
                IntersectThreePlanes(f.GetPlane(Frustum::PlaneId::Far), f.GetPlane(Frustum::PlaneId::Bottom), f.GetPlane(Frustum::PlaneId::Right), corners[FarBottomRight]))
            {

                uint32_t lineIndices[24]{
                    //near plane
                    NearTopLeft, NearTopRight,
                    NearTopRight, NearBottomRight,
                    NearBottomRight, NearBottomLeft,
                    NearBottomLeft, NearTopLeft,

                    //Far plane
                    FarTopLeft, FarTopRight,
                    FarTopRight, FarBottomRight,
                    FarBottomRight, FarBottomLeft,
                    FarBottomLeft, FarTopLeft,

                    //Near-to-Far connecting lines
                    NearTopLeft, FarTopLeft,
                    NearTopRight, FarTopRight,
                    NearBottomLeft, FarBottomLeft,
                    NearBottomRight, FarBottomRight
                };
                RPI::AuxGeomDraw::AuxGeomDynamicIndexedDrawArguments drawArgs;
                drawArgs.m_verts = corners;
                drawArgs.m_vertCount = 8;
                drawArgs.m_indices = lineIndices;
                drawArgs.m_indexCount = 24;
                drawArgs.m_colors = &color;
                drawArgs.m_colorCount = 1;
                auxGeom->DrawLines(drawArgs);

                uint32_t triangleIndices[36]{
                    //near
                    NearBottomLeft, NearTopLeft, NearTopRight,
                    NearBottomLeft, NearTopRight, NearBottomRight,

                    //far
                    FarBottomRight, FarTopRight, FarTopLeft,
                    FarBottomRight, FarTopLeft, FarBottomLeft,

                    //left
                    FarBottomLeft, NearBottomLeft, NearTopLeft,
                    FarBottomLeft, NearTopLeft, FarTopLeft,

                    //right
                    NearBottomRight, NearTopRight, FarTopRight,
                    NearBottomRight, FarTopRight, FarBottomRight,

                    //bottom
                    FarBottomLeft, NearBottomLeft, NearBottomRight,
                    FarBottomLeft, NearBottomRight, FarBottomRight,

                    //top
                    NearTopLeft, FarTopLeft, FarTopRight,
                    NearTopLeft, FarTopRight, NearTopRight
                };
                Color transparentColor(color.GetR(), color.GetG(), color.GetB(), color.GetA() * 0.3f);
                drawArgs.m_indices = triangleIndices;
                drawArgs.m_indexCount = 36;
                drawArgs.m_colors = &transparentColor;
                auxGeom->DrawTriangles(drawArgs);

                // plane normals
                Vector3 planeNormals[] =
                {
                    //near
                    0.25f * (corners[NearBottomLeft] + corners[NearBottomRight] + corners[NearTopLeft] + corners[NearTopRight]),
                    0.25f * (corners[NearBottomLeft] + corners[NearBottomRight] + corners[NearTopLeft] + corners[NearTopRight]) + f.GetPlane(Frustum::PlaneId::Near).GetNormal(),

                    //far
                    0.25f * (corners[FarBottomLeft] + corners[FarBottomRight] + corners[FarTopLeft] + corners[FarTopRight]),
                    0.25f * (corners[FarBottomLeft] + corners[FarBottomRight] + corners[FarTopLeft] + corners[FarTopRight]) + f.GetPlane(Frustum::PlaneId::Far).GetNormal(),

                    //left
                    0.5f * (corners[NearBottomLeft] + corners[NearTopLeft]),
                    0.5f * (corners[NearBottomLeft] + corners[NearTopLeft]) + f.GetPlane(Frustum::PlaneId::Left).GetNormal(),

                    //right
                    0.5f * (corners[NearBottomRight] + corners[NearTopRight]),
                    0.5f * (corners[NearBottomRight] + corners[NearTopRight]) + f.GetPlane(Frustum::PlaneId::Right).GetNormal(),

                    //bottom
                    0.5f * (corners[NearBottomLeft] + corners[NearBottomRight]),
                    0.5f * (corners[NearBottomLeft] + corners[NearBottomRight]) + f.GetPlane(Frustum::PlaneId::Bottom).GetNormal(),

                    //top
                    0.5f * (corners[NearTopLeft] + corners[NearTopRight]),
                    0.5f * (corners[NearTopLeft] + corners[NearTopRight]) + f.GetPlane(Frustum::PlaneId::Top).GetNormal(),
                };
                Color planeNormalColors[] =
                {
                    Colors::Red, Colors::Red,       //near
                    Colors::Green, Colors::Green,   //far
                    Colors::Blue, Colors::Blue,     //left
                    Colors::Orange, Colors::Orange, //right
                    Colors::Pink, Colors::Pink,     //bottom
                    Colors::MediumPurple, Colors::MediumPurple, //top
                };
                RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments planeNormalLineArgs;
                planeNormalLineArgs.m_verts = planeNormals;
                planeNormalLineArgs.m_vertCount = 12;
                planeNormalLineArgs.m_colors = planeNormalColors;
                planeNormalLineArgs.m_colorCount = planeNormalLineArgs.m_vertCount;
                planeNormalLineArgs.m_depthTest = RPI::AuxGeomDraw::DepthTest::Off;
                auxGeom->DrawLines(planeNormalLineArgs);
            }
            else
            {
                AZ_Assert(false, "invalid frustum, cannot draw");
            }
        }

        void SimpleSpotLightFeatureProcessor::UpdateFrustum(LightHandle handle)
        {
            SimpleSpotLightData data = m_lightData.GetData<0>(handle.GetIndex());

            ViewFrustumAttributes desc;
            desc.m_aspectRatio = 1.0f;
            desc.m_nearClip = data.m_radius * 0.1f;
            desc.m_farClip = data.m_radius;
            desc.m_verticalFovRadians = data.m_fovRadians;

            AZ::Vector3 position = AZ::Vector3::CreateFromFloat3(data.m_position.data());
            AZ::Vector3 normal = AZ::Vector3::CreateFromFloat3(data.m_direction.data());
            desc.m_worldTransform = AZ::Transform::CreateLookAt(position, position + normal);

            m_lightData.GetData<1>(handle.GetIndex()).Set(AZ::Frustum(desc));

            // debug

            AZ::Frustum& frustum = m_lightData.GetData<1>(handle.GetIndex());
            AuxGeomFeatureProcessor* auxGeoFeatureProcessor = GetParentScene()->GetFeatureProcessor<AuxGeomFeatureProcessor>();
            if (auxGeoFeatureProcessor)
            {
                DebugDrawFrustum(frustum, auxGeoFeatureProcessor->GetDrawQueue().get(), AZ::Color::CreateOne());
            }

        }

    } // namespace Render
} // namespace AZ
