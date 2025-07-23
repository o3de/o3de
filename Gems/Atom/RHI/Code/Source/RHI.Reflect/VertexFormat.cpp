/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/VertexFormat.h>

namespace AZ::RHI
{
    uint32_t GetVertexFormatSize(VertexFormat vertexFormat)
    {
        uint32_t vertexFormatSize{ Internal::GetVertexFormatSize(vertexFormat) };
        AZ_Assert(vertexFormatSize != 0, "Failed to get size of vertex format %d", vertexFormat);
        return vertexFormatSize;
    }

    VertexFormat ConvertToVertexFormat(RHI::Format format)
    {
        switch (format)
        {
        case RHI::Format::R32G32B32A32_FLOAT:
            return VertexFormat::R32G32B32A32_FLOAT;
        case RHI::Format::R32G32B32_FLOAT:
            return VertexFormat::R32G32B32_FLOAT;
        case RHI::Format::R32G32_FLOAT:
            return VertexFormat::R32G32_FLOAT;
        case RHI::Format::R16G16B16A16_FLOAT:
            return VertexFormat::R16G16B16A16_FLOAT;
        case RHI::Format::R16G16_FLOAT:
            return VertexFormat::R16G16_FLOAT;
        case RHI::Format::R8G8_UNORM:
            return VertexFormat::R8G8_UNORM;
        default:
            AZ_Assert(false, "Failed to convert format %s to vertex format", RHI::ToString(format));
            return VertexFormat::Unknown;
        }
    }

    RHI::Format ConvertToImageFormat(VertexFormat vertexFormat)
    {
        switch (vertexFormat)
        {
        case VertexFormat::R32G32B32A32_FLOAT:
            return Format::R32G32B32A32_FLOAT;
        case VertexFormat::R32G32B32_FLOAT:
            return Format::R32G32B32_FLOAT;
        case VertexFormat::R32G32_FLOAT:
            return Format::R32G32_FLOAT;
        case VertexFormat::R16G16B16A16_FLOAT:
            return Format::R16G16B16A16_FLOAT;
        case VertexFormat::R16G16_FLOAT:
            return Format::R16G16_FLOAT;
        case VertexFormat::R8G8_UNORM:
            return Format::R8G8_UNORM;
        default:
            AZ_Assert(false, "Failed to convert vertex format %d to image format", vertexFormat);
            return Format::Unknown;
        }
    }
} // namespace AZ::RHI
