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
#include <Atom/RHI/SingleDeviceRayTracingAccelerationStructure.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::RHI
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
        RHI::MultiDeviceStreamBufferView m_mdVertexBuffer;
        RHI::MultiDeviceIndexBufferView m_mdIndexBuffer;
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

        //! Returns the device-specific SingleDeviceRayTracingBlasDescriptor for the given index
        SingleDeviceRayTracingBlasDescriptor GetDeviceRayTracingBlasDescriptor(int deviceIndex) const;

        //! Accessors
        const MultiDeviceRayTracingGeometryVector& GetGeometries() const
        {
            return m_mdGeometries;
        }
        MultiDeviceRayTracingGeometryVector& GetGeometries()
        {
            return m_mdGeometries;
        }

        [[nodiscard]] const RayTracingAccelerationStructureBuildFlags& GetBuildFlags() const
        {
            return m_mdBuildFlags;
        }

        //! Build operations
        MultiDeviceRayTracingBlasDescriptor* Build();
        MultiDeviceRayTracingBlasDescriptor* Geometry();
        MultiDeviceRayTracingBlasDescriptor* AABB(const AZ::Aabb& aabb);
        MultiDeviceRayTracingBlasDescriptor* VertexBuffer(const RHI::MultiDeviceStreamBufferView& vertexBuffer);
        MultiDeviceRayTracingBlasDescriptor* VertexFormat(RHI::Format vertexFormat);
        MultiDeviceRayTracingBlasDescriptor* IndexBuffer(const RHI::MultiDeviceIndexBufferView& indexBuffer);
        MultiDeviceRayTracingBlasDescriptor* BuildFlags(const RHI::RayTracingAccelerationStructureBuildFlags& buildFlags);

    private:
        MultiDeviceRayTracingGeometryVector m_mdGeometries;
        AZStd::optional<AZ::Aabb> m_aabb;
        MultiDeviceRayTracingGeometry* m_mdBuildContext = nullptr;
        RayTracingAccelerationStructureBuildFlags m_mdBuildFlags = AZ::RHI::RayTracingAccelerationStructureBuildFlags::FAST_TRACE;
    };

    //! MultiDeviceRayTracingBlas
    //!
    //! A MultiDeviceRayTracingBlas is created from the information in the MultiDeviceRayTracingBlasDescriptor.
    class MultiDeviceRayTracingBlas : public MultiDeviceObject
    {
    public:
        AZ_CLASS_ALLOCATOR(MultiDeviceRayTracingBlas, AZ::SystemAllocator, 0);
        AZ_RTTI(MultiDeviceRayTracingBlas, "{D17E050F-ECC2-4C20-A073-F43008F2D168}", MultiDeviceObject);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(RayTracingBlas);
        MultiDeviceRayTracingBlas() = default;
        virtual ~MultiDeviceRayTracingBlas() = default;

        //! Creates the internal BLAS buffers from the descriptor
        ResultCode CreateBuffers(
            MultiDevice::DeviceMask deviceMask,
            const MultiDeviceRayTracingBlasDescriptor* descriptor,
            const MultiDeviceRayTracingBufferPools& rayTracingBufferPools);

        //! Returns true if the MultiDeviceRayTracingBlas has been initialized
        bool IsValid() const;

    private:
        MultiDeviceRayTracingBlasDescriptor m_mdDescriptor;
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
        uint32_t m_instanceMask = 0x1; // Setting this to 1 to be backwards-compatible
        AZ::Transform m_transform = AZ::Transform::CreateIdentity();
        AZ::Vector3 m_nonUniformScale = AZ::Vector3::CreateOne();
        bool m_transparent = false;
        RHI::Ptr<RHI::MultiDeviceRayTracingBlas> m_mdBlas;
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

        //! Returns the device-specific SingleDeviceRayTracingTlasDescriptor for the given index
        SingleDeviceRayTracingTlasDescriptor GetDeviceRayTracingTlasDescriptor(int deviceIndex) const;

        //! Accessors
        const MultiDeviceRayTracingTlasInstanceVector& GetInstances() const
        {
            return m_mdInstances;
        }
        MultiDeviceRayTracingTlasInstanceVector& GetInstances()
        {
            return m_mdInstances;
        }

        const RHI::Ptr<RHI::MultiDeviceBuffer>& GetInstancesBuffer() const
        {
            return m_mdInstancesBuffer;
        }
        RHI::Ptr<RHI::MultiDeviceBuffer>& GetInstancesBuffer()
        {
            return m_mdInstancesBuffer;
        }

        uint32_t GetNumInstancesInBuffer() const
        {
            return m_numInstancesInBuffer;
        }

        //! Build operations
        MultiDeviceRayTracingTlasDescriptor* Build();
        MultiDeviceRayTracingTlasDescriptor* Instance();
        MultiDeviceRayTracingTlasDescriptor* InstanceID(uint32_t instanceID);
        MultiDeviceRayTracingTlasDescriptor* InstanceMask(uint32_t instanceMask);
        MultiDeviceRayTracingTlasDescriptor* HitGroupIndex(uint32_t hitGroupIndex);
        MultiDeviceRayTracingTlasDescriptor* Transform(const AZ::Transform& transform);
        MultiDeviceRayTracingTlasDescriptor* NonUniformScale(const AZ::Vector3& nonUniformScale);
        MultiDeviceRayTracingTlasDescriptor* Transparent(bool transparent);
        MultiDeviceRayTracingTlasDescriptor* Blas(const RHI::Ptr<MultiDeviceRayTracingBlas> &blas);
        MultiDeviceRayTracingTlasDescriptor* InstancesBuffer(RHI::Ptr<RHI::MultiDeviceBuffer>& tlasInstances);
        MultiDeviceRayTracingTlasDescriptor* NumInstances(uint32_t numInstancesInBuffer);

    private:
        MultiDeviceRayTracingTlasInstanceVector m_mdInstances;
        MultiDeviceRayTracingTlasInstance* m_mdBuildContext = nullptr;

        //! externally created Instances buffer, cannot be combined with other Instances
        RHI::Ptr<RHI::MultiDeviceBuffer> m_mdInstancesBuffer;
        uint32_t m_numInstancesInBuffer = 0;
    };

    //! MultiDeviceRayTracingTlas
    //!
    //! A MultiDeviceRayTracingTlas is created from the information in the MultiDeviceRayTracingTlasDescriptor.
    class MultiDeviceRayTracingTlas : public MultiDeviceObject
    {
    public:
        AZ_CLASS_ALLOCATOR(MultiDeviceRayTracingTlas, AZ::SystemAllocator, 0);
        AZ_RTTI(MultiDeviceRayTracingTlas, "{A2B0F8F1-D0B5-4D90-8AFA-CEF543D20E34}", MultiDeviceObject);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(RayTracingTlas);
        MultiDeviceRayTracingTlas() = default;
        virtual ~MultiDeviceRayTracingTlas() = default;

        //! Creates the internal TLAS buffers from the descriptor
        ResultCode CreateBuffers(
            MultiDevice::DeviceMask deviceMask,
            const MultiDeviceRayTracingTlasDescriptor* descriptor,
            const MultiDeviceRayTracingBufferPools& rayTracingBufferPools);

        //! Returns the TLAS RHI buffer
        const RHI::Ptr<RHI::MultiDeviceBuffer> GetTlasBuffer() const;
        const RHI::Ptr<RHI::MultiDeviceBuffer> GetTlasInstancesBuffer() const;

    private:
        //! Safe-guard access to creation of buffers cache during parallel access
        mutable AZStd::mutex m_tlasBufferMutex;
        mutable AZStd::mutex m_tlasInstancesBufferMutex;
        MultiDeviceRayTracingTlasDescriptor m_mdDescriptor;
        mutable RHI::Ptr<RHI::MultiDeviceBuffer> m_tlasBuffer;
        mutable RHI::Ptr<RHI::MultiDeviceBuffer> m_tlasInstancesBuffer;
    };
} // namespace AZ::RHI
