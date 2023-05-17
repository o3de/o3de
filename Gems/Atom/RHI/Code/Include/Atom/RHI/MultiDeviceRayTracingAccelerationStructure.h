/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI/MultiDeviceIndexBufferView.h>
#include <Atom/RHI/MultiDeviceStreamBufferView.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace RHI
    {
        class MultiDeviceRayTracingBufferPools;

        /////////////////////////////////////////////////////////////////////////////////////////////
        // Bottom Level Acceleration Structure (BLAS)

        //! MultiDeviceRayTracingGeometry
        //!
        //! The geometry entry contains the vertex and index buffers associated with geometry in the
        //! scene.  Each MultiDeviceRayTracingBlas contains a list of these entries.
        struct MultiDeviceRayTracingGeometry
        {
            RHI::Format m_vertexFormat = RHI::Format::Unknown;
            RHI::MultiDeviceStreamBufferView m_vertexBuffer;
            RHI::MultiDeviceIndexBufferView m_indexBuffer;
            // [GFX TODO][ATOM-4989] Add DXR BLAS Transform Buffer
        };
        using MultiDeviceRayTracingGeometryVector = AZStd::vector<MultiDeviceRayTracingGeometry>;

        //! MultiDeviceRayTracingBlasDescriptor
        //!
        //! The Build() operation in the descriptor allows the BLAS to be initialized
        //! using the following pattern:
        //!
        //! RHI::MultiDeviceRayTracingBlasDescriptor descriptor;
        //! descriptor.Build()
        //!    ->Geometry()
        //!        ->VertexFormat(RHI::Format::R32G32B32_FLOAT)
        //!        ->VertexBuffer(vertexBufferView)
        //!        ->IndexBuffer(indexBufferView)
        //!    ;
        class MultiDeviceRayTracingBlasDescriptor final
        {
        public:
            MultiDeviceRayTracingBlasDescriptor() = default;
            ~MultiDeviceRayTracingBlasDescriptor() = default;

            RayTracingBlasDescriptor GetDeviceRayTracingBlasDescriptor(int deviceIndex) const;

            // accessors
            const MultiDeviceRayTracingGeometryVector& GetGeometries() const
            {
                return m_geometries;
            }
            MultiDeviceRayTracingGeometryVector& GetGeometries()
            {
                return m_geometries;
            }

            // build operations
            MultiDeviceRayTracingBlasDescriptor* Build();
            MultiDeviceRayTracingBlasDescriptor* Geometry();
            MultiDeviceRayTracingBlasDescriptor* VertexBuffer(const RHI::MultiDeviceStreamBufferView& vertexBuffer);
            MultiDeviceRayTracingBlasDescriptor* VertexFormat(RHI::Format vertexFormat);
            MultiDeviceRayTracingBlasDescriptor* IndexBuffer(const RHI::MultiDeviceIndexBufferView& indexBuffer);

        private:
            MultiDeviceRayTracingGeometryVector m_geometries;
            MultiDeviceRayTracingGeometry* m_buildContext = nullptr;
        };

        //! MultiDeviceRayTracingBlas
        //!
        //! A MultiDeviceRayTracingBlas is created from the information in the MultiDeviceRayTracingBlasDescriptor.
        class MultiDeviceRayTracingBlas : public MultiDeviceObject
        {
        public:
            AZ_CLASS_ALLOCATOR(MultiDeviceRayTracingBlas, AZ::SystemAllocator, 0);
            AZ_RTTI(MultiDeviceRayTracingBlas, "{D17E050F-ECC2-4C20-A073-F43008F2D168}", MultiDeviceObject)
            MultiDeviceRayTracingBlas() = default;
            virtual ~MultiDeviceRayTracingBlas() = default;

            const RHI::Ptr<RayTracingBlas>& GetDeviceRayTracingBlas(int deviceIndex)
            {
                AZ_Error(
                    "MultiDeviceRayTracingBlas",
                    m_deviceRayTracingBlas.find(deviceIndex) != m_deviceRayTracingBlas.end(),
                    "No DeviceRayTracingBlas found for device index %d\n",
                    deviceIndex);

                return m_deviceRayTracingBlas.at(deviceIndex);
            }

            //! Creates the internal BLAS buffers from the descriptor
            ResultCode CreateBuffers(
                MultiDevice::DeviceMask deviceMask,
                const MultiDeviceRayTracingBlasDescriptor* descriptor,
                const MultiDeviceRayTracingBufferPools& rayTracingBufferPools);

            //! Returns true if the MultiDeviceRayTracingBlas has been initialized
            bool IsValid() const;

        private:
            AZStd::unordered_map<int, Ptr<RayTracingBlas>> m_deviceRayTracingBlas;
            MultiDeviceRayTracingBlasDescriptor m_descriptor;
        };

        /////////////////////////////////////////////////////////////////////////////////////////////
        // Top Level Acceleration Structure (TLAS)

        //! MultiDeviceRayTracingTlasInstance
        //!
        //! Each TLAS instance entry refers to a MultiDeviceRayTracingBlas, and can contain a transform which
        //! will be applied to all of the geometry entries in the Blas.  It also contains a hitGroupIndex
        //! which is used to index into the MultiDeviceRayTracingShaderTable to determine the hit shader when a
        //! ray hits any geometry in the instance.
        struct MultiDeviceRayTracingTlasInstance
        {
            uint32_t m_instanceID = 0;
            uint32_t m_hitGroupIndex = 0;
            AZ::Transform m_transform = AZ::Transform::CreateIdentity();
            AZ::Vector3 m_nonUniformScale = AZ::Vector3::CreateOne();
            bool m_transparent = false;
            RHI::Ptr<RHI::MultiDeviceRayTracingBlas> m_blas;
        };
        using MultiDeviceRayTracingTlasInstanceVector = AZStd::vector<MultiDeviceRayTracingTlasInstance>;

        //! MultiDeviceRayTracingTlasDescriptor
        //!
        //! The Build() operation in the descriptor allows the TLAS to be initialized
        //! using the following pattern:
        //!
        //! RHI::MultiDeviceRayTracingTlasDescriptor descriptor;
        //! descriptor.Build()
        //!     ->Instance()
        //!         ->InstanceID(0)
        //!         ->HitGroupIndex(0)
        //!         ->Blas(blas1)
        //!         ->Transform(transform1)
        //!     ->Instance()
        //!         ->InstanceID(1)
        //!         ->HitGroupIndex(1)
        //!         ->Blas(blas2)
        //!         ->Transform(transform2)
        //!     ;
        class MultiDeviceRayTracingTlasDescriptor final
        {
        public:
            MultiDeviceRayTracingTlasDescriptor() = default;
            ~MultiDeviceRayTracingTlasDescriptor() = default;

            RayTracingTlasDescriptor GetDeviceRayTracingTlasDescriptor(int deviceIndex) const;

            // accessors
            const MultiDeviceRayTracingTlasInstanceVector& GetInstances() const
            {
                return m_instances;
            }
            MultiDeviceRayTracingTlasInstanceVector& GetInstances()
            {
                return m_instances;
            }

            const RHI::Ptr<RHI::MultiDeviceBuffer>& GetInstancesBuffer() const
            {
                return m_instancesBuffer;
            }
            RHI::Ptr<RHI::MultiDeviceBuffer>& GetInstancesBuffer()
            {
                return m_instancesBuffer;
            }

            uint32_t GetNumInstancesInBuffer() const
            {
                return m_numInstancesInBuffer;
            }

            // build operations
            MultiDeviceRayTracingTlasDescriptor* Build();
            MultiDeviceRayTracingTlasDescriptor* Instance();
            MultiDeviceRayTracingTlasDescriptor* InstanceID(uint32_t instanceID);
            MultiDeviceRayTracingTlasDescriptor* HitGroupIndex(uint32_t hitGroupIndex);
            MultiDeviceRayTracingTlasDescriptor* Transform(const AZ::Transform& transform);
            MultiDeviceRayTracingTlasDescriptor* NonUniformScale(const AZ::Vector3& nonUniformScale);
            MultiDeviceRayTracingTlasDescriptor* Transparent(bool transparent);
            MultiDeviceRayTracingTlasDescriptor* Blas(RHI::Ptr<RHI::MultiDeviceRayTracingBlas>& blas);
            MultiDeviceRayTracingTlasDescriptor* InstancesBuffer(RHI::Ptr<RHI::MultiDeviceBuffer>& tlasInstances);
            MultiDeviceRayTracingTlasDescriptor* NumInstances(uint32_t numInstancesInBuffer);

        private:
            MultiDeviceRayTracingTlasInstanceVector m_instances;
            MultiDeviceRayTracingTlasInstance* m_buildContext = nullptr;

            // externally created Instances buffer, cannot be combined with other Instances
            RHI::Ptr<RHI::MultiDeviceBuffer> m_instancesBuffer;
            uint32_t m_numInstancesInBuffer;
        };

        //! MultiDeviceRayTracingTlas
        //!
        //! A MultiDeviceRayTracingTlas is created from the information in the MultiDeviceRayTracingTlasDescriptor.
        class MultiDeviceRayTracingTlas : public MultiDeviceObject
        {
        public:
            AZ_CLASS_ALLOCATOR(MultiDeviceRayTracingTlas, AZ::SystemAllocator, 0);
            AZ_RTTI(MultiDeviceRayTracingTlas, "{A2B0F8F1-D0B5-4D90-8AFA-CEF543D20E34}", MultiDeviceObject)
            MultiDeviceRayTracingTlas() = default;
            virtual ~MultiDeviceRayTracingTlas() = default;

            const RHI::Ptr<RayTracingTlas>& GetDeviceRayTracingTlas(int deviceIndex)
            {
                AZ_Error(
                    "MultiDeviceRayTracingTlas",
                    m_deviceRayTracingTlas.find(deviceIndex) != m_deviceRayTracingTlas.end(),
                    "No DeviceRayTracingTlas found for device index %d\n",
                    deviceIndex);

                return m_deviceRayTracingTlas.at(deviceIndex);
            }

            //! Creates the internal TLAS buffers from the descriptor
            ResultCode CreateBuffers(
                MultiDevice::DeviceMask deviceMask,
                const MultiDeviceRayTracingTlasDescriptor* descriptor,
                const MultiDeviceRayTracingBufferPools& rayTracingBufferPools);

            //! Returns the TLAS RHI buffer
            const RHI::Ptr<RHI::MultiDeviceBuffer> GetTlasBuffer() const;
            const RHI::Ptr<RHI::MultiDeviceBuffer> GetTlasInstancesBuffer() const;

        private:
            AZStd::unordered_map<int, Ptr<RayTracingTlas>> m_deviceRayTracingTlas;
            MultiDeviceRayTracingTlasDescriptor m_descriptor;
        };
    } // namespace RHI
} // namespace AZ
