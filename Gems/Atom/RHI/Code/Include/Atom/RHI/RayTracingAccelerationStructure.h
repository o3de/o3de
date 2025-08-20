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
        RHI::VertexFormat m_vertexFormat = RHI::VertexFormat::Unknown;
        RHI::StreamBufferView m_vertexBuffer;
        RHI::IndexBufferView m_indexBuffer;
    };
    using RayTracingGeometryVector = AZStd::vector<RayTracingGeometry>;

    //! RayTracingBlasDescriptor
    //!
    //! Describes a ray tracing bottom-level acceleration structure.
    class ATOM_RHI_PUBLIC_API RayTracingBlasDescriptor final
    {
    public:
        //! Returns the device-specific DeviceRayTracingBlasDescriptor for the given index
        DeviceRayTracingBlasDescriptor GetDeviceRayTracingBlasDescriptor(int deviceIndex) const;

        RayTracingGeometryVector m_geometries;
        AZStd::optional<AZ::Aabb> m_aabb;
        RayTracingAccelerationStructureBuildFlags m_buildFlags = AZ::RHI::RayTracingAccelerationStructureBuildFlags::FAST_TRACE;
    };

    //! RayTracingBlas
    //!
    //! A RayTracingBlas is created from the information in the RayTracingBlasDescriptor.
    class ATOM_RHI_PUBLIC_API RayTracingBlas : public MultiDeviceObject
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
            MultiDevice::DeviceMask deviceMask,
            const RayTracingBlas& sourceBlas,
            const AZStd::unordered_map<int, uint64_t>& compactedSizes,
            const RayTracingBufferPools& rayTracingBufferPools);

        ResultCode AddDevice(int deviceIndex, const RayTracingBufferPools& rayTracingBufferPools);
        ResultCode AddDeviceCompacted(
            int deviceIndex, const RayTracingBlas& sourceBlas, uint64_t compactedSize, const RayTracingBufferPools& rayTracingBufferPools);
        void RemoveDevice(int deviceIndex);

        //! Returns true if the RayTracingBlas has been initialized
        bool IsValid() const;

    private:
        RayTracingBlasDescriptor m_descriptor;
    };


    /////////////////////////////////////////////////////////////////////////////////////////////
    // Cluster Bottom Level Acceleration Structure (Cluster BLAS)

    //! RayTracingClusterBlasDescriptor
    //!
    //! Describes a ray tracing cluster bottom-level acceleration structure.
    struct RayTracingClusterBlasDescriptor final
    {
        //! Returns the device-specific DeviceRayTracingBlasDescriptor for the given index
        DeviceRayTracingClusterBlasDescriptor GetDeviceRayTracingClusterBlasDescriptor(int deviceIndex) const;

        RHI::VertexFormat m_vertexFormat;
        uint32_t m_maxGeometryIndexValue;
        uint32_t m_maxClusterUniqueGeometryCount;
        uint32_t m_maxClusterTriangleCount;
        uint32_t m_maxClusterVertexCount;
        uint32_t m_maxTotalTriangleCount;
        uint32_t m_maxTotalVertexCount;
        uint32_t m_minPositionTruncateBitCount;
        uint32_t m_maxClusterCount;
        RayTracingAccelerationStructureBuildFlags m_buildFlags = AZ::RHI::RayTracingAccelerationStructureBuildFlags::FAST_TRACE;
        RHI::Ptr<RHI::BufferView> m_srcInfosArrayBufferView;
        RHI::Ptr<RHI::BufferView> m_srcInfosCountBufferView;
    };

    //! RayTracingClusterBlas
    //!
    //! A RayTracingClusterBlas is created from the information in the RayTracingClusterBlasDescriptor.
    class ATOM_RHI_PUBLIC_API RayTracingClusterBlas
        : public MultiDeviceObject
    {
    public:
        AZ_CLASS_ALLOCATOR(RayTracingClusterBlas, AZ::SystemAllocator, 0);
        AZ_RTTI(RayTracingClusterBlas, "{1FFD3320-3EFF-4655-B648-BF268843BF1C}", MultiDeviceObject);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(RayTracingClusterBlas);
        RayTracingClusterBlas() = default;
        virtual ~RayTracingClusterBlas() = default;

        //! Creates the internal Cluster BLAS buffers from the descriptor
        ResultCode CreateBuffers(MultiDevice::DeviceMask deviceMask, const RHI::RayTracingClusterBlasDescriptor* descriptor, const RayTracingBufferPools& rayTracingBufferPools);

        ResultCode AddDevice(int deviceIndex, const RayTracingBufferPools& rayTracingBufferPools);

    private:
        RayTracingClusterBlasDescriptor m_descriptor;
    };


    /////////////////////////////////////////////////////////////////////////////////////////////
    // Top Level Acceleration Structure (TLAS)

    //! RayTracingTlas
    //!
    //! A RayTracingTlas is created from the information in the RayTracingTlasDescriptor.
    class ATOM_RHI_PUBLIC_API RayTracingTlas : public MultiDeviceObject
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
            const AZStd::unordered_map<int, DeviceRayTracingTlasDescriptor>& descriptor,
            const RayTracingBufferPools& rayTracingBufferPools);

        //! Returns the TLAS RHI buffer
        const RHI::Ptr<RHI::Buffer> GetTlasBuffer() const;
        const RHI::Ptr<RHI::Buffer> GetTlasInstancesBuffer() const;

    private:
        //! Safe-guard access to creation of buffers cache during parallel access
        mutable AZStd::mutex m_tlasBufferMutex;
        mutable AZStd::mutex m_tlasInstancesBufferMutex;
        mutable RHI::Ptr<RHI::Buffer> m_tlasBuffer;
        mutable RHI::Ptr<RHI::Buffer> m_tlasInstancesBuffer;
    };
} // namespace AZ::RHI
