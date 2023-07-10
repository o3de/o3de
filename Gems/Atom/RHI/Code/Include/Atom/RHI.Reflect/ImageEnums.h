/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>

#include <AzCore/Preprocessor/Enum.h>

namespace AZ::RHI
{
    //! A set of combinable flags which inform the system how an image is to be
    //! bound to the pipeline at all stages of its lifetime.
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(ImageBindFlags, uint32_t,
        (None, 0),

        /// Supports read access through a ShaderResourceGroup.
        (ShaderRead, AZ_BIT(0)),

        /// Supports write access through a ShaderResourceGroup.
        (ShaderWrite, AZ_BIT(1)),

        /// Supports read-write access through a ShaderResourceGroup.
        (ShaderReadWrite, ShaderRead | ShaderWrite),

        /// Supports use as a color attachment on a scope.
        (Color, AZ_BIT(2)),

        /// Supports use as depth attachment on a scope.
        (Depth, AZ_BIT(3)),

        /// Supports use as stencil attachment on a scope.
        (Stencil, AZ_BIT(4)),

        /// Supports use as a depth stencil attachment on a scope.
        (DepthStencil, Depth | Stencil),

        /// Supports read access for GPU copy operations.
        (CopyRead, AZ_BIT(5)),

        /// Supports write access for GPU copy operations.
        (CopyWrite, AZ_BIT(6)),

        /// Supports use as a shading rate image on a scope.
        (ShadingRate, AZ_BIT(7)));

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::ImageBindFlags);

    enum class ImageDimension
    {
        Image1D = 1,
        Image2D = 2,
        Image3D = 3,
    };

    //! Aspects (or Planes in some graphic APIs) that an image can contain.
    enum class ImageAspect : uint32_t
    {
        Color = 0,  //< Represents the color aspect of an image
        Depth,      //< Represents the depth aspect of an image
        Stencil,    //< Represents the stencil aspect of an image
        Count
    };

    //! Number of Image Aspect supported.
    static const uint32_t ImageAspectCount = static_cast<uint32_t>(ImageAspect::Count);

    //! Mask of Aspects (or Planes in some graphic APIs) that an image can contain.
    enum class ImageAspectFlags : uint32_t
    {
        None = 0,
        Color = AZ_BIT(static_cast<uint32_t>(ImageAspect::Color)),
        Depth = AZ_BIT(static_cast<uint32_t>(ImageAspect::Depth)),
        Stencil = AZ_BIT(static_cast<uint32_t>(ImageAspect::Stencil)),
        DepthStencil = Depth | Stencil,
        All = ~uint32_t(0)
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::ImageAspectFlags);

    // Bind enums with uuids. Required for named enum support.
    // Note: AZ_TYPE_INFO_SPECIALIZE has to be declared in AZ namespace
    AZ_TYPE_INFO_SPECIALIZE(ImageBindFlags, "{4D596B3F-92E5-4210-A04B-584E38E87822}");
    AZ_TYPE_INFO_SPECIALIZE(ImageAspectFlags, "{7C44319C-696A-4C86-AF31-955391DC7ABB}");

}
