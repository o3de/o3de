/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>
#include <Atom/RHI/DeviceIndexBufferView.h>
#include <Atom/RHI/DeviceStreamBufferView.h>
#include <Atom/RHI.Reflect/VertexFormat.h>
#include <Atom/RHI/DeviceObject.h>

namespace AZ::RHI
{
    class DeviceRayTracingBufferPools;

    //! RayTracingAccelerationStructureBuildFlags
    //!
    //! These build flags can be used to signal to the API what kind of Ray Tracing Acceleration Structure (RTAS) build it should prefer.
    //! For example, if skinned meshes are present in the scene, it might be best to enable a mode where the RTAS is
    //! periodically updated and not completely rebuilt every frame. These options can be set on both BLAS objects.
    //!
    //! FAST_TRACE: Sets a preference to build the RTAS to have faster raytracing capabilities. Can incur longer build times.
    //! FAST_BUILD: Sets a preference for faster build times of the Acceleration Structure over faster raytracing.
    //! ENABLE_UPDATE: Enables incremental updating of a BLAS object. Needs to be set at BLAS creation time.
    enum class RayTracingAccelerationStructureBuildFlags : uint32_t
    {
        FAST_TRACE = AZ_BIT(1),
        FAST_BUILD = AZ_BIT(2),
        ENABLE_UPDATE = AZ_BIT(3),
        ENABLE_COMPACTION = AZ_BIT(4),
    };
    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::RayTracingAccelerationStructureBuildFlags);

    /////////////////////////////////////////////////////////////////////////////////////////////
    // Bottom Level Acceleration Structure (BLAS)

    //! RayTracingAccelerationStructureInstanceInclusionMask
    //!
    //! The following flags are set by the MeshFeatureProcessor to allow for more fine-grained control over which geometry instances are included
    //! during ray intersection tests. We currently make two distinctions between static and dynamic (skinned) meshes.
    //!
    //! STATIC_MESH_INSTANCE: Default instance mask value and given to all static meshes
    //! SKINNED_MESH_INSTANCE: Instance mask value given to skinned meshes. Currently only used for skinned meshes
    //!
    //! For more information on instance occlusion masks visit: https://learn.microsoft.com/en-us/windows/win32/direct3d12/traceray-function
    enum class RayTracingAccelerationStructureInstanceInclusionMask : uint32_t
    {
        STATIC_MESH = AZ_BIT(0),
        SKINNED_MESH = AZ_BIT(1),
    };
    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::RayTracingAccelerationStructureInstanceInclusionMask);

    //! DeviceRayTracingGeometry
    //!
    //! The geometry entry contains the vertex and index buffers associated with geometry in the
    //! scene.  Each DeviceRayTracingBlas contains a list of these entries.
    struct DeviceRayTracingGeometry
    {
        RHI::VertexFormat m_vertexFormat = RHI::VertexFormat::Unknown;
        RHI::DeviceStreamBufferView m_vertexBuffer;
        RHI::DeviceIndexBufferView m_indexBuffer;
        // [GFX TODO][ATOM-4989] Add DXR BLAS Transform Buffer
    };
    using DeviceRayTracingGeometryVector = AZStd::vector<DeviceRayTracingGeometry>;

    //! DeviceRayTracingBlasDescriptor
    //!
    //! Describes a single-device ray tracing bottom-level acceleration structure.
    struct DeviceRayTracingBlasDescriptor final
    {
        DeviceRayTracingGeometryVector m_geometries;
        AZStd::optional<AZ::Aabb> m_aabb;
        DeviceRayTracingGeometry* m_buildContext = nullptr;
        RayTracingAccelerationStructureBuildFlags m_buildFlags = AZ::RHI::RayTracingAccelerationStructureBuildFlags::FAST_TRACE;
    };

    //! DeviceRayTracingBlas
    //!
    //! A DeviceRayTracingBlas is created from the information in the DeviceRayTracingBlasDescriptor.
    class DeviceRayTracingBlas
        : public DeviceObject
    {
    public:
        DeviceRayTracingBlas() = default;
        virtual ~DeviceRayTracingBlas() = default;

        static RHI::Ptr<RHI::DeviceRayTracingBlas> CreateRHIRayTracingBlas();

        //! Creates the internal BLAS buffers for the compacted version of the sourceBlas
        //! The compactedBufferSize can be queried using a RayTracingCompactionQuery
        ResultCode CreateCompactedBuffers(
            Device& device,
            RHI::Ptr<RHI::DeviceRayTracingBlas> sourceBlas,
            uint64_t compactedBufferSize,
            const DeviceRayTracingBufferPools& rayTracingBufferPools);

        //! Creates the internal BLAS buffers from the descriptor
        ResultCode CreateBuffers(Device& device, const DeviceRayTracingBlasDescriptor* descriptor, const DeviceRayTracingBufferPools& rayTracingBufferPools);

        //! Returns true if the DeviceRayTracingBlas has been initialized
        virtual bool IsValid() const = 0;

        DeviceRayTracingGeometryVector& GetGeometries()
        {
            return m_geometries;
        }

        virtual uint64_t GetAccelerationStructureByteSize() = 0;

    private:
        // Platform API
        virtual RHI::ResultCode CreateBuffersInternal(RHI::Device& deviceBase, const RHI::DeviceRayTracingBlasDescriptor* descriptor, const DeviceRayTracingBufferPools& rayTracingBufferPools) = 0;
        virtual RHI::ResultCode CreateCompactedBuffersInternal(
            Device& device,
            RHI::Ptr<RHI::DeviceRayTracingBlas> sourceBlas,
            uint64_t compactedBufferSize,
            const DeviceRayTracingBufferPools& rayTracingBufferPools) = 0;

        DeviceRayTracingGeometryVector m_geometries;
    };

    /////////////////////////////////////////////////////////////////////////////////////////////
    // Top Level Acceleration Structure (TLAS)

    //! DeviceRayTracingTlasInstance
    //!
    //! Each TLAS instance entry refers to a DeviceRayTracingBlas, and can contain a transform which
    //! will be applied to all of the geometry entries in the Blas.  It also contains a hitGroupIndex
    //! which is used to index into the DeviceRayTracingShaderTable to determine the hit shader when a
    //! ray hits any geometry in the instance.
    struct DeviceRayTracingTlasInstance
    {
        uint32_t m_instanceID = 0;
        uint32_t m_hitGroupIndex = 0;
        uint32_t m_instanceMask = 0x1; // Setting this to 1 to be backwards-compatible
        AZ::Transform m_transform = AZ::Transform::CreateIdentity();
        AZ::Vector3 m_nonUniformScale = AZ::Vector3::CreateOne();
        bool m_transparent = false;
        RHI::Ptr<RHI::DeviceRayTracingBlas> m_blas;
    };
    using DeviceRayTracingTlasInstanceVector = AZStd::vector<DeviceRayTracingTlasInstance>;

    //! DeviceRayTracingTlasDescriptor
    //!
    //! Describes a single-device ray tracing top-level acceleration structure.
    struct DeviceRayTracingTlasDescriptor
    {
        DeviceRayTracingTlasInstanceVector m_instances;

        // externally created Instances buffer, cannot be combined with other Instances
        RHI::Ptr<RHI::DeviceBuffer> m_instancesBuffer;
        uint32_t m_numInstancesInBuffer;
    };

    //! DeviceRayTracingTlas
    //!
    //! A DeviceRayTracingTlas is created from the information in the DeviceRayTracingTlasDescriptor.
    class DeviceRayTracingTlas
        : public DeviceObject
    {
    public:
        DeviceRayTracingTlas() = default;
        virtual ~DeviceRayTracingTlas() = default;

        static RHI::Ptr<RHI::DeviceRayTracingTlas> CreateRHIRayTracingTlas();

        //! Creates the internal TLAS buffers from the descriptor
        ResultCode CreateBuffers(Device& device, const DeviceRayTracingTlasDescriptor* descriptor, const DeviceRayTracingBufferPools& rayTracingBufferPools);

        //! Returns the TLAS RHI buffer
        virtual const RHI::Ptr<RHI::DeviceBuffer> GetTlasBuffer() const = 0;
        virtual const RHI::Ptr<RHI::DeviceBuffer> GetTlasInstancesBuffer() const = 0;

    private:
        // Platform API
        virtual RHI::ResultCode CreateBuffersInternal(RHI::Device& deviceBase, const RHI::DeviceRayTracingTlasDescriptor* descriptor, const DeviceRayTracingBufferPools& rayTracingBufferPools) = 0;
    };
}
