/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/IndirectArguments.h>

namespace AZ::RHI
{
    struct DrawInstanceArguments
    {
        DrawInstanceArguments() = default;

        DrawInstanceArguments(
            uint32_t instanceCount,
            uint32_t instanceOffset)
            : m_instanceCount(instanceCount)
            , m_instanceOffset(instanceOffset)
        { }

        uint32_t m_instanceCount = 1;
        uint32_t m_instanceOffset = 0;
    };

    struct DrawLinear
    {
        DrawLinear() = default;

        DrawLinear(
            uint32_t vertexCount,
            uint32_t vertexOffset)
            : m_vertexCount(vertexCount)
            , m_vertexOffset(vertexOffset)
        {}

        uint32_t m_vertexCount = 0;
        uint32_t m_vertexOffset = 0;
    };

    struct DrawIndexed
    {
        DrawIndexed() = default;

        DrawIndexed(
            uint32_t vertexOffset,
            uint32_t indexCount,
            uint32_t indexOffset)
            : m_vertexOffset(vertexOffset)
            , m_indexCount(indexCount)
            , m_indexOffset(indexOffset)
        {}

        uint32_t m_vertexOffset = 0;
        uint32_t m_indexCount = 0;
        uint32_t m_indexOffset = 0;
    };

    //! Arguments for a hardware mesh-shader draw (DrawType::DispatchMesh). Carries only the
    //! thread-group grid dimensions -- the mesh/amplification pipeline pulls its own geometry,
    //! so there is no vertex/index/instance data. API-neutral: DX12 DispatchMesh, Vulkan
    //! vkCmdDrawMeshTasksEXT, Metal drawMeshThreadgroups (per-threadgroup thread counts come
    //! from PSO reflection, not from here).
    struct DispatchMeshDirect
    {
        DispatchMeshDirect() = default;

        DispatchMeshDirect(
            uint32_t groupCountX,
            uint32_t groupCountY,
            uint32_t groupCountZ)
            : m_groupCountX(groupCountX)
            , m_groupCountY(groupCountY)
            , m_groupCountZ(groupCountZ)
        {}

        uint32_t m_groupCountX = 1;
        uint32_t m_groupCountY = 1;
        uint32_t m_groupCountZ = 1;
    };

    using DeviceDrawIndirect = DeviceIndirectArguments;

    enum class DrawType : uint8_t
    {
        Indexed = 0,
        Linear,
        Indirect,
        DispatchMesh
    };

    struct DeviceDrawArguments
    {
        AZ_TYPE_INFO(DeviceDrawArguments, "B8127BDE-513E-4D5C-98C2-027BA1DE9E6E");

        DeviceDrawArguments() : DeviceDrawArguments(DrawIndexed{}) {}

        DeviceDrawArguments(const DrawIndexed& indexed)
            : m_type{ DrawType::Indexed }
            , m_indexed{ indexed }
        {}

        DeviceDrawArguments(const DrawLinear& linear)
            : m_type{ DrawType::Linear }
            , m_linear{ linear }
        {}


        DeviceDrawArguments(const DeviceDrawIndirect& indirect)
            : m_type{ DrawType::Indirect }
            , m_indirect{ indirect }
        {}

        DeviceDrawArguments(const DispatchMeshDirect& dispatchMesh)
            : m_type{ DrawType::DispatchMesh }
            , m_dispatchMesh{ dispatchMesh }
        {}

        DrawType m_type;
        union
        {
            DrawIndexed m_indexed;
            DrawLinear m_linear;
            DeviceDrawIndirect m_indirect;
            DispatchMeshDirect m_dispatchMesh;
        };
    };
}
