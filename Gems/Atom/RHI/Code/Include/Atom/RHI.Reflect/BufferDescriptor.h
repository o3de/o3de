/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>

#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    //! A set of combinable flags which inform the system how a buffer is to be
    //! bound to the pipeline at all stages of its lifetime.
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(BufferBindFlags, uint32_t,
        (None            , 0),

        /// Supports input assembly access through a IndexBufferView or StreamBufferView. This flag is for buffers that are not updated often
        (InputAssembly   , AZ_BIT(0)),
            
        /// Supports input assembly access through a IndexBufferView or StreamBufferView. This flag is for buffers that are updated frequently
        (DynamicInputAssembly , AZ_BIT(1)),
            
        /// Supports constant access through a ShaderResourceGroup.
        (Constant        , AZ_BIT(2)),

        /// Supports read access through a ShaderResourceGroup.
        (ShaderRead      , AZ_BIT(3)),

        /// Supports write access through ShaderResourceGroup.
        (ShaderWrite     , AZ_BIT(4)),

        /// Supports read-write access through a ShaderResourceGroup.
        (ShaderReadWrite , ShaderRead | ShaderWrite),

        /// Supports read access for GPU copy operations.
        (CopyRead        , AZ_BIT(5)),

        /// Supports write access for GPU copy operations.
        (CopyWrite       , AZ_BIT(6)),

        /// Supports predication access for conditional rendering.
        (Predication     , AZ_BIT(7)),

        /// Supports indirect buffer access for indirect draw/dispatch.
        (Indirect        , AZ_BIT(8)),

        /// Supports ray tracing acceleration structure usage.
        (RayTracingAccelerationStructure , AZ_BIT(9)),

        /// Supports ray tracing shader table usage.
        (RayTracingShaderTable , AZ_BIT(10)),

        /// Supports ray tracing scratch buffer usage.
        (RayTracingScratchBuffer, AZ_BIT(11)));


    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::BufferBindFlags);

    BufferBindFlags GetBufferBindFlags(ScopeAttachmentUsage usage, ScopeAttachmentAccess access);

    //! A buffer corresponds to a region of linear memory and used for rendering operations.
    //! Its lifecycle is managed by buffer pools.
    struct BufferDescriptor
    {
        AZ_TYPE_INFO(BufferDescriptor, "{05321516-CDE4-451D-80A2-3D179AB3DB5D}");

        BufferDescriptor() = default;
        BufferDescriptor(BufferBindFlags bindFlags, size_t byteCount);

        static void Reflect(AZ::ReflectContext* context);

        AZ::HashValue64 GetHash(AZ::HashValue64 seed = AZ::HashValue64{ 0 }) const;

        /// Number of bytes in the buffer. Does not need to adhere to alignment requirements
        /// by the hardware (that is done for you internally). This type can't be size_t since it's
        /// reflected data and size_t is a different size depending on the platform.
        AZ::u64 m_byteCount = 0;

        /// [GFX TODO] we need to reconsider where is the best place to propagate a buffer alignment to backend
        /// Provide the higher level code to provide a desired alignment for backend allocators to use when
        /// allocate memory for this buffer.
        /// For example, this value need to be set as its buffer view descriptor's m_elementSize  
        /// for dx12 to create SRV or UAV properly.
        /// If it set to 0, the default alignment defined internally will be used.
        AZ::u64 m_alignment = 0;

        /// Union of all bind points for this buffer.
        BufferBindFlags m_bindFlags = BufferBindFlags::None;

        /// The mask of queue classes supporting shared access of this resource.
        HardwareQueueClassMask m_sharedQueueMask = HardwareQueueClassMask::All;
    };

    // Bind enums with uuids. Required for named enum support.
    // Note: AZ_TYPE_INFO_SPECIALIZE has to be declared in AZ namespace
    AZ_TYPE_INFO_SPECIALIZE(BufferBindFlags, "{BC151340-958F-4EDA-802F-2F34566D4329}");
}
