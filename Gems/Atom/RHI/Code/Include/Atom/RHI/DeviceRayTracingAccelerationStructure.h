/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI/DeviceIndexBufferView.h>
#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI/DeviceRayTracingBufferPools.h>
#include <Atom/RHI/DeviceStreamBufferView.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace RHI
    {
        /////////////////////////////////////////////////////////////////////////////////////////////
        // Bottom Level Acceleration Structure (BLAS)

        //! DeviceRayTracingGeometry
        //!
        //! The geometry entry contains the vertex and index buffers associated with geometry in the
        //! scene.  Each DeviceRayTracingBlas contains a list of these entries.
        struct DeviceRayTracingGeometry
        {
            RHI::Format m_vertexFormat = RHI::Format::Unknown;
            RHI::DeviceStreamBufferView m_vertexBuffer;
            RHI::DeviceIndexBufferView m_indexBuffer;
            // [GFX TODO][ATOM-4989] Add DXR BLAS Transform Buffer
        };
        using DeviceRayTracingGeometryVector = AZStd::vector<DeviceRayTracingGeometry>;

        //! DeviceRayTracingBlasDescriptor
        //!
        //! The Build() operation in the descriptor allows the BLAS to be initialized
        //! using the following pattern:
        //!
        //! RHI::DeviceRayTracingBlasDescriptor descriptor;
        //! descriptor.Build()
        //!    ->Geometry()
        //!        ->VertexFormat(RHI::Format::R32G32B32_FLOAT)
        //!        ->VertexBuffer(vertexBufferView)
        //!        ->IndexBuffer(indexBufferView)
        //!    ;
        class DeviceRayTracingBlasDescriptor final
        {
        public:
            DeviceRayTracingBlasDescriptor() = default;
            ~DeviceRayTracingBlasDescriptor() = default;

            // accessors
            const DeviceRayTracingGeometryVector& GetGeometries() const
            {
                return m_geometries;
            }
            DeviceRayTracingGeometryVector& GetGeometries()
            {
                return m_geometries;
            }

            // build operations
            DeviceRayTracingBlasDescriptor* Build();
            DeviceRayTracingBlasDescriptor* Geometry();
            DeviceRayTracingBlasDescriptor* VertexBuffer(const RHI::DeviceStreamBufferView& vertexBuffer);
            DeviceRayTracingBlasDescriptor* VertexFormat(RHI::Format vertexFormat);
            DeviceRayTracingBlasDescriptor* IndexBuffer(const RHI::DeviceIndexBufferView& indexBuffer);

        private:
            DeviceRayTracingGeometryVector m_geometries;
            DeviceRayTracingGeometry* m_buildContext = nullptr;
        };

        //! DeviceRayTracingBlas
        //!
        //! A DeviceRayTracingBlas is created from the information in the DeviceRayTracingBlasDescriptor.
        class DeviceRayTracingBlas : public DeviceObject
        {
        public:
            DeviceRayTracingBlas() = default;
            virtual ~DeviceRayTracingBlas() = default;

            static RHI::Ptr<RHI::DeviceRayTracingBlas> CreateRHIRayTracingBlas();

            //! Creates the internal BLAS buffers from the descriptor
            ResultCode CreateBuffers(
                Device& device, const DeviceRayTracingBlasDescriptor* descriptor, const DeviceRayTracingBufferPools& rayTracingBufferPools);

            //! Returns true if the DeviceRayTracingBlas has been initialized
            virtual bool IsValid() const = 0;

        private:
            // Platform API
            virtual RHI::ResultCode CreateBuffersInternal(
                RHI::Device& deviceBase,
                const RHI::DeviceRayTracingBlasDescriptor* descriptor,
                const DeviceRayTracingBufferPools& rayTracingBufferPools) = 0;
        };

        /////////////////////////////////////////////////////////////////////////////////////////////
        // Top Level Acceleration Structure (TLAS)

        //! DeviceRayTracingTlasInstance
        //!
        //! Each TLAS instance entry refers to a DeviceRayTracingBlas, and can contain a transform which
        //! will be applied to all of the geometry entries in the Blas.  It also contains a hitGroupIndex
        //! which is used to index into the RayTracingShaderTable to determine the hit shader when a
        //! ray hits any geometry in the instance.
        struct DeviceRayTracingTlasInstance
        {
            uint32_t m_instanceID = 0;
            uint32_t m_hitGroupIndex = 0;
            AZ::Transform m_transform = AZ::Transform::CreateIdentity();
            AZ::Vector3 m_nonUniformScale = AZ::Vector3::CreateOne();
            bool m_transparent = false;
            RHI::Ptr<RHI::DeviceRayTracingBlas> m_blas;
        };
        using DeviceRayTracingTlasInstanceVector = AZStd::vector<DeviceRayTracingTlasInstance>;

        //! DeviceRayTracingTlasDescriptor
        //!
        //! The Build() operation in the descriptor allows the TLAS to be initialized
        //! using the following pattern:
        //!
        //! RHI::DeviceRayTracingTlasDescriptor descriptor;
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
        class DeviceRayTracingTlasDescriptor final
        {
        public:
            DeviceRayTracingTlasDescriptor() = default;
            ~DeviceRayTracingTlasDescriptor() = default;

            // accessors
            const DeviceRayTracingTlasInstanceVector& GetInstances() const
            {
                return m_instances;
            }
            DeviceRayTracingTlasInstanceVector& GetInstances()
            {
                return m_instances;
            }

            const RHI::Ptr<RHI::DeviceBuffer>& GetInstancesBuffer() const
            {
                return m_instancesBuffer;
            }
            RHI::Ptr<RHI::DeviceBuffer>& GetInstancesBuffer()
            {
                return m_instancesBuffer;
            }

            uint32_t GetNumInstancesInBuffer() const
            {
                return m_numInstancesInBuffer;
            }

            // build operations
            DeviceRayTracingTlasDescriptor* Build();
            DeviceRayTracingTlasDescriptor* Instance();
            DeviceRayTracingTlasDescriptor* InstanceID(uint32_t instanceID);
            DeviceRayTracingTlasDescriptor* HitGroupIndex(uint32_t hitGroupIndex);
            DeviceRayTracingTlasDescriptor* Transform(const AZ::Transform& transform);
            DeviceRayTracingTlasDescriptor* NonUniformScale(const AZ::Vector3& nonUniformScale);
            DeviceRayTracingTlasDescriptor* Transparent(bool transparent);
            DeviceRayTracingTlasDescriptor* Blas(const RHI::Ptr<RHI::DeviceRayTracingBlas>& blas);
            DeviceRayTracingTlasDescriptor* InstancesBuffer(const RHI::Ptr<RHI::DeviceBuffer>& tlasInstances);
            DeviceRayTracingTlasDescriptor* NumInstances(uint32_t numInstancesInBuffer);

        private:
            DeviceRayTracingTlasInstanceVector m_instances;
            DeviceRayTracingTlasInstance* m_buildContext = nullptr;

            // externally created Instances buffer, cannot be combined with other Instances
            RHI::Ptr<RHI::DeviceBuffer> m_instancesBuffer;
            uint32_t m_numInstancesInBuffer;
        };

        //! DeviceRayTracingTlas
        //!
        //! A DeviceRayTracingTlas is created from the information in the DeviceRayTracingTlasDescriptor.
        class DeviceRayTracingTlas : public DeviceObject
        {
        public:
            DeviceRayTracingTlas() = default;
            virtual ~DeviceRayTracingTlas() = default;

            static RHI::Ptr<RHI::DeviceRayTracingTlas> CreateRHIRayTracingTlas();

            //! Creates the internal TLAS buffers from the descriptor
            ResultCode CreateBuffers(
                Device& device, const DeviceRayTracingTlasDescriptor* descriptor, const DeviceRayTracingBufferPools& rayTracingBufferPools);

            //! Returns the TLAS RHI buffer
            virtual const RHI::Ptr<RHI::DeviceBuffer> GetTlasBuffer() const = 0;
            virtual const RHI::Ptr<RHI::DeviceBuffer> GetTlasInstancesBuffer() const = 0;

        private:
            // Platform API
            virtual RHI::ResultCode CreateBuffersInternal(
                RHI::Device& deviceBase,
                const RHI::DeviceRayTracingTlasDescriptor* descriptor,
                const DeviceRayTracingBufferPools& rayTracingBufferPools) = 0;
        };
    } // namespace RHI
} // namespace AZ
