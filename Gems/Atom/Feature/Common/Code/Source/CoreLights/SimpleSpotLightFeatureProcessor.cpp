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

        void SimpleSpotLightFeatureProcessor::UpdateFrustum(LightHandle handle)
        {
            SimpleSpotLightData data = m_lightData.GetData<0>(handle.GetIndex());

            ViewFrustumAttributes desc;
            desc.m_aspectRatio = 1.0f;

            float radius = GetMax(data.m_radius, 0.001f);
            desc.m_nearClip = radius * 0.1f;
            desc.m_farClip = radius;
            desc.m_verticalFovRadians = GetMax(data.m_fovRadians, 0.001f);

            AZ::Vector3 position = AZ::Vector3::CreateFromFloat3(data.m_position.data());
            AZ::Vector3 normal = AZ::Vector3::CreateFromFloat3(data.m_direction.data());
            desc.m_worldTransform = AZ::Transform::CreateLookAt(position, position + normal);

            AZ::Frustum frustum = AZ::Frustum(desc);
            // Move the near plane onto the point of the light (the frustum can't be constructed this way due to divide by zero issues).
            frustum.SetPlane(AZ::Frustum::Near, AZ::Plane::CreateFromNormalAndPoint(desc.m_worldTransform.GetBasisY(), position));
            m_lightData.GetData<1>(handle.GetIndex()).Set(frustum);
        }

    } // namespace Render
} // namespace AZ
