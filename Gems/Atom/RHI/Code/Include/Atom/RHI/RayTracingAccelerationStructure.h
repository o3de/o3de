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
    //! Describes a ray tracing bottom-level acceleration structure.
    class RayTracingBlasDescriptor final
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
    // Top Level Acceleration Structure (TLAS)

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
