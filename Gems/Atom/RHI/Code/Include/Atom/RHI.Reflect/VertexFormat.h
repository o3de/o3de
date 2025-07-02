/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Format.h>

namespace AZ::RHI
{
    // Use "Internal" namespace to avoid exposing "using uint..." to the AZ::RHI namespace
    namespace Internal
    {
        using uint = uint32_t;
#define VertexFormatEnumType : uint32_t
#include "../../../RHI/Assets/ShaderLib/Atom/RHI/VertexFormat.azsli"
#undef VertexFormatEnumType
    } // namespace Internal

    using VertexFormat = Internal::VertexFormat;

    //! @brief Returns the size of the given vertex format in bytes.
    uint32_t GetVertexFormatSize(VertexFormat vertexFormat);

    //! @brief Converts the given image format to a VertexFormat or asserts if no such conversion is possible
    VertexFormat ConvertToVertexFormat(RHI::Format format);

    //! @brief Converts the given VertexFormat to an image format or asserts if no such conversion is possible
    RHI::Format ConvertToImageFormat(VertexFormat vertexFormat);
} // namespace AZ::RHI
