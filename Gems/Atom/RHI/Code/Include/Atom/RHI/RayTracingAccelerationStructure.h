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
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI/DeviceObject.h>

namespace AZ::RHI
{
    class RayTracingBufferPools;

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
        bool HasAABB() const { return m_aabb.has_value(); }
        const RayTracingGeometryVector& GetGeometries() const { return m_geometries; }
        const AZ::Aabb& GetAABB() const { return *m_aabb; }
        RayTracingGeometryVector& GetGeometries() { return m_geometries; }

        [[nodiscard]] const RayTracingAccelerationStructureBuildFlags& GetBuildFlags() const { return m_buildFlags; }

        // build operations
        RayTracingBlasDescriptor* Build();
        RayTracingBlasDescriptor* Geometry();
        RayTracingBlasDescriptor* AABB(const AZ::Aabb& aabb);
        RayTracingBlasDescriptor* VertexBuffer(const RHI::StreamBufferView& vertexBuffer);
        RayTracingBlasDescriptor* VertexFormat(RHI::Format vertexFormat);
        RayTracingBlasDescriptor* IndexBuffer(const RHI::IndexBufferView& indexBuffer);
        RayTracingBlasDescriptor* BuildFlags(const RHI::RayTracingAccelerationStructureBuildFlags &buildFlags);

    private:
        RayTracingGeometryVector m_geometries;
        AZStd::optional<AZ::Aabb> m_aabb;
        RayTracingGeometry* m_buildContext = nullptr;
        RayTracingAccelerationStructureBuildFlags m_buildFlags = AZ::RHI::RayTracingAccelerationStructureBuildFlags::FAST_TRACE;
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

        RayTracingGeometryVector& GetGeometries()
        {
            return m_geometries;
        }

    private:
        // Platform API
        virtual RHI::ResultCode CreateBuffersInternal(RHI::Device& deviceBase, const RHI::RayTracingBlasDescriptor* descriptor, const RayTracingBufferPools& rayTracingBufferPools) = 0;

        RayTracingGeometryVector m_geometries;
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
        RayTracingTlasDescriptor* InstanceMask(uint32_t instanceMask);
        RayTracingTlasDescriptor* HitGroupIndex(uint32_t hitGroupIndex);
        RayTracingTlasDescriptor* Transform(const AZ::Transform& transform);
        RayTracingTlasDescriptor* NonUniformScale(const AZ::Vector3& nonUniformScale);
        RayTracingTlasDescriptor* Transparent(bool transparent);
        RayTracingTlasDescriptor* Blas(const RHI::Ptr<RHI::RayTracingBlas>& blas);
        RayTracingTlasDescriptor* InstancesBuffer(const RHI::Ptr<RHI::Buffer>& tlasInstances);
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
