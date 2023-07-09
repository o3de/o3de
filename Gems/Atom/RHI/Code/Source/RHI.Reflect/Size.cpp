/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/Size.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/hash.h>

namespace AZ::RHI
{
    void Size::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<Size>()
                ->Version(1)
                ->Field("Width", &Size::m_width)
                ->Field("Height", &Size::m_height)
                ->Field("Depth", &Size::m_depth)
                ;
        }
    }

    Size::Size(uint32_t width, uint32_t height, uint32_t depth)
        : m_width{width}
        , m_height{height}
        , m_depth{depth}
    {}

    Size Size::GetReducedMip(uint32_t mipLevel) const
    {
        Size size;
        size.m_width  = AZStd::max(m_width  >> mipLevel, 1u);
        size.m_height = AZStd::max(m_height >> mipLevel, 1u);
        size.m_depth  = AZStd::max(m_depth  >> mipLevel, 1u);
        return size;
    }

    bool Size::operator == (const Size& rhs) const
    {
        return m_width == rhs.m_width && m_height == rhs.m_height && m_depth == rhs.m_depth;
    }

    bool Size::operator != (const Size& rhs) const
    {
        return m_width != rhs.m_width || m_height != rhs.m_height || m_depth != rhs.m_depth;
    }

    uint32_t& Size::operator [] (uint32_t idx)
    {
        uint32_t* ptr = &m_width;
        return *(ptr + idx);
    }

    uint32_t Size::operator [] (uint32_t idx) const
    {
        const uint32_t* ptr = &m_width;
        return *(ptr + idx);
    }
}
