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
    struct DrawLinear
    {
        DrawLinear() = default;

        DrawLinear(
            uint32_t instanceCount,
            uint32_t instanceOffset,
            uint32_t vertexCount,
            uint32_t vertexOffset)
            : m_instanceCount(instanceCount)
            , m_instanceOffset(instanceOffset)
            , m_vertexCount(vertexCount)
            , m_vertexOffset(vertexOffset)
        {}

        uint32_t m_instanceCount = 1;
        uint32_t m_instanceOffset = 0;
        uint32_t m_vertexCount = 0;
        uint32_t m_vertexOffset = 0;
    };

    struct DrawIndexed
    {
        DrawIndexed() = default;

        DrawIndexed(
            uint32_t instanceCount,
            uint32_t instanceOffset,
            uint32_t vertexOffset,
            uint32_t indexCount,
            uint32_t indexOffset)
            : m_instanceCount(instanceCount)
            , m_instanceOffset(instanceOffset)
            , m_vertexOffset(vertexOffset)
            , m_indexCount(indexCount)
            , m_indexOffset(indexOffset)
        {}

        uint32_t m_instanceCount = 1;
        uint32_t m_instanceOffset = 0;
        uint32_t m_vertexOffset = 0;
        uint32_t m_indexCount = 0;
        uint32_t m_indexOffset = 0;
    };

    using DrawIndirect = IndirectArguments;

    enum class DrawType : uint8_t
    {
        Indexed = 0,
        Linear,
        Indirect
    };

    struct DrawArguments
    {
        AZ_TYPE_INFO(DrawArguments, "B8127BDE-513E-4D5C-98C2-027BA1DE9E6E");

        DrawArguments() : DrawArguments(DrawIndexed{}) {}

        DrawArguments(const DrawIndexed& indexed)
            : m_type{DrawType::Indexed}
            , m_indexed{indexed}
        {}

        DrawArguments(const DrawLinear& linear)
            : m_type{DrawType::Linear}
            , m_linear{linear}
        {}


        DrawArguments(const DrawIndirect& indirect)
            : m_type{ DrawType::Indirect }
            , m_indirect{ indirect }
        {}

        DrawType m_type;
        union
        {
            DrawIndexed m_indexed;
            DrawLinear m_linear;
            DrawIndirect m_indirect;
        };
    };
}
