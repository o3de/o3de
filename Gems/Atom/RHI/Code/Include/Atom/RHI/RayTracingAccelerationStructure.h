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
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RHI/DeviceRayTracingAccelerationStructure.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::RHI
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

        //! Returns the device-specific DeviceRayTracingBlasDescriptor for the given index
        DeviceRayTracingBlasDescriptor GetDeviceRayTracingBlasDescriptor(int deviceIndex) const;

        //! Accessors
        const RayTracingGeometryVector& GetGeometries() const
        {
            return m_geometries;
        }
        RayTracingGeometryVector& GetGeometries()
        {
            return m_geometries;
        }

        [[nodiscard]] const RayTracingAccelerationStructureBuildFlags& GetBuildFlags() const
        {
            return m_buildFlags;
        }

        //! Build operations
        RayTracingBlasDescriptor* Build();
        RayTracingBlasDescriptor* Geometry();
        RayTracingBlasDescriptor* AABB(const AZ::Aabb& aabb);
        RayTracingBlasDescriptor* VertexBuffer(const RHI::StreamBufferView& vertexBuffer);
        RayTracingBlasDescriptor* VertexFormat(RHI::Format vertexFormat);
        RayTracingBlasDescriptor* IndexBuffer(const RHI::IndexBufferView& indexBuffer);
        RayTracingBlasDescriptor* BuildFlags(const RHI::RayTracingAccelerationStructureBuildFlags& buildFlags);

    private:
        RayTracingGeometryVector m_geometries;
        AZStd::optional<AZ::Aabb> m_aabb;
        RayTracingGeometry* m_buildContext = nullptr;
        RayTracingAccelerationStructureBuildFlags m_buildFlags = AZ::RHI::RayTracingAccelerationStructureBuildFlags::FAST_TRACE;
    };

    //! RayTracingBlas
    //!
    //! A RayTracingBlas is created from the information in the RayTracingBlasDescriptor.
    class RayTracingBlas : public MultiDeviceObject
    {
    public:
        AZ_CLASS_ALLOCATOR(RayTracingBlas, AZ::SystemAllocator, 0);
        AZ_RTTI(RayTracingBlas, "{D17E050F-ECC2-4C20-A073-F43008F2D168}", MultiDeviceObject);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(RayTracingBlas);
        RayTracingBlas() = default;
        virtual ~RayTracingBlas() = default;

        //! Creates the internal BLAS buffers from the descriptor
        ResultCode CreateBuffers(
            MultiDevice::DeviceMask deviceMask,
            const RayTracingBlasDescriptor* descriptor,
            const RayTracingBufferPools& rayTracingBufferPools);

        //! Creates the internal BLAS buffers for the compacted version of the sourceBlas
        //! The compactedBufferSizes can be queried using a RayTracingCompactionQuery
        ResultCode CreateCompactedBuffers(
            const RayTracingBlas& sourceBlas,
            const AZStd::unordered_map<int, uint64_t>& compactedSizes,
            const RayTracingBufferPools& rayTracingBufferPools);

        //! Returns true if the RayTracingBlas has been initialized
        bool IsValid() const;

    private:
        RayTracingBlasDescriptor m_descriptor;
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
        uint32_t m_instanceMask = 0x1; // Setting this to 1 to be backwards-compatible
        AZ::Transform m_transform = AZ::Transform::CreateIdentity();
        AZ::Vector3 m_nonUniformScale = AZ::Vector3::CreateOne();
        bool m_transparent = false;
        RHI::Ptr<RHI::RayTracingBlas> m_blas;
    };
    using MultiDeviceRayTracingTlasInstanceVector = AZStd::vector<RayTracingTlasInstance>;

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

        //! Returns the device-specific DeviceRayTracingTlasDescriptor for the given index
        DeviceRayTracingTlasDescriptor GetDeviceRayTracingTlasDescriptor(int deviceIndex) const;

        //! Accessors
        const MultiDeviceRayTracingTlasInstanceVector& GetInstances() const
        {
            return m_instances;
        }
        MultiDeviceRayTracingTlasInstanceVector& GetInstances()
        {
            return m_instances;
        }

        const RHI::Ptr<RHI::Buffer>& GetInstancesBuffer() const
        {
            return m_instancesBuffer;
        }
        RHI::Ptr<RHI::Buffer>& GetInstancesBuffer()
        {
            return m_instancesBuffer;
        }

        uint32_t GetNumInstancesInBuffer() const
        {
            return m_numInstancesInBuffer;
        }

        //! Build operations
        RayTracingTlasDescriptor* Build();
        RayTracingTlasDescriptor* Instance();
        RayTracingTlasDescriptor* InstanceID(uint32_t instanceID);
        RayTracingTlasDescriptor* InstanceMask(uint32_t instanceMask);
        RayTracingTlasDescriptor* HitGroupIndex(uint32_t hitGroupIndex);
        RayTracingTlasDescriptor* Transform(const AZ::Transform& transform);
        RayTracingTlasDescriptor* NonUniformScale(const AZ::Vector3& nonUniformScale);
        RayTracingTlasDescriptor* Transparent(bool transparent);
        RayTracingTlasDescriptor* Blas(const RHI::Ptr<RayTracingBlas> &blas);
        RayTracingTlasDescriptor* InstancesBuffer(RHI::Ptr<RHI::Buffer>& tlasInstances);
        RayTracingTlasDescriptor* NumInstances(uint32_t numInstancesInBuffer);

    private:
        MultiDeviceRayTracingTlasInstanceVector m_instances;
        RayTracingTlasInstance* m_buildContext = nullptr;

        //! externally created Instances buffer, cannot be combined with other Instances
        RHI::Ptr<RHI::Buffer> m_instancesBuffer;
        uint32_t m_numInstancesInBuffer = 0;
    };

    //! RayTracingTlas
    //!
    //! A RayTracingTlas is created from the information in the RayTracingTlasDescriptor.
    class RayTracingTlas : public MultiDeviceObject
    {
    public:
        AZ_CLASS_ALLOCATOR(RayTracingTlas, AZ::SystemAllocator, 0);
        AZ_RTTI(RayTracingTlas, "{A2B0F8F1-D0B5-4D90-8AFA-CEF543D20E34}", MultiDeviceObject);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(RayTracingTlas);
        RayTracingTlas() = default;
        virtual ~RayTracingTlas() = default;

        //! Creates the internal TLAS buffers from the descriptor
        ResultCode CreateBuffers(
            MultiDevice::DeviceMask deviceMask,
            const RayTracingTlasDescriptor* descriptor,
            const RayTracingBufferPools& rayTracingBufferPools);

        //! Returns the TLAS RHI buffer
        const RHI::Ptr<RHI::Buffer> GetTlasBuffer() const;
        const RHI::Ptr<RHI::Buffer> GetTlasInstancesBuffer() const;

    private:
        //! Safe-guard access to creation of buffers cache during parallel access
        mutable AZStd::mutex m_tlasBufferMutex;
        mutable AZStd::mutex m_tlasInstancesBufferMutex;
        RayTracingTlasDescriptor m_descriptor;
        mutable RHI::Ptr<RHI::Buffer> m_tlasBuffer;
        mutable RHI::Ptr<RHI::Buffer> m_tlasInstancesBuffer;
    };
} // namespace AZ::RHI
