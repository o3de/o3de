/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/any.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/utils.h>

#include <Atom/RHI.Reflect/Base.h>
#include <AzCore/Name/Name.h>

namespace AZ::RHI
{
    //! The RHI uses 'virtual' shader stages that encapsulate some of the platform differences
    //! around tessellation. For example, Metal utilizes compute to manipulate control
    //! points and patch functions, and then feeds the results into a vertex function. Therefore,
    //! the specifics of tessellation are contained under a single virtual 'Tessellation' stage.
    //!
    //! Also, Geometry shaders are currently not supported. They are rife with performance problems
    //! on most platforms and the industry as a whole is moving away from them.
    enum class ShaderStage : uint32_t
    {
        //! A sentinel type used when a shader stage is not set properly. Used to track invalid state.
        Unknown = static_cast<uint32_t>(-1),

        //! This virtual stage contains shader stages that expand an input assembly stream and manipulate
        //! a vertex. On certain platforms like Metal, this stage may occur after tessellation.
        Vertex = 0,

        //! This virtual stage contains shader stages that expand an input assembly stream and manipulate
        //! a vertex Note: Not supported on metal.
        Geometry,

        //! This virtual stage contains the platform-specific stages necessary to process screen space fragments.
        //! Currently, on all supported platforms, this maps 1-to-1 with a hardware shader stage.
        Fragment,

        // This virtual stage represents compute shaders. The mechanics of compute is standard across all platforms that
        // support it, so this maps 1-to-1 with each hardware compute stage.
        Compute,

        // This virtual stage represents ray tracing shaders.  On DXIL platforms this is implemented with a DXIL Library.
        RayTracing,

        Count,
        GraphicsCount = Compute,
    };

    const uint32_t ShaderStageCount = static_cast<uint32_t>(ShaderStage::Count);
    const uint32_t ShaderStageVertex = static_cast<uint32_t>(ShaderStage::Vertex);
    const uint32_t ShaderStageFragment = static_cast<uint32_t>(ShaderStage::Fragment);
    const uint32_t ShaderStageGraphicsCount = static_cast<uint32_t>(ShaderStage::GraphicsCount);

    using ShaderStageAttributeArguments = AZStd::vector<AZStd::any>;
    using ShaderStageAttributeMap       = AZStd::unordered_map<Name /*attributeName*/, ShaderStageAttributeArguments>;
    using ShaderStageAttributeMapList   = AZStd::fixed_vector<RHI::ShaderStageAttributeMap, RHI::ShaderStageCount>;

    //! Describes shader stages as a mask, where each bit represents a shader stage type.
    enum class ShaderStageMask : uint32_t
    {
        None = 0,
        Vertex = AZ_BIT(static_cast<uint32_t>(ShaderStage::Vertex)),
        Geometry = AZ_BIT(static_cast<uint32_t>(ShaderStage::Geometry)),
        Fragment = AZ_BIT(static_cast<uint32_t>(ShaderStage::Fragment)),
        Compute = AZ_BIT(static_cast<uint32_t>(ShaderStage::Compute)),
        RayTracing = AZ_BIT(static_cast<uint32_t>(ShaderStage::RayTracing)),
        All = Vertex | Geometry | Fragment | Compute | RayTracing
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::ShaderStageMask)
}
