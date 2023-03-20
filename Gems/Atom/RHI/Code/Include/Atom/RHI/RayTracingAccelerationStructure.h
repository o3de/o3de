/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Transform.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI/DeviceObject.h>

namespace AZ
{
    namespace RHI
    {
        class RayTracingBufferPools;

        /////////////////////////////////////////////////////////////////////////////////////////////
        // Bottom Level Acceleration Structure (BLAS)

        //! RayTracingGeometry
        //!
        //! The geometry entry contains the vertex and index buffers associated with geometry in the
        //! scene.  Each RayTracingBlas contains a list of these entries.
        struct RayTracingGeometry
        {
            RHI::Format m_vertexFormat = RHI::Format::Unknown;
            RHI::StreamBufferView m_vertexBuffer;
            RHI::IndexBufferView m_indexBuffer;
            // [GFX TODO][ATOM-4989] Add DXR BLAS Transform Buffer
        };
        using RayTracingGeometryVector = AZStd::vector<RayTracingGeometry>;

        //! RayTracingBlasDescriptor
        //!
        //! The Build() operation in the descriptor allows the BLAS to be initialized
        //! using the following pattern:
        //!
        //! RHI::RayTracingBlasDescriptor descriptor;
        //! descriptor.Build()
        //!    ->Geometry()
        //!        ->VertexFormat(RHI::Format::R32G32B32_FLOAT)
        //!        ->VertexBuffer(vertexBufferView)
        //!        ->IndexBuffer(indexBufferView)
        //!    ;
        class RayTracingBlasDescriptor final
        {
        public:
            RayTracingBlasDescriptor() = default;
            ~RayTracingBlasDescriptor() = default;

            // accessors
            const RayTracingGeometryVector& GetGeometries() const { return m_geometries; }
            RayTracingGeometryVector& GetGeometries() { return m_geometries; }

            // build operations
            RayTracingBlasDescriptor* Build();
            RayTracingBlasDescriptor* Geometry();
            RayTracingBlasDescriptor* VertexBuffer(const RHI::StreamBufferView& vertexBuffer);
            RayTracingBlasDescriptor* VertexFormat(RHI::Format vertexFormat);
            RayTracingBlasDescriptor* IndexBuffer(const RHI::IndexBufferView& indexBuffer);

        private:
            RayTracingGeometryVector m_geometries;
            RayTracingGeometry* m_buildContext = nullptr;
        };

        //! RayTracingBlas
        //!
        //! A RayTracingBlas is created from the information in the RayTracingBlasDescriptor.
        class RayTracingBlas
            : public DeviceObject
        {
        public:
            RayTracingBlas() = default;
            virtual ~RayTracingBlas() = default;

            static RHI::Ptr<RHI::RayTracingBlas> CreateRHIRayTracingBlas();

            //! Creates the internal BLAS buffers from the descriptor
            ResultCode CreateBuffers(Device& device, const RayTracingBlasDescriptor* descriptor, const RayTracingBufferPools& rayTracingBufferPools);

            //! Returns true if the RayTracingBlas has been initialized
            virtual bool IsValid() const = 0;

        private:
            // Platform API
            virtual RHI::ResultCode CreateBuffersInternal(RHI::Device& deviceBase, const RHI::RayTracingBlasDescriptor* descriptor, const RayTracingBufferPools& rayTracingBufferPools) = 0;
        };

        /////////////////////////////////////////////////////////////////////////////////////////////
        // Top Level Acceleration Structure (TLAS)

        //! RayTracingTlasInstance
        //!
        //! Each TLAS instance entry refers to a RayTracingBlas, and can contain a transform which
        //! will be applied to all of the geometry entries in the Blas.  It also contains a hitGroupIndex
        //! which is used to index into the RayTracingShaderTable to determine the hit shader when a
        //! ray hits any geometry in the instance.
        struct RayTracingTlasInstance
        {
            uint32_t m_instanceID = 0;
            uint32_t m_hitGroupIndex = 0;
            AZ::Transform m_transform = AZ::Transform::CreateIdentity();
            AZ::Vector3 m_nonUniformScale = AZ::Vector3::CreateOne();
            bool m_transparent = false;
            RHI::Ptr<RHI::RayTracingBlas> m_blas;
        };
        using RayTracingTlasInstanceVector = AZStd::vector<RayTracingTlasInstance>;
        
        //! RayTracingTlasDescriptor
        //!
        //! The Build() operation in the descriptor allows the TLAS to be initialized
        //! using the following pattern:
        //!
        //! RHI::RayTracingTlasDescriptor descriptor;
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
        class RayTracingTlasDescriptor final
        {
        public:
            RayTracingTlasDescriptor() = default;
            ~RayTracingTlasDescriptor() = default;
        
            // accessors
            const RayTracingTlasInstanceVector& GetInstances() const { return m_instances; }
            RayTracingTlasInstanceVector& GetInstances() { return m_instances; }

            const RHI::Ptr<RHI::Buffer>& GetInstancesBuffer() const { return  m_instancesBuffer; }
            RHI::Ptr<RHI::Buffer>& GetInstancesBuffer() { return  m_instancesBuffer; }

            uint32_t GetNumInstancesInBuffer() const { return m_numInstancesInBuffer; }

            // build operations
            RayTracingTlasDescriptor* Build();
            RayTracingTlasDescriptor* Instance();
            RayTracingTlasDescriptor* InstanceID(uint32_t instanceID);
            RayTracingTlasDescriptor* HitGroupIndex(uint32_t hitGroupIndex);
            RayTracingTlasDescriptor* Transform(const AZ::Transform& transform);
            RayTracingTlasDescriptor* NonUniformScale(const AZ::Vector3& nonUniformScale);
            RayTracingTlasDescriptor* Transparent(bool transparent);
            RayTracingTlasDescriptor* Blas(RHI::Ptr<RHI::RayTracingBlas>& blas);
            RayTracingTlasDescriptor* InstancesBuffer(RHI::Ptr<RHI::Buffer>& tlasInstances);
            RayTracingTlasDescriptor* NumInstances(uint32_t numInstancesInBuffer);

        private:
            RayTracingTlasInstanceVector m_instances;
            RayTracingTlasInstance* m_buildContext = nullptr;

            // externally created Instances buffer, cannot be combined with other Instances
            RHI::Ptr<RHI::Buffer> m_instancesBuffer;
            uint32_t m_numInstancesInBuffer;
        };

        //! RayTracingTlas
        //!
        //! A RayTracingTlas is created from the information in the RayTracingTlasDescriptor.
        class RayTracingTlas
            : public DeviceObject
        {
        public:
            RayTracingTlas() = default;
            virtual ~RayTracingTlas() = default;

            static RHI::Ptr<RHI::RayTracingTlas> CreateRHIRayTracingTlas();

            //! Creates the internal TLAS buffers from the descriptor
            ResultCode CreateBuffers(Device& device, const RayTracingTlasDescriptor* descriptor, const RayTracingBufferPools& rayTracingBufferPools);

            //! Returns the TLAS RHI buffer
            virtual const RHI::Ptr<RHI::Buffer> GetTlasBuffer() const = 0;
            virtual const RHI::Ptr<RHI::Buffer> GetTlasInstancesBuffer() const = 0;

        private:
            // Platform API
            virtual RHI::ResultCode CreateBuffersInternal(RHI::Device& deviceBase, const RHI::RayTracingTlasDescriptor* descriptor, const RayTracingBufferPools& rayTracingBufferPools) = 0;
        };
    }
}
